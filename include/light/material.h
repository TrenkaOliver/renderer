#ifndef MATERIAL_H
#define MATERIAL_H

#include "math/vec.h"
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

typedef struct MaterialEntry {
    char name[128];
    size_t id;
} MaterialEntry;


#endif