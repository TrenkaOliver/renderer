#include <stdio.h>
#include <time.h>
#include "render/render.h"
#include "light/material.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "math/vec.h"

Material blue = {
    .diffuse = {.x = 0.2, .y = 0.4, .z = 1.0},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32.0,
    .reflectivity = 0.0,
};

Material mirror = {
    .diffuse = {.x = 0.85, .y = 0.85, .z = 0.85},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32.0,
    .reflectivity = 0.6,
};

Material white = {
    .diffuse = {.x = 1.0, .y = 1.0, .z = 1.0},
    .specular = {.x = 0.0, .y = 0.0, .z = 0.0},
    .shininess = 1,
};

Material orange = {
    .diffuse = {.x = 1.0, .y = 0.4, .z = 0.2},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32.0,
    .reflectivity = 0.0,
};

Material green = {
    .diffuse = {.x = 0.1, .y = 1.0, .z = 0.2},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32.0,
    .reflectivity = 0.0,
};

int main() {
    clock_t start = clock();

    int width = 1280;
    int height = 720;

    FILE *f = fopen("result.ppm", "wb");
    if (!f) return 1;

    Scene scene = create_scene();
    RenderSettings settings = {.width = width, .height = height, .max_depth = 0, .aa_samples = 3};
    Camera cam = create_look_at_camera(vec(0.0, -75.0, 0.0), vec(0.0, 0.0, 0.0), 1.0472);

    scene.dir_light.dir = normalize(vec(-0.1, -0.3, 1.0));

    Mesh *body_mesh = inport_mesh(&scene, "./models/FinalBaseMesh.obj");
    set_mesh_rotation(&scene, body_mesh, vec(1.5, 0.0, 0.0));
    apply_mesh_transform(&scene, body_mesh);

    scale_mesh(&scene, body_mesh, vec(2.0, 2.0, 2.0));
    set_mesh_position(&scene, body_mesh, vec(
        body_mesh->size.x * -0.5,
        body_mesh->size.y * -0.5,
        body_mesh->size.z * -0.5
    ));

    add_sphere(&scene, body_mesh->aabb.min, 1, &green);
    add_sphere(&scene, body_mesh->aabb.max, 1, &orange);

    //Mesh *cube_mesh = inport_mesh(&scene, "./models/cube.obj");
    //set_mesh_position(&scene, cube_mesh, vec(0.0, 0.0, 0.0));

    //add_box(&scene, vec(40.0, 40.0, 0.0), vec(0.0, 0.0, 0.0), vec(20.0, 20.0, 20.0), &orange);
    //set_mesh_position(&scene, body_mesh, vec(0.0, 0.0, 0.0));

    //add_box(&scene, vec(15.0, 10.0, 0.0), vec(0.0, 0.0, 0.0), v_add(scale(scene.dir_light.dir, 10.0), vec(10.0, 1.0, 1.0)), &orange);

    //printf("size: %f, %f, %f\n", body_mesh->size.x, body_mesh->size.y, body_mesh->size.z);

    // for (int x = -300; x <= 300; x += 60) {
    //     for (int y = 0; y <= 1200; y += 60) {
    //         for (int z = 20; z <= 260; z += 60) {

    //             int idx = ((x + 300) / 60)
    //                     + ((y      ) / 60)
    //                     + ((z - 20 ) / 60);

    //             if (idx % 2 == 0) {
    //                 add_sphere(
    //                     &scene,
    //                     vec(x, y, z),
    //                     15.0,
    //                     (idx % 4 == 0) ? &mirror : &blue
    //                 );
    //             } else {
    //                 add_box(
    //                     &scene,
    //                     vec(x - 12.0, y - 12.0, z - 12.0),
    //                     vec(0.0, 0.0, 0.0),
    //                     vec(24.0, 24.0, 24.0),
    //                     &orange
    //                 );
    //             }
    //         }
    //     }
    // }

    // add_plane(
    //     &scene,
    //     vec(0.0, 0.0, 0.0),
    //     vec(0.0, 0.0, 1.0),
    //     &white
    // );


    render(f, &scene, &cam, &settings);
    fclose(f);

    clock_t end = clock();

    printf("Time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);
}