#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "accel/bvh.h"
#include "geometry/object.h"
#include "render/render.h"
#include "render/trace.h"
#include "scene/scene.h"

uint64_t visited_nodes = 0;
uint64_t primitive_tests = 0;
uint64_t first_object_called = 0;
uint64_t s_visited_nodes = 0;
uint64_t s_first_object_called = 0;
uint32_t aabb_test = 0;
uint32_t triangle_test = 0;
uint32_t total_miss = 0;


int render(FILE *f, Scene *scene, Camera *cam, RenderSettings *settings, BVH8Tree *bvh) {
    unsigned char *pa, *pp;
    int i, j, aa_i, aa_j;
    double right, up, aa_right, aa_up, aspect, fov_scale, color_scale, x_scale, inv_width, inv_height, inv_samples;
    size_t count;
    Pixel p;
    Vec c;
    Ray ray;

    fprintf(f, "P6\n");
    fprintf(f, "%d %d\n", settings->width, settings->height);
    fprintf(f, "255\n");

    count = settings->width * settings->height * 3;
    pa = pp = malloc(count);
    if (!pa) return 1;

    aspect = (double)settings->width / settings->height;
    fov_scale = tan(cam->fov * 0.5);
    color_scale = 1.0 / (settings->aa_samples * settings->aa_samples);
    x_scale = aspect * fov_scale;
    inv_width = 1.0 / settings->width;
    inv_height = 1.0 / settings->height;
    inv_samples = 1.0 / settings->aa_samples;

    for (i = 0; i < settings->height; i++) {
        uint32_t next_idx = 8;
        up = (1.0 - (i + 0.5) * inv_height * 2.0) * fov_scale;

        unsigned char *dst[8];
        int valid_lanes = 0xFF;
        __m256i ps_depth = _mm256_setzero_si256();
        Ray ray[8];
        ps_Vec c[2];
        __m256 str[2];

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

        str[0] = _mm256_setzero_ps();
        str[1] = _mm256_setzero_ps();

        for (int k = 0; k < 8; k++) {
            right = (-1.0 + (k + 0.5) * inv_width * 2.0) * x_scale;

            dst[k] = pa + ((size_t)i * settings->width + k) * 3;
            ray[k] = create_ray(
                cam->position,
                normalize(v_add(
                    cam->forward,
                    v_add(
                        scale(cam->right, right),
                        scale(cam->up, up)
                    )
                ))
            );
        }

        while(valid_lanes)  {
            TraceResult res = packed_trace_ray(ray, scene, cam, bvh, ps_depth);
            __m256 depth2 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_set1_epi32(2), ps_depth));
            __m256 depth1 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_set1_epi32(1), ps_depth));
            __m256 not0 = _mm256_or_ps(depth2, depth1);
            __m256 depth0 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_set1_epi32(0), ps_depth));
            __m256 not_valid =  _mm256_xor_ps(res.valid_ref, _mm256_castsi256_ps(_mm256_set1_epi32(-1)));

            c[1].x = _mm256_blendv_ps(c[1].x, res.c.x, depth1);
            c[1].y = _mm256_blendv_ps(c[1].y, res.c.y, depth1);
            c[1].z = _mm256_blendv_ps(c[1].z, res.c.z, depth1);
            str[1] = _mm256_blendv_ps(str[1], res.ref_str, _mm256_and_ps(depth1, res.valid_ref));

            c[0].x = _mm256_blendv_ps(c[0].x, res.c.x, depth0);
            c[0].y = _mm256_blendv_ps(c[0].y, res.c.y, depth0);
            c[0].z = _mm256_blendv_ps(c[0].z, res.c.z, depth0);
            str[0] = _mm256_blendv_ps(str[0], res.ref_str, _mm256_and_ps(depth0, res.valid_ref));

            c[1] = ps_v_add(c[1], ps_scale_ps(
                res.c,
                _mm256_blendv_ps(_mm256_setzero_ps(), str[1], depth2)
            ));
            
            __m256 accum_c0 = _mm256_and_ps(not0, not_valid);
            c[0] = ps_v_add(c[0], ps_scale_ps(
                c[1],
                _mm256_blendv_ps(_mm256_setzero_ps(), str[0], accum_c0)
            ));

            __m256 load_new = _mm256_or_ps(depth2, _mm256_andnot_ps(res.valid_ref, _mm256_or_ps(depth0, depth1)));

            int load_new_mask = _mm256_movemask_ps(load_new);

            PackedPixel p = color_to_packed_pixel(c[0]);

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

                right = (-1.0 + (idx + 0.5) * inv_width * 2.0) * x_scale;

                dst[k] = pa + ((size_t)i * settings->width + idx) * 3;
                ray[k] = create_ray(
                    cam->position,
                    normalize(v_add(
                        cam->forward,
                        v_add(
                            scale(cam->right, right),
                            scale(cam->up, up)
                        )
                    ))
                );
            }

            ps_depth = _mm256_add_epi32(ps_depth, _mm256_set1_epi32(1));
            ps_depth = _mm256_blendv_epi8(ps_depth, _mm256_setzero_si256(), _mm256_castps_si256(load_new));
        }
    }

    // printf("next: %u\n", next);

    // printf("max_i: %u\n", scene->triangle_count);

    // for (i = 0; i < settings->height; i += 2) {        
    //     for (j = 0; j < settings->width; j += 4) {            
    //         Ray ray[8];
            
    //         for (int ii = 0; ii < 2; ii++) {
    //             int y = i + ii;
    //             up = (1.0 - (y + 0.5) * inv_height * 2.0) * fov_scale;
                
    //             for (int jj = 0; jj < 4; jj++) {
    //                 int x = j + jj;
    //                 right = (-1.0 + (x + 0.5) * inv_width * 2.0) * x_scale;

    //                 ray[4 * ii + jj] = create_ray(
    //                     cam->position,
    //                     normalize(v_add(
    //                         cam->forward,
    //                         v_add(
    //                             scale(cam->right, right),
    //                             scale(cam->up, up)
    //                         )
    //                     ))
    //                 );
    //             }
    //         }

    //         ps_Vec c;// = packed_trace_ray(ray, scene, cam, bvh, settings->max_depth);

    //         PackedPixel p = color_to_packed_pixel(c);
    //         for (int k = 0; k < 8; k++) {
    //             int y = i + k / 4;
    //             int x = j + k % 4;

    //             unsigned char *dst = pa + ((size_t)y * settings->width + x) * 3;

    //             dst[0] = p.r[k];
    //             dst[1] = p.g[k];
    //             dst[2] = p.b[k];
    //         }
    //     }
    // }

    // for (i = 0; i < settings->height; i++) {
    //     for (j = 0; j < settings->width; j++) {
    //         c = vec(0.0, 0.0, 0.0);            
    //         for (aa_i = 0; aa_i < settings->aa_samples; aa_i++) {
    //             aa_up = (aa_i + 0.5) * inv_samples;
    //             up = (1.0 - (i + aa_up) * inv_height * 2.0) * fov_scale;
    //             for (aa_j = 0; aa_j < settings->aa_samples; aa_j++) {
    //                 aa_right = (aa_j + 0.5) * inv_samples;
    //                 right = (-1.0 + (j + aa_right) * inv_width * 2.0) * x_scale;

    //                 ray = create_ray(
    //                     cam->position,
    //                     normalize(v_add(
    //                         cam->forward,
    //                         v_add(
    //                             scale(cam->right, right), 
    //                             scale(cam->up, up)
    //                         )
    //                     ))
    //                 );

    //                 c = v_add(c, scale(trace_ray(&ray, scene, cam, &bvh, settings->max_depth), color_scale));
    //             }
    //         }
    //         p = color_to_pixel(c);
    //         *pp++ = p.r;
    //         *pp++ = p.g;
    //         *pp++ = p.b;
    //     }
    // }

    // printf("aabb tests: %u\n", aabb_test);
    // printf("triangle tests: %u\n", triangle_test);
    // printf("no_traversal: %u\n", total_miss);
    // printf("traversal: %u\n", 360 * 640 - total_miss);
    // aabb_test = 0;
    // triangle_test = 0;
    // total_miss = 0;

    fwrite(pa, sizeof(char), count, f);
    
    free(pa);

    return 0;
}