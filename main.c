#include <stdio.h>
#include "graphics/render.h"
#include "graphics/light.h"
#include "scene.h"
#include "camera.h"
#include "geometry/vec.h"
#include "geometry/mesh/rectangle.h"

Material blue = {
    .diffuse = {.x = 0.2, .y = 0.4, .z = 1.0},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32.0,
    .reflectivity = 0.0,
};

Material mirror = {
    .diffuse = {.x = 0.7, .y = 0.7, .z = 1.0},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32.0,
    .reflectivity = 0.5,
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
    .diffuse = {.x = 0.1, .y = 0.1, .z = 0.2},
    .specular = {.x = 0.4, .y = 0.4, .z = 0.4},
    .shininess = 32.0,
    .reflectivity = 0.0,
};

int main() {
    int width = 1280;
    int height = 720;

    FILE *f = fopen("result.ppm", "wb");
    if (!f) return 1;

    Scene scene = create_scene();
    Camera cam = create_look_at_camera(vec(0.0, 0.0, 125.0), vec(0.0, 1.0, 125.0));

    add_sphere(&scene, vec(0.0, 100.0, 125.0), 25.0, &mirror);

    add_box(&scene, vec(-120.0, 0.0, 0.0), vec(0.0, 0.0, 0.0), vec(10.0, 300.0, 300.0), &blue);

    add_box(&scene, vec(120.0, 0.0, 0.0), vec(0.0, 0.0, 0.0), vec(10.0, 300.0, 300.0), &orange);



    add_plane(
        &scene, 
        vec(0.0, 0.0, 0.0),
        vec(0.0, 0.0, 1.0),
        &white
    );


    render(f, width, height, &scene, &cam);
    fclose(f);
}