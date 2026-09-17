#ifndef HIT_H
#define HIT_H

#include "math/vec.h"
#include "light/material.h"
#include <immintrin.h>

typedef struct HitResult {
    Vec point;
    Vec ng;
    Vec ns;
    double t;
    Material *material;
    double d_u;
    double d_v;
} HitResult;

typedef struct Info {
    double u;
    double v;
    double w;
} Info;

typedef struct PackedInfo {
    __m256 u;
    __m256 v;
} PackedInfo;


#endif