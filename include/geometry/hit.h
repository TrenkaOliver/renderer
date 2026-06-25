#ifndef HIT_H
#define HIT_H

#include "math/vec.h"
#include "light/material.h"

typedef struct HitResult {
    Vec point;
    Vec ng;
    Vec ns;
    double t;
    Material *material;
} HitResult;

typedef struct Info {
    double u;
    double v;
    double w;
} Info;


#endif