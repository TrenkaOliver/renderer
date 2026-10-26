#ifndef SCENE_H
#define SCENE_H

#include <stddef.h>
#include "math/vec.h"
#include "geometry/triangle.h"
#include "geometry/mesh.h"
#include "light/light.h"
#include "light/material.h"
#include "array/array.h"

typedef struct Scene {
    DirectionalLight dir_light;
    Vec global_ambient;
    Texture environment;
    Texture irradiance;
    Texture prefiltered[6];
    Texture brdf_lut;
    OriginalTriangle triangles;
    uint32_t vertex_info[6]; // p_len, p_cap, n_len, n_cal, t_len, t_cap
    float *vertex_data[3]; //p(x, y, z), n(x, y, z), t(u, v)
    uint32_t triangle_count;
    uint32_t triangle_capacity;
    DynArray meshes;
    DynArray materials;
    DynArray textures;
} Scene;

Scene create_scene();

//size_t add_sphere(Scene *scene, Vec center, double radious, Material *material);

size_t add_triangle(Scene *scene, Vec a, Vec b, Vec c, uint32_t material);

void move_mesh(Scene *scene, Mesh *mesh, Vec delta);   
void scale_mesh(Scene *scene, Mesh *mesh, Vec scale);
void rotate_mesh(Scene *scene, Mesh *mesh, Vec rotation);

void set_mesh_position(Scene *scene, Mesh *mesh, Vec position);   
void set_mesh_size(Scene *scene, Mesh *mesh, Vec size);
void set_mesh_rotation(Scene *scene, Mesh *mesh, Vec rotation);

void apply_mesh_transform(Mesh *mesh);

size_t import_obj_mesh(Scene *scene, char *file_name);
uint32_t import_glTF(Scene *scene, char *file_name);
uint32_t import_env(Scene *scene, char *file_name);

#endif