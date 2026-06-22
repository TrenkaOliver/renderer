#ifndef SCENE_H
#define SCENE_H

#include <stddef.h>
#include "math/vec.h"
#include "geometry/object.h"
#include "geometry/plane.h"
#include "light/light.h"
#include "light/material.h"

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


typedef struct Scene {
    DirectionalLight dir_light;
    Vec global_ambient;
    Planes planes;
    Objects objects;
} Scene;

Scene create_scene();

Plane *add_plane(Scene *scene, Vec point, Vec normal, Material *material);

Object *add_object(Scene *scene);

Object *add_sphere(Scene *scene, Vec center, double radious, Material *material);
Object *add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *material);
Object *add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *material);

#endif