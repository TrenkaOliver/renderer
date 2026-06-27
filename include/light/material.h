#ifndef MATERIAL_H
#define MATERIAL_H

#include "math/vec.h"
#include "array/array.h"

typedef struct Material {
    Vec diffuse;
    Vec specular;
    double shininess;
    double reflectivity;
} Material;

typedef struct MaterialEntry {
    char name[128];
    size_t id;
} MaterialEntry;

#endif