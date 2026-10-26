#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "accel/bvh.h"
#include "render/render.h"
#include "render/trace.h"
#include "scene/scene.h"

typedef struct PackedPixel {
    uint8_t r[8];
    uint8_t g[8];
    uint8_t b[8];
} PackedPixel;

static inline PackedPixel color_to_packed_pixel(const ps_Vec c) {
    const __m256 one = _mm256_set1_ps(1.0f);
    const __m256 scale = _mm256_set1_ps(255.0f);

    const __m256i r = _mm256_cvttps_epi32(_mm256_mul_ps(_mm256_min_ps(c.x, one), scale));
    const __m256i g = _mm256_cvttps_epi32(_mm256_mul_ps(_mm256_min_ps(c.y, one), scale));
    const __m256i b = _mm256_cvttps_epi32(_mm256_mul_ps(_mm256_min_ps(c.z, one), scale));

    uint32_t r_array[8], g_array[8], b_array[8];

    _mm256_storeu_si256((__m256i*)r_array, r);
    _mm256_storeu_si256((__m256i*)g_array, g);
    _mm256_storeu_si256((__m256i*)b_array, b);

    PackedPixel packed_pixel;

    for (int i = 0; i < 8; i++) {
        packed_pixel.r[i] = (uint8_t)r_array[i];
        packed_pixel.g[i] = (uint8_t)g_array[i];
        packed_pixel.b[i] = (uint8_t)b_array[i];
    }

    return packed_pixel;
}

