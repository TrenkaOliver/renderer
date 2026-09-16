#ifndef SCENE_H
#define SCENE_H

#include <stddef.h>
#include "math/vec.h"
#include "geometry/object.h"
#include "geometry/mesh.h"
#include "geometry/plane.h"
#include "light/light.h"
#include "light/material.h"
#include "array/array.h"

typedef struct Scene {
    DirectionalLight dir_light;
    Vec global_ambient;
    DynArray planes;
    Vertex vertices;
    BuildingTriangle triangles;
    uint32_t vertex_pos_count;
    uint32_t vertex_normal_count;
    uint32_t vertex_texcoord_count;
    uint32_t vertex_pos_capacity;
    uint32_t vertex_normal_capacity;
    uint32_t vertex_texcoord_capacity;
    uint32_t triangle_count;
    uint32_t triangle_capacity;
    DynArray meshes;
    DynArray materials;
    DynArray textures;
    //DynArray objects;
} Scene;

Scene create_scene();

size_t add_plane(Scene *scene, Vec point, Vec normal, Material *material);

size_t import_mesh(Scene *scene, char *file_name);
size_t get_material_id(char *s, DynArray *arr);
size_t add_material(char *s, DynArray *arr, Scene *scene);

//size_t add_sphere(Scene *scene, Vec center, double radious, Material *material);

size_t add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *material);
uint32_t add_vertex_pos(float x, float y, float z, Scene *scene);
uint32_t add_vertex_normal(float nx, float ny, float nz, Scene *scene);
uint32_t add_vertex_texcoord(float u, float v, Scene *scene);
uint32_t add_triangle_from_indices(uint32_t ai, uint32_t bi, uint32_t ci, uint32_t nai, uint32_t nbi, uint32_t nci, uint32_t tai, uint32_t tbi, uint32_t tci, Material *material, Scene *scene);
//size_t add_triangle_ns(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Material *material);
//size_t add_triangle_t(Scene *scene, Vec a, Vec b, Vec c, Vec ta, Vec tb, Vec tc, Material *material);
//size_t add_triangle_complete(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Vec ta, Vec tb, Vec tc, Material *material);

//size_t add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *material);

void move_mesh(Scene *scene, Mesh *mesh, Vec delta);   
void scale_mesh(Scene *scene, Mesh *mesh, Vec scale);
void rotate_mesh(Scene *scene, Mesh *mesh, Vec rotation);

void set_mesh_position(Scene *scene, Mesh *mesh, Vec position);   
void set_mesh_size(Scene *scene, Mesh *mesh, Vec size);
void set_mesh_rotation(Scene *scene, Mesh *mesh, Vec rotation);

void apply_mesh_transform(Mesh *mesh);

#endif