#include <stdlib.h>
#include <math.h>
#include "render/render.h"
#include "render/trace.h"
#include "scene/scene.h"
#include "geometry/object.h"

int render(FILE *f, Scene *scene, Camera *cam, RenderSettings *settings) {
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
        printf("row: %d\n", i);
        for (j = 0; j < settings->width; j++) {
            c = vec(0.0, 0.0, 0.0);            
            for (aa_i = 0; aa_i < settings->aa_samples; aa_i++) {
                aa_up = (aa_i + 0.5) * inv_samples;
                up = (1.0 - (i + aa_up) * inv_height * 2.0) * fov_scale;
                for (aa_j = 0; aa_j < settings->aa_samples; aa_j++) {
                    aa_right = (aa_j + 0.5) * inv_samples;
                    right = (-1.0 + (j + aa_right) * inv_width * 2.0) * x_scale;

                    ray = create_ray(
                        cam->position,
                        normalize(v_add(
                            cam->forward,
                            v_add(
                                scale(cam->right, right), 
                                scale(cam->up, up)
                            )
                        ))
                    );

                    c = v_add(c, scale(trace_ray(&ray, scene, cam, settings->max_depth), color_scale));
                }
            }
            p = color_to_pixel(c);
            *pp++ = p.r;
            *pp++ = p.g;
            *pp++ = p.b;
        }
    }

    fwrite(pa, sizeof(char), count, f);
    
    free(pa);

    return 0;
}