int render(FILE * restrict f, const Scene * restrict scene, const Camera * restrict cam, const RenderSettings * restrict settings, const BVH8Tree * restrict bvh) {

    fprintf(f, "P6\n");
    fprintf(f, "%d %d\n", settings->width, settings->height);
    fprintf(f, "255\n");

    size_t count = settings->width * settings->height * 3;
    unsigned char *pa = malloc(count);
    if (!pa) return 1;

    double aspect = (double)settings->width / settings->height;
    double fov_scale = tan(cam->fov * 0.5);
    double x_scale = aspect * fov_scale;
    double inv_width = 1.0 / settings->width;
    double inv_height = 1.0 / settings->height;

    for (int i = 0; i < settings->height; i++) {
        int next_idx = 8;
        double up = (1.0 - (i + 0.5) * inv_height * 2.0) * fov_scale;

        unsigned char *dst[8];
        int valid_lanes = 0xFF;
        __m256i ps_depth = _mm256_setzero_si256();
        SoARay ray;
        ps_Vec c[2];
        ps_Vec mul[2];

        c[0] = (ps_Vec){
            .x = _mm256_setzero_ps(),
            .y = _mm256_setzero_ps(),
            .z = _mm256_setzero_ps()
        };
        c[1] = (ps_Vec){
            .x = _mm256_setzero_ps(),
            .y = _mm256_setzero_ps(),
            .z = _mm256_setzero_ps()
        };
        mul[0] = (ps_Vec){
            .x = _mm256_setzero_ps(),
            .y = _mm256_setzero_ps(),
            .z = _mm256_setzero_ps()
        };
        mul[1] = (ps_Vec){
            .x = _mm256_setzero_ps(),
            .y = _mm256_setzero_ps(),
            .z = _mm256_setzero_ps()
        };

        for (int k = 0; k < 8; k++) {
            double right = (-1.0 + (k + 0.5) * inv_width * 2.0) * x_scale;

            dst[k] = pa + ((size_t)i * settings->width + k) * 3;
            // Ray r = create_ray(
            //     cam->position,
            //     normalize(v_add(
            //         cam->forward,
            //         v_add(
            //             scale(cam->right, right),
            //             scale(cam->up, up)
            //         )
            //     ))
            // );

            Vec v = normalize(v_add(
                cam->forward,
                v_add(
                    scale(cam->right, right),
                    scale(cam->up, up)
                )
            ));

            ray.ox[k] = cam->position.x;
            ray.oy[k] = cam->position.y;
            ray.oz[k] = cam->position.z;
            ray.vx[k] = v.x;
            ray.vy[k] = v.y;
            ray.vz[k] = v.z;
        }

        while(valid_lanes)  {
            const TraceResult res = packed_trace_ray(&ray, scene, cam, bvh, ps_depth);

            const __m256 depth2 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_set1_epi32(2), ps_depth));
            const __m256 depth1 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_set1_epi32(1), ps_depth));
            const __m256 not0 = _mm256_or_ps(depth2, depth1);
            const __m256 depth0 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_set1_epi32(0), ps_depth));
            const __m256 not_valid =  _mm256_xor_ps(res.valid_ref, _mm256_castsi256_ps(_mm256_set1_epi32(-1)));

            c[1].x = _mm256_blendv_ps(c[1].x, res.c.x, depth1);
            c[1].y = _mm256_blendv_ps(c[1].y, res.c.y, depth1);
            c[1].z = _mm256_blendv_ps(c[1].z, res.c.z, depth1);
            const __m256 blend_mul1 = _mm256_and_ps(depth1, res.valid_ref);
            mul[1].x = _mm256_blendv_ps(mul[1].x, res.ref_mul.x, blend_mul1);
            mul[1].y = _mm256_blendv_ps(mul[1].y, res.ref_mul.y, blend_mul1);
            mul[1].z = _mm256_blendv_ps(mul[1].z, res.ref_mul.z, blend_mul1);

            c[0].x = _mm256_blendv_ps(c[0].x, res.c.x, depth0);
            c[0].y = _mm256_blendv_ps(c[0].y, res.c.y, depth0);
            c[0].z = _mm256_blendv_ps(c[0].z, res.c.z, depth0);
            const __m256 blend_mul0 = _mm256_and_ps(depth0, res.valid_ref);
            mul[0].x = _mm256_blendv_ps(mul[0].x, res.ref_mul.x, blend_mul0);
            mul[0].y = _mm256_blendv_ps(mul[0].y, res.ref_mul.y, blend_mul0);
            mul[0].z = _mm256_blendv_ps(mul[0].z, res.ref_mul.z, blend_mul0);

            c[1] = ps_v_add(c[1], ps_hadamard(
                res.c,
                (ps_Vec) {
                    .x = _mm256_and_ps(mul[1].x, depth2),
                    .y = _mm256_and_ps(mul[1].y, depth2),
                    .z = _mm256_and_ps(mul[1].z, depth2),
                }
            ));
            
            const __m256 accum_c0 = _mm256_and_ps(not0, not_valid);
            c[0] = ps_v_add(c[0], ps_hadamard(
                c[1],
                (ps_Vec) {
                    .x = _mm256_and_ps(mul[0].x, accum_c0),
                    .y = _mm256_and_ps(mul[0].y, accum_c0),
                    .z = _mm256_and_ps(mul[0].z, accum_c0),
                }
            ));

            const __m256 load_new = _mm256_or_ps(depth2, _mm256_andnot_ps(res.valid_ref, _mm256_or_ps(depth0, depth1)));

            const int load_new_mask = _mm256_movemask_ps(load_new);

            const PackedPixel p = color_to_packed_pixel(c[0]);

            for (int k = 0; k < 8; k++) {
                int mask = 1u << k;
                if (!(load_new_mask & mask)) continue;

                if (valid_lanes & mask) {
                    dst[k][0] = p.r[k];
                    dst[k][1] = p.g[k];
                    dst[k][2] = p.b[k];
                }

                int idx;

                if (next_idx >= settings->width) {
                    valid_lanes &= ~mask;
                    idx = -1;
                } else {
                    idx = next_idx++;
                }

                double right = (-1.0 + (idx + 0.5) * inv_width * 2.0) * x_scale;

                dst[k] = pa + ((size_t)i * settings->width + idx) * 3;
                // Ray r = create_ray(
                //     cam->position,
                //     normalize(v_add(
                //         cam->forward,
                //         v_add(
                //             scale(cam->right, right),
                //             scale(cam->up, up)
                //         )
                //     ))
                // );

                Vec v = normalize(v_add(
                    cam->forward,
                    v_add(
                        scale(cam->right, right),
                        scale(cam->up, up)
                    )
                ));

                ray.ox[k] = cam->position.x;
                ray.oy[k] = cam->position.y;
                ray.oz[k] = cam->position.z;
                ray.vx[k] = v.x;
                ray.vy[k] = v.y;
                ray.vz[k] = v.z;
            }

            ps_depth = _mm256_add_epi32(ps_depth, _mm256_set1_epi32(1));
            ps_depth = _mm256_blendv_epi8(ps_depth, _mm256_setzero_si256(), _mm256_castps_si256(load_new));
        }
    }

    fwrite(pa, sizeof(char), count, f);
    
    free(pa);

    return 0;
}