#ifndef LIGHT_H
#define LIGHT_H

#include "geometry/vec.h"

typedef struct Material {
    Vec diffuse;
    Vec specular;
    double shininess;
} Material;

typedef struct DirectionalLight {
    Vec direction;
    Vec color;
    double intensity;
} DirectionalLight;



#endif