#ifndef AABB_H
#define AABB_H

#include "math/vec.h"
#include "math/ray.h"
#include "math/packed_vec.h"
#include "math/packed_ray.h"

typedef struct AABB {
    float min[3];
    float max[3];
} AABB;

static inline void vecf_min(float *src, float *dst) {
    dst[0] = fminf(dst[0], src[0]);
    dst[1] = fminf(dst[1], src[1]);
    dst[2] = fminf(dst[2], src[2]);
}

static inline void vecf_max(float *src, float *dst) {
    dst[0] = fmaxf(dst[0], src[0]);
    dst[1] = fmaxf(dst[1], src[1]);
    dst[2] = fmaxf(dst[2], src[2]);
}

static inline void vecf_min3(float *src1, float *src2, float *dst) {
    dst[0] = fminf(src1[0], src2[0]);
    dst[1] = fminf(src1[1], src2[1]);
    dst[2] = fminf(src1[2], src2[2]);
}

static inline void vecf_max3(float *src1, float *src2, float *dst) {
    dst[0] = fmaxf(src1[0], src2[0]);
    dst[1] = fmaxf(src1[1], src2[1]);
    dst[2] = fmaxf(src1[2], src2[2]);
}

static inline void vecf_add(float *src, float *dst) {
    dst[0] += src[0];
    dst[1] += src[1];
    dst[2] += src[2];
}

static inline void vecf_sub(float *src, float *dst) {
    dst[0] -= src[0];
    dst[1] -= src[1];
    dst[2] -= src[2];
}

static inline AABB aabb_merge(AABB a, AABB b) {
    return (AABB){
        .min = {fminf(a.min[0], b.min[0]), fminf(a.min[1], b.min[1]), fminf(a.min[2], b.min[2])},
        .max = {fmaxf(a.max[0], b.max[0]), fmaxf(a.max[1], b.max[1]), fmaxf(a.max[2], b.max[2])}
    };
}

static inline Vec calc_centroid(AABB aabb) {
    return (Vec){
        .x = (aabb.min[0] + aabb.max[0]) / 2.0,
        .y = (aabb.min[1] + aabb.max[1]) / 2.0,
        .z = (aabb.min[2] + aabb.max[2]) / 2.0
    };
}

static inline float aabb_ray_intersection(AABB *aabb, Ray *ray) {
    float tx0, tx1, ty0, ty1, tz0, tz1, t_min, t_max;

    tx0 = (aabb->min[0] - ray->o.x) * ray->inv_v.x;
    tx1 = (aabb->max[0] - ray->o.x) * ray->inv_v.x;

    t_min = fminf(tx0, tx1);
    t_max = fmaxf(tx0, tx1);

    ty0 = (aabb->min[1] - ray->o.y) * ray->inv_v.y;
    ty1 = (aabb->max[1] - ray->o.y) * ray->inv_v.y;

    t_min = fmaxf(t_min, fminf(ty0, ty1));
    t_max = fminf(t_max, fmaxf(ty0, ty1));

    if (t_min > t_max) return -1.0;
    
    tz0 = (aabb->min[2] - ray->o.z) * ray->inv_v.z;
    tz1 = (aabb->max[2] - ray->o.z) * ray->inv_v.z;

    t_min = fmaxf(t_min, fminf(tz0, tz1));
    t_max = fminf(t_max, fmaxf(tz0, tz1));

    if (t_min > t_max || t_max < 0.0) return -1.0;

    return t_min >= 0.0 ? t_min : t_max;
}

static inline __m256 packed_aabb_ray_intersection(AABB *aabb, PackedRay *ray) {
    ps_Vec min = (ps_Vec) {
        .x = _mm256_set1_ps(aabb->min[0]),
        .y = _mm256_set1_ps(aabb->min[1]),
        .z = _mm256_set1_ps(aabb->min[2]),
    };
    ps_Vec max = (ps_Vec) {
        .x = _mm256_set1_ps(aabb->max[0]),
        .y = _mm256_set1_ps(aabb->max[1]),
        .z = _mm256_set1_ps(aabb->max[2]),
    };

    __m256 tx0 = _mm256_mul_ps(_mm256_sub_ps(min.x, ray->o.x), ray->inv_v.x);
    __m256 tx1 = _mm256_mul_ps(_mm256_sub_ps(max.x, ray->o.x), ray->inv_v.x);

    __m256 t_min = _mm256_min_ps(tx0, tx1);
    __m256 t_max = _mm256_max_ps(tx0, tx1);

    __m256 ty0 = _mm256_mul_ps(_mm256_sub_ps(min.y, ray->o.y), ray->inv_v.y);
    __m256 ty1 = _mm256_mul_ps(_mm256_sub_ps(max.y, ray->o.y), ray->inv_v.y);

    t_min = _mm256_max_ps(t_min, _mm256_min_ps(ty0, ty1));
    t_max = _mm256_min_ps(t_max, _mm256_max_ps(ty0, ty1));

    __m256 tz0 = _mm256_mul_ps(_mm256_sub_ps(min.z, ray->o.z), ray->inv_v.z);
    __m256 tz1 = _mm256_mul_ps(_mm256_sub_ps(max.z, ray->o.z), ray->inv_v.z);

    t_min = _mm256_max_ps(t_min, _mm256_min_ps(tz0, tz1));
    t_max = _mm256_min_ps(t_max, _mm256_max_ps(tz0, tz1));

    const __m256 zero = _mm256_setzero_ps();
    const __m256 miss = _mm256_set1_ps(-1.0f);

    __m256 miss_mask = _mm256_cmp_ps(t_min, t_max, _CMP_GT_OQ);
    miss_mask = _mm256_or_ps(miss_mask, _mm256_cmp_ps(t_max, zero, _CMP_LT_OQ));

    __m256 result = _mm256_blendv_ps(t_min, t_max, _mm256_cmp_ps(t_min, zero, _CMP_LT_OQ));

    return _mm256_blendv_ps(result, miss, miss_mask);

}

#endif