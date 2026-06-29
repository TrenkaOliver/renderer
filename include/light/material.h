#ifndef MATERIAL_H
#define MATERIAL_H

#include "math/vec.h"
#include "array/array.h"

typedef struct Material {
    Vec diffuse;
    Vec specular;

    double shininess;
    double reflectivity;

    size_t diffuse_map;
    size_t splecular_map;
    size_t normal_map;
} Material;

typedef struct MaterialEntry {
    char name[128];
    size_t id;
} MaterialEntry;


#endif