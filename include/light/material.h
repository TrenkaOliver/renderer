#ifndef MATERIAL_H
#define MATERIAL_H

#include "math/vec.h"
#include "array/array.h"
#include "stdint.h"

typedef struct Material {
    // Vec diffuse;
    // Vec specular;

    // double shininess;
    // double reflectivity;

    // size_t diffuse_map;
    // size_t splecular_map;
    // size_t normal_map;

    Vec base_color_factor;
    float metallic;
    float roughness;
    float normal_scale;

    uint32_t base_color_map;
    uint32_t metallic_roughness_map;
    uint32_t normal_map;
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