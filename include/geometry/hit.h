#ifndef HIT_H
#define HIT_H

#include "math/vec.h"
#include "math/packed_ray.h"
#include "light/material.h"
#include <immintrin.h>

typedef struct PackedHitResult {
    PackedRay ray;
    ps_Vec point;
    ps_Vec ng;
    ps_Vec ns;
    ps_Vec tangent;
    ps_Vec bitangent;
    __m256 valid_tangent;
    __m256 valid_hit;
    __m256 t;
    PackedMaterial material;
    __m256 d_u;
    __m256 d_v;    
} PackedHitResult;

typedef struct PackedInfo {
    __m256 u;
    __m256 v;
} PackedInfo;


#endif