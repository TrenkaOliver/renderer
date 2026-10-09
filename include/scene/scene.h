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

#define ENV_MIP_COUNT       6

#define IRRADIANCE_SAMPLES  256
#define PREFILTER_SAMPLES   256
#define BRDF_SAMPLES        256



typedef struct Scene {
    DirectionalLight dir_light;
    Vec global_ambient;
    Texture environment;
    Texture irradiance;
    Texture prefiltered[6];
    Texture brdf_lut;
    DynArray planes;
    BuildingTriangle triangles;
    uint32_t vertex_info[6]; // p_len, p_cap, n_len, n_cal, t_len, t_cap
    float *vertex_data[3]; //p(x, y, z), n(x, y, z), t(u, v)
    uint32_t triangle_count;
    uint32_t triangle_capacity;
    DynArray meshes;
    DynArray materials;
    DynArray textures;
} Scene;

Scene create_scene();

size_t add_plane(Scene *scene, Vec point, Vec normal, Material *material);

size_t import_obj_mesh(Scene *scene, char *file_name);
uint32_t import_glTF(Scene *scene, char *file_name);
uint32_t import_env(Scene *scene, char *file_name);

Vec sample(Vec N, Texture *map);
Vec sample_prefiltered(Vec R, double roughness, Texture maps[]);
Vec sample_brdf_lut(double NdotV, double roughness, Texture *map);

double radical_inverse(unsigned k);
void hammersley(uint32_t i, uint32_t n, double *u, double *v);
void make_coord_space(Vec N, Vec *T, Vec *B);
Vec importance_sample_GGX(double u1, double u2, double roughness, Vec N);

size_t get_material_id(char *s, DynArray *arr);
size_t add_material(char *s, DynArray *arr, Scene *scene);

//size_t add_sphere(Scene *scene, Vec center, double radious, Material *material);

size_t add_triangle(Scene *scene, Vec a, Vec b, Vec c, uint32_t material);
uint32_t add_vertex_pos(float x, float y, float z, Scene *scene);
uint32_t add_vertex_normal(float nx, float ny, float nz, Scene *scene);
uint32_t add_vertex_texcoord(float u, float v, Scene *scene);
uint32_t add_triangle_from_indices(uint32_t ai, uint32_t bi, uint32_t ci, uint32_t nai, uint32_t nbi, uint32_t nci, uint32_t tai, uint32_t tbi, uint32_t tci, uint32_t material, Scene *scene);
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

uint32_t clone_mesh(Scene *scene, uint32_t id);

#endif