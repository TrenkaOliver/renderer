#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "render/render.h"
#include "light/material.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "accel/bvh.h"
#include "math/vec.h"

#define RES 2
#define SAMPLE 1

#if RES == 2

#define WIDTH 3840
#define HEIGHT 2160

#elif RES == 1

#define WIDTH 1920
#define HEIGHT 1080

#else 

#define WIDTH 640
#define HEIGHT 360

#endif

#define AA 1
#define MAX_DEPTH 2

int create_import_scene(void);

int main(void) {
    return create_import_scene();
}

int create_import_scene(void) {
    srand(12345);

    const int width = WIDTH;
    const int height = HEIGHT;

    FILE *f = fopen("results/result.ppm", "wb");

    if (!f) return 1;

    Scene scene = create_scene();

    RenderSettings settings = {
        .width = width,
        .height = height,

        .max_depth = MAX_DEPTH,

        .aa_samples = AA
    };

    //import_glTF(&scene, "./glTF/Spheres/MetalRoughSpheres.gltf");
    import_glTF(&scene, "./glTF/DamagedHelmet/DamagedHelmet.gltf");
    import_glTF(&scene, "./glTF/DamagedHelmet/DamagedHelmet.gltf");
    import_env(&scene, "./glTF/env/hangar.hdr");

    Mesh *mesh0 = (Mesh *)get_element(0, &scene.meshes);
    Mesh *mesh1 = (Mesh *)get_element(1, &scene.meshes);
    rotate_mesh(&scene, mesh0, vec(0.0, 3.14, 2.0));
    rotate_mesh(&scene, mesh1, vec(0.0, 3.14, -1.7));
    
    move_mesh(&scene, mesh0, vec(1.0, 0.0, 0.0));
    move_mesh(&scene, mesh1, vec(-1.0, 1.0, 0.0));

    Vec pos0 = ((Mesh *)get_element(0, &scene.meshes))->position;
    Vec pos1 = ((Mesh *)get_element(1, &scene.meshes))->position;

    printf("%u\n", scene.triangle_count);

    Camera cam = create_look_at_camera(
        v_add(scale(v_add(pos0, pos1), 0.5), vec(0.0, -3.0, 0.0)),
        scale(v_add(pos0, pos1), 0.5),
        1.0472
    );

    //add_plane(&scene, v_add(pos0, vec(0.0, 0.0, -5.0)), vec(0.0, 0.0, 1.0), get_element(0, &scene.materials))


    scene.dir_light = (DirectionalLight) {
        .dir = normalize(vec(-0.5, -0.8, 1.0)),
        .color = vec(1.0, 0.95, 0.9),
        .intensity = 1,
    };

    scene.dir_light.neg_dir = neg(scene.dir_light.dir);
    scene.dir_light.scaled_color = scale(scene.dir_light.color, scene.dir_light.intensity);

    clock_t bvh_start = clock();
    BVH8Tree bvh = create_bvh8_tree(&scene.triangles, scene.vertex_data, scene.triangle_count);
    clock_t bvh_end = clock();

    printf("Bvh creation: %.3f s\n", (double)(bvh_end - bvh_start) / CLOCKS_PER_SEC);

    clock_t render_start = clock();

    for (int i = 0; i < SAMPLE; i++) {
        render(f, &scene, &cam, &settings, &bvh);
    }

    clock_t render_end = clock();

    printf("Total render time: %.3f s\n",
        (double)(render_end - render_start) / CLOCKS_PER_SEC);

    printf("Average render time: %.3f s\n",
        (double)(render_end - render_start) / CLOCKS_PER_SEC / (double)SAMPLE);

    fclose(f);

    return 0;
        
}