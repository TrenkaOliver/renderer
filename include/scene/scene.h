#ifndef SCENE_H
#define SCENE_H

#include <stddef.h>
#include "math/vec.h"
#include "geometry/object.h"
#include "geometry/mesh.h"
#include "geometry/plane.h"
#include "light/light.h"
#include "light/material.h"

typedef struct Planes {
    Plane *ptr;
    size_t count;
    size_t capacity;
} Planes;

typedef struct Objects {
    Object *ptr;
    size_t count;
    size_t capacity;
} Objects;

typedef struct Meshes {
    Mesh *ptr;
    size_t count;
    size_t capacity;
} Meshes;

typedef struct Scene {
    DirectionalLight dir_light;
    Vec global_ambient;
    Planes planes;
    Objects objects;
    Meshes meshes;
} Scene;

Scene create_scene();

Plane *add_plane(Scene *scene, Vec point, Vec normal, Material *material);

Object *add_object(Scene *scene);
Mesh *add_mesh(Scene *scene);
Mesh *inport_mesh(Scene *scene, char *file_name);

Object *add_sphere(Scene *scene, Vec center, double radious, Material *material);
Object *add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *material);
Object *add_triangle_ns(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Material *material);
Object *add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *material);


void set_mesh_position(Scene *scene, Mesh *mesh, Vec position);   
void set_mesh_size(Scene *scene, Mesh *mesh, Vec size);
void set_mesh_rotation(Scene *scene, Mesh *mesh, Vec rotation);

#endif