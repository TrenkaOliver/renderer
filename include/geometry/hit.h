#ifndef HIT_H
#define HIT_H

#include "math/vec.h"
#include "math/packed_vec.h"
#include "light/material.h"

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

typedef struct PackedHitResult {
    ps_Vec point;
    ps_Vec ng;
    ps_Vec ns;
    __m256 t;
    Material *material[8];
    __m256 d_u;
    __m256 d_v;
} PackedHitResult;

typedef struct PackedInfo {
    __m256 u;
    __m256 v;
    __m256 w;
} PackedInfo;


#endif