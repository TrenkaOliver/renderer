#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "accel/bvh.h"
#include "geometry/object.h"
#include "render/render.h"
#include "render/trace.h"
#include "scene/scene.h"
#include "math/packed_vec.h"
#include "math/packed_ray.h"

int render(FILE *f, Scene *scene, Camera *cam, RenderSettings *settings) {
    unsigned char *pa, *pp;
    int i, j, aa_i, aa_j;
    double right, up, aa_right, aa_up, aspect, fov_scale, color_scale, x_scale, inv_width, inv_height, inv_samples;
    size_t count;
    PackedPixel p;
    Ray ray;
    BVH bvh;

    clock_t bvh_start = clock();

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

    bvh = create_bvh(scene->objects.ptr, scene->objects.count);

    clock_t bvh_end = clock();

    printf("Bvh creation: %.3f s\n", (double)(bvh_end - bvh_start) / CLOCKS_PER_SEC);

    clock_t render_start = clock();

    ps_Vec ps_cam_pos = ps_from_vec(cam->position);
    ps_Vec ps_cam_forward = ps_from_vec(cam->forward);
    ps_Vec ps_cam_up = ps_from_vec(cam->up);
    ps_Vec ps_cam_right = ps_from_vec(cam->right);
    ps_Vec c;
    PackedRay ps_ray;

    for (i = 0; i < settings->height; i+= 2) {
        __m256 up = _mm256_set_ps(i + 1, i + 1, i + 1, i + 1, i, i, i, i);
        up = _mm256_fmadd_ps(up, _mm256_set1_ps(inv_height * 2.0f), _mm256_set1_ps(1.0f));
        up = _mm256_mul_ps(up, _mm256_set1_ps(fov_scale));
        for (j = 0; j < settings->width; j+= 4) {
            __m256 right = _mm256_set_ps(j + 3, j + 2, j + 1, j, j + 3, j + 2, j + 1, j);
            right = _mm256_fmadd_ps(right, _mm256_set1_ps(inv_width * 2.0f), _mm256_set1_ps(-1.0f));
            right = _mm256_mul_ps(right, _mm256_set1_ps(x_scale));

            c = ps_zero_vec();
            // for (aa_i = 0; aa_i < settings->aa_samples; aa_i++) {
            //     aa_up = (aa_i + 0.5) * inv_samples;
            //     up = (1.0 - (i + aa_up) * inv_height * 2.0) * fov_scale;
            //     for (aa_j = 0; aa_j < settings->aa_samples; aa_j++) {
            //         aa_right = (aa_j + 0.5) * inv_samples;
            //         right = (-1.0 + (j + aa_right) * inv_width * 2.0) * x_scale;

            //         ray = create_ray(
            //             cam->position,
            //             normalize(v_add(
            //                 cam->forward,
            //                 v_add(
            //                     scale(cam->right, right), 
            //                     scale(cam->up, up)
            //                 )
            //             ))
            //         );

            //         c = v_add(c, scale(trace_ray(&ray, scene, cam, &bvh, settings->max_depth), color_scale));
            //     }
            // }

            ps_ray = ps_create_ray(
                ps_cam_pos,
                ps_normalize(ps_v_add(
                    ps_cam_forward,
                    ps_v_add(
                        ps_scale_ps(ps_cam_right, right),
                        ps_scale_ps(ps_cam_up, up)
                    )
                ))
            );

            p = color_to_packed_pixel(c);
            for (int k = 0; k < 8; k++) {
                *pp++ = p.r[k];
                *pp++ = p.g[k];
                *pp++ = p.b[k];
            }
        }
    }

    fwrite(pa, sizeof(char), count, f);
    
    free(pa);

    clock_t render_end = clock();

    printf("Rendering: %.3f s\n", (double)(render_end - render_start) / CLOCKS_PER_SEC);

    return 0;
}