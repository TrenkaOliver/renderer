#include <stdlib.h>
#include <math.h>
#include "graphics/render.h"
#include "scene.h"
#include "geometry/object.h"
#include "graphics/trace.h"

int render(FILE *f, int width, int height, Scene *scene, Camera *cam) {
    unsigned char *pa, *pp;
    int i, j;
    double right, up, aspect;
    size_t count;
    Pixel p;
    Vec c;
    Line ray;

    fprintf(f, "P6\n");
    fprintf(f, "%d %d\n", width, height);
    fprintf(f, "255\n");

    count = width * height * 3;
    pa = pp = malloc(count);
    if (!pa) return 1;

    aspect = (double)width / height;

    for (i = 0; i < height; i++) {
        up = 1.0 - (double)i / height * 2.0;
        for (j = 0; j < width; j++) {
            right = (-1.0 + (double)j / width * 2.0) * aspect;

            ray = create_line(
                cam->position,
                normalize(v_add(
                    cam->forward,
                    v_add(
                        scale(cam->right, right), 
                        scale(cam->up, up)
                    )
                ))
            );

            c = trace_ray(&ray, scene, cam, 3);

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