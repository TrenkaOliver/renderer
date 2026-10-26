#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include "scene/scene.h"
#include "scene/sample.h"
#include "light/light.h"
#include "light/material.h"
#include "render/trace.h"
#include "stb_image.h"


Vec RIGHT = {.x = 1.0, .y = 0.0, .z = 0.0};
Vec FORWARD = {.x = 0.0, .y = 1.0, .z = 0.0};
Vec UP = {.x = 0.0, .y = 0.0, .z = 1.0};

Material no_material = {
    .base_color_factor = {.x = 1.0, .y = 1.0, .z = 1.0},
    .metallic = 0.0,
    .roughness = 0.0,
    .base_color_map = (uint32_t)-1,
    .normal_map = (uint32_t)-1,
    .metallic_roughness_map = (uint32_t)-1
};

Scene create_scene() {
    Scene scene;

    scene.meshes = create_dyn_array(sizeof(Mesh), 16);
    scene.materials = create_dyn_array(sizeof(Material), 16);
    scene.textures = create_dyn_array(sizeof(Texture), 4);

    scene.vertex_info[0] = 0;
    scene.vertex_info[1] = 128;
    scene.vertex_info[2] = 0;
    scene.vertex_info[3] = 128;
    scene.vertex_info[4] = 0;
    scene.vertex_info[5] = 96;

    scene.vertex_data[0] = malloc(scene.vertex_info[1] * 3 * sizeof(float));
    scene.vertex_data[1] = malloc(scene.vertex_info[3] * 3 * sizeof(float));
    scene.vertex_data[2] = malloc(scene.vertex_info[5] * 2 * sizeof(float));

    scene.triangle_count = 0;
    scene.triangle_capacity = 128;

    scene.triangles = (OriginalTriangle){
        .ai = malloc(scene.triangle_capacity * sizeof(uint32_t)),
        .bi = malloc(scene.triangle_capacity * sizeof(uint32_t)),
        .ci = malloc(scene.triangle_capacity * sizeof(uint32_t)),

        .nx = malloc(scene.triangle_capacity * sizeof(float)),
        .ny = malloc(scene.triangle_capacity * sizeof(float)),
        .nz = malloc(scene.triangle_capacity * sizeof(float)),

        .aabb = malloc(scene.triangle_capacity * sizeof(AABB)),
        .centroid = malloc(scene.triangle_capacity * 3 * sizeof(float)),
        .material = malloc(scene.triangle_capacity * sizeof(uint32_t))
    };

    scene.dir_light = (DirectionalLight){
        .dir = normalize(vec(0.0, 0.0, 1.0)),
        .color = vec(1.0, 0.95, 0.9),
        .intensity = 1,
    };

    scene.dir_light.neg_dir = neg(scene.dir_light.dir);
    scene.dir_light.scaled_color = scale(scene.dir_light.color, scene.dir_light.intensity);

    scene.global_ambient = vec(0.33, 0.33, 0.33);

    return scene;
}

uint32_t import_env(Scene *scene, char *file_name) {
    int w, h;
    
    unsigned char *ptr = (unsigned char *)stbi_loadf(file_name, &w, &h, NULL, 3);

    if (!ptr) return 0;

    scene->environment.w = (uint32_t)w;
    scene->environment.h = (uint32_t)h;
    scene->environment.ptr = ptr;

    generate_irradiance_map(&scene->environment, &scene->irradiance);

    for (int i = 0; i < ENV_MIP_COUNT; i++) {
        double roughness = (double)i / (double)(ENV_MIP_COUNT - 1);

        generate_prefiltered_layer(&scene->environment, scene->prefiltered + i, roughness);
    }

    generate_brdf_lut(&scene->brdf_lut);

    return 1;
}

size_t add_triangle(Scene *scene, Vec a, Vec b, Vec c, uint32_t m) {
    scene->vertex_info[0] += 3;
    scene->triangle_count += 1;

    if (scene->vertex_info[1] < scene->vertex_info[0]) {
        scene->vertex_data[0] = realloc(scene->vertex_data[0], (scene->vertex_info[0]) * 12);
        scene->vertex_info[1] = scene->vertex_info[0];
    }

    scene->vertex_info[2] += 3;
    scene->triangle_count += 1;

    if (scene->vertex_info[3] < scene->vertex_info[2]) {
        scene->vertex_data[2] = realloc(scene->vertex_data[2], (scene->vertex_info[2]) * 12);
        scene->vertex_info[3] = scene->vertex_info[2];
    }

    scene->vertex_info[4] += 2;
    scene->triangle_count += 1;

    if (scene->vertex_info[5] < scene->vertex_info[4]) {
        scene->vertex_data[4] = realloc(scene->vertex_data[4], (scene->vertex_info[0]) * 8);
        scene->vertex_info[5] = scene->vertex_info[4];
    }

    if (scene->triangle_capacity < scene->triangle_count) {
        scene->triangles.ai = realloc(scene->triangles.ai, (scene->triangle_count) * sizeof(uint32_t));
        scene->triangles.bi = realloc(scene->triangles.bi, (scene->triangle_count) * sizeof(uint32_t));
        scene->triangles.ci = realloc(scene->triangles.ci, (scene->triangle_count) * sizeof(uint32_t));
        scene->triangles.nx = realloc(scene->triangles.nx, (scene->triangle_count) * sizeof(float));
        scene->triangles.ny = realloc(scene->triangles.ny, (scene->triangle_count) * sizeof(float));
        scene->triangles.nz = realloc(scene->triangles.nz, (scene->triangle_count) * sizeof(float));
        scene->triangles.aabb = realloc(scene->triangles.aabb, (scene->triangle_count) * sizeof(AABB));
        scene->triangles.centroid = realloc(scene->triangles.centroid, (scene->triangle_count) * sizeof(float) * 3);
        scene->triangles.material = realloc(scene->triangles.material, (scene->triangle_count) * sizeof(uint32_t));
        scene->triangle_capacity = scene->triangle_count;
    }

    scene->vertex_data[0][3 * scene->vertex_info[1] - 9] = a.x;
    scene->vertex_data[0][3 * scene->vertex_info[1] - 8] = a.y;
    scene->vertex_data[0][3 * scene->vertex_info[1] - 7] = a.z;

    scene->vertex_data[0][3 * scene->vertex_info[1] - 6] = b.x;
    scene->vertex_data[0][3 * scene->vertex_info[1] - 5] = b.y;
    scene->vertex_data[0][3 * scene->vertex_info[1] - 4] = b.z;

    scene->vertex_data[0][3 * scene->vertex_info[1] - 3] = c.x;
    scene->vertex_data[0][3 * scene->vertex_info[1] - 2] = c.y;
    scene->vertex_data[0][3 * scene->vertex_info[1] - 1] = c.z;

    scene->triangles.ai[scene->triangle_count - 1] = scene->vertex_info[1] - 3;
    scene->triangles.bi[scene->triangle_count - 1] = scene->vertex_info[1] - 2;
    scene->triangles.ci[scene->triangle_count - 1] = scene->vertex_info[1] - 1;

    Vec ng = normalize(cross(v_sub(b, a), v_sub(c, a)));

    scene->triangles.nx[scene->triangle_count - 1] = ng.x;
    scene->triangles.ny[scene->triangle_count - 1] = ng.y;
    scene->triangles.nz[scene->triangle_count - 1] = ng.z;

    scene->vertex_data[1][3 * scene->vertex_info[1] - 9] = ng.x;
    scene->vertex_data[1][3 * scene->vertex_info[1] - 8] = ng.y;
    scene->vertex_data[1][3 * scene->vertex_info[1] - 7] = ng.z;

    scene->vertex_data[1][3 * scene->vertex_info[1] - 6] = ng.x;
    scene->vertex_data[1][3 * scene->vertex_info[1] - 5] = ng.y;
    scene->vertex_data[1][3 * scene->vertex_info[1] - 4] = ng.z;

    scene->vertex_data[1][3 * scene->vertex_info[1] - 3] = ng.x;
    scene->vertex_data[1][3 * scene->vertex_info[1] - 2] = ng.y;
    scene->vertex_data[1][3 * scene->vertex_info[1] - 1] = ng.z;


    scene->vertex_data[2][2 * scene->vertex_info[1] - 6] = 0.0f;
    scene->vertex_data[2][2 * scene->vertex_info[1] - 5] = 0.0f;
    scene->vertex_data[2][2 * scene->vertex_info[1] - 4] = 0.0f;

    scene->vertex_data[2][2 * scene->vertex_info[1] - 3] = 0.0f;
    scene->vertex_data[2][2 * scene->vertex_info[1] - 2] = 0.0f;
    scene->vertex_data[2][2 * scene->vertex_info[1] - 1] = 0.0f;


    scene->triangles.aabb[scene->triangle_count - 1] = (AABB){
        .min = {fminf(fminf(a.x, b.x), c.x), fminf(fminf(a.y, b.y), c.y), fminf(fminf(a.z, b.z), c.z)},
        .max = {fmaxf(fmaxf(a.x, b.x), c.x), fmaxf(fmaxf(a.y, b.y), c.y), fmaxf(fmaxf(a.z, b.z), c.z)}
    };

    scene->triangles.centroid[(scene->triangle_count - 1) * 3 + 0] = (float)(scene->triangles.aabb[scene->triangle_count - 1].min[0] + scene->triangles.aabb[scene->triangle_count - 1].max[0]) / 2.0f;
    scene->triangles.centroid[(scene->triangle_count - 1) * 3 + 1] = (float)(scene->triangles.aabb[scene->triangle_count - 1].min[1] + scene->triangles.aabb[scene->triangle_count - 1].max[1]) / 2.0f;
    scene->triangles.centroid[(scene->triangle_count - 1) * 3 + 2] = (float)(scene->triangles.aabb[scene->triangle_count - 1].min[2] + scene->triangles.aabb[scene->triangle_count - 1].max[2]) / 2.0f;

    scene->triangles.material[scene->triangle_count - 1] = m;

    return scene->triangle_count - 1;
}