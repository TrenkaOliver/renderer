#ifndef MATERIAL_H
#define MATERIAL_H

#include "math/vec.h"
#include "math/packed_vec.h"
#include "array/array.h"
#include "stdint.h"

typedef struct Material {
    Vec base_color_factor;
    float metallic;
    float roughness;
    Vec emissive_factor;
    float normal_scale;
    float occlusion_strength;

    uint32_t base_color_map;
    uint32_t metallic_roughness_map;
    uint32_t emissive_map;
    uint32_t normal_map;
    uint32_t occlusion_map;
} Material;

typedef struct Texture {
    int w;
    int h;
    unsigned char *ptr;
} Texture;

typedef struct PackedMaterial {
    ps_Vec base_color_factor;
    __m256 metallic;
    __m256 roughness;
    ps_Vec emissive_factor;
    __m256 normal_scale;
    __m256 occlusion_strenght;

    Texture *base_color_map[8];
    Texture *metallic_roughness_map[8];
    Texture *emissive_map[8];
    Texture *normal_map[8];
    Texture *occlusion_map[8];
} PackedMaterial;

typedef struct MaterialEntry {
    char name[128];
    size_t id;
} MaterialEntry;


#endif