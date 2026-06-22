#ifndef LIGHT_H
#define LIGHT_H

#include "math/vec.h"

typedef struct DirectionalLight {
    Vec direction;
    Vec color;
    double intensity;
} DirectionalLight;



#endif