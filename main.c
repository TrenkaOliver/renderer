#include <stdio.h>
#include "graphics/render.h"
#include "graphics/light.h"
#include "scene.h"
#include "camera.h"
#include "geometry/vec.h"
#include "geometry/mesh/rectangle.h"

Material sphere_material = {
    .diffuse = {.x = 0.2, .y = 0.4, .z = 1.0},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32,
};

Material plane_material = {
    .diffuse = {.x = 1.0, .y = 1.0, .z = 1.0},
    .specular = {.x = 0.0, .y = 0.0, .z = 0.0},
    .shininess = 1,
};

Material box_material = {
    .diffuse = {.x = 1.0, .y = 0.4, .z = 0.2},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32,
};

int main() {
    int width = 1280;
    int height = 720;

    FILE *f = fopen("result.ppm", "wb");
    if (!f) return 1;

    Scene scene = create_scene();
    Camera cam = create_look_at_camera(vec(0.0, -150.0, 150.0), vec(0.0, 0.0, 60.0));

    add_sphere(
        &scene,
        vec(-90.0, 20.0, 45.0),
        18.0,
        &sphere_material
    );

    add_sphere(
        &scene,
        vec(90.0, 30.0, 70.0),
        24.0,
        &sphere_material
    );

    add_sphere(
        &scene,
        vec(-40.0, 130.0, 90.0),
        20.0,
        &sphere_material
    );

    add_sphere(
        &scene,
        vec(80.0, 160.0, 55.0),
        22.0,
        &sphere_material
    );

    add_box(
        &scene,
        vec(-30.0, 10.0, 10.0),
        vec(0.3, 0.5, 0.2),
        vec(50.0, 50.0, 60.0),
        &box_material
    );

    add_box(
        &scene,
        vec(120.0, 50.0, 15.0),
        vec(0.8, 0.2, 0.6),
        vec(45.0, 70.0, 50.0),
        &box_material
    );

    add_box(
        &scene,
        vec(-140.0, 90.0, 5.0),
        vec(0.2, 0.9, 0.3),
        vec(60.0, 45.0, 80.0),
        &box_material
    );

    add_box(
        &scene,
        vec(20.0, 140.0, 20.0),
        vec(0.7, 0.4, 0.9),
        vec(65.0, 40.0, 55.0),
        &box_material
    );

    add_box(
        &scene,
        vec(-60.0, 220.0, 10.0),
        vec(0.4, 1.0, 0.5),
        vec(50.0, 50.0, 70.0),
        &box_material
    );


    add_plane(
        &scene, 
        vec(0.0, 0.0, -40.0),
        vec(0.0, 0.0, -1.0),
        &plane_material
    );


    render(f, width, height, &scene, &cam);
    fclose(f);
}