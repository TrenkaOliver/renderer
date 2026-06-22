#ifndef MATERIAL_H
#define MATERIAL_H

#include "math/vec.h"

typedef struct Material {
    Vec diffuse;
    Vec specular;
    double shininess;
    double reflectivity;
} Material;

#endif