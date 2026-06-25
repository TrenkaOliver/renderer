#ifndef LIGHT_H
#define LIGHT_H

#include "math/vec.h"

typedef struct DirectionalLight {
    Vec dir;
    Vec neg_dir;
    Vec scaled_color;
    Vec color;
    double intensity;
} DirectionalLight;



#endif