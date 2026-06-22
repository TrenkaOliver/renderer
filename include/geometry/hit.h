#ifndef HIT_H
#define HIT_H

#include "math/vec.h"
#include "light/material.h"

typedef struct HitResult {
    Vec point;
    Vec normal;
    double t;
    Material *material;
} HitResult;

#endif