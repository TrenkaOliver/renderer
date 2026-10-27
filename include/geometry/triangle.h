#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "math/vec.h"
#include "geometry/aabb.h"
#include "geometry/hit.h"
#include "light/material.h"
#include "stdint.h"

typedef struct OriginalTriangle {
    uint32_t *ai;
    uint32_t *bi;
    uint32_t *ci;

    float *nx;
    float *ny;
    float *nz;

    AABB *aabb;
    float *centroid;
    uint32_t *material;
    
} OriginalTriangle;

typedef struct RuntimeTriangle {
    float *arr;
    uint32_t *building_idx;
} RuntimeTriangle;

__m256 packed_triangle_ray_intersection(const uint32_t idx, const uint32_t count, const ps_Vec ray_o, const ps_Vec ray_v, const float * restrict array, PackedInfo * restrict info);

PackedHitResult packed_triangle_result(
const float t_arr[8], const __m256 u, const __m256 v, const uint32_t building_indices[8], const PackedRay ray, const RuntimeTriangle * restrict runtime_triangles,
const OriginalTriangle * restrict building_triangles, float **vertex_data, const DynArray * restrict materials, const DynArray * restrict textures
);


#endif
