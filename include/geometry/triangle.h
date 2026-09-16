#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "math/vec.h"
#include "geometry/aabb.h"
#include "light/material.h"
#include "stdint.h"

typedef struct Triangle {
    int const_normal;
    int has_texture;
    Vec a;
    Vec b;
    Vec c;
    Vec na;
    Vec nb;
    Vec nc;
    Vec ng;
    Vec ta;
    Vec tb;
    Vec tc;
} Triangle;

typedef struct Vertex {
    float *x;
    float *y;
    float *z;
    
    float *nx;
    float *ny;
    float *nz;

    float *u;
    float *v;
} Vertex;

typedef struct BuildingTriangle {
    uint32_t *ai;
    uint32_t *bi;
    uint32_t *ci;

    uint32_t *nai;
    uint32_t *nbi;
    uint32_t *nci;

    uint32_t *tai;
    uint32_t *tbi;
    uint32_t *tci;

    float *nx;
    float *ny;
    float *nz;

    AABB *aabb;
    float *centroid;
    Material **material;
    
} BuildingTriangle;

typedef struct RuntimeTriangle {
    float *arr;
    uint32_t offset;

    uint32_t *building_idx;
    Material **mat_ptr_arr;
} RuntimeTriangle;

#endif
