#ifndef SCENE_H
#define SCENE_H

#include "geometry/object.h"
#include "geometry/mesh/rectangle.h"
#include "geometry/vec.h"
#include "graphics/light.h"

typedef struct Planes {
    Plane *pp;
    size_t count;
    size_t capacity;
} Planes;

typedef struct Objects {
    Object *ptr;
    size_t count;
    size_t capacity;
} Objects;


// typedef struct Spheres {
//     Sphere *sp;
//     size_t count;
//     size_t capacity;
// } Spheres;


// typedef struct Triangles {
//     Triangle *tp;
//     size_t count;
//     size_t capacity;
// } Triangles;

// typedef struct Boxes {
//     Box *bp;
//     size_t count;
//     size_t capacity;
// } Boxes;


typedef struct Scene {
    DirectionalLight dir_light;
    Vec global_ambient;
    Planes planes;
    Objects objects;
    // // Spheres spheres;
    // Triangles triangles;
    // Boxes boxes;
} Scene;

Scene create_scene();

Plane *add_plane(Scene *scene, Vec point, Vec normal, Material *material);

Object *add_object(Scene *scene);

Object *add_sphere(Scene *scene, Vec center, double radious, Material *material);
Object *add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *material);
Object *add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *material);

// Sphere *add_sphere(Scene *scene, Vec center, double radious, Material *material);


// Triangle *add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *material);

// Box *add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *m);

// Rectangle add_rectangle(Scene *scene, Vec position, Vec rotation, Vec size, Material *m);
// Rectangle add_rectangle_by_points(Scene *scene, Vec a, Vec b, Vec c, Vec d, Material *m);
// Rectangle copy_rectangle(Scene *scene, Rectangle *rect);

#endif