#ifndef HIT_H
#define HIT_H

#include "math/vec.h"
#include "math/packed_ray.h"
#include "light/material.h"
#include <immintrin.h>

typedef struct HitResult {
    Vec point;
    Vec ng;
    Vec ns;
    Vec tangent;
    Vec bitangent;
    int valid_tangent;
    double t;
    Material *material;
    double d_u;
    double d_v;
} HitResult;

typedef struct ScalarHitResult {
    float t;
    float u;
    float v;
    uint32_t idx;
}  ScalarHitResult;

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