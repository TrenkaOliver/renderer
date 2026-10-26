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

typedef struct PackedAABB {
    ps_Vec min;
    ps_Vec max;
} PackedAABB;

static inline void vecf_min(const float * restrict src1, const float * restrict src2, float * restrict dst) {
    dst[0] = fminf(src1[0], src2[0]);
    dst[1] = fminf(src1[1], src2[1]);
    dst[2] = fminf(src1[2], src2[2]);
}

static inline void vecf_max(const float * restrict src1, const float * restrict src2, float * restrict dst) {
    dst[0] = fmaxf(src1[0], src2[0]);
    dst[1] = fmaxf(src1[1], src2[1]);
    dst[2] = fmaxf(src1[2], src2[2]);
}

static inline void vecf_add(const float * restrict src, float * restrict dst) {
    dst[0] += src[0];
    dst[1] += src[1];
    dst[2] += src[2];
}

static inline AABB aabb_merge(const AABB a, const AABB b) {
    return (AABB){
        .min = {fminf(a.min[0], b.min[0]), fminf(a.min[1], b.min[1]), fminf(a.min[2], b.min[2])},
        .max = {fmaxf(a.max[0], b.max[0]), fmaxf(a.max[1], b.max[1]), fmaxf(a.max[2], b.max[2])}
    };
}

static inline __m256 packed_aabb_ray_intersection(const PackedAABB *aabb, const PackedRay *ray) {
    const __m256 tx0 = _mm256_mul_ps(_mm256_sub_ps(aabb->min.x, ray->o.x), ray->inv_v.x);
    const __m256 tx1 = _mm256_mul_ps(_mm256_sub_ps(aabb->max.x, ray->o.x), ray->inv_v.x);

    __m256 t_min = _mm256_min_ps(tx0, tx1);
    __m256 t_max = _mm256_max_ps(tx0, tx1);

    const __m256 ty0 = _mm256_mul_ps(_mm256_sub_ps(aabb->min.y, ray->o.y), ray->inv_v.y);
    const __m256 ty1 = _mm256_mul_ps(_mm256_sub_ps(aabb->max.y, ray->o.y), ray->inv_v.y);

    t_min = _mm256_max_ps(t_min, _mm256_min_ps(ty0, ty1));
    t_max = _mm256_min_ps(t_max, _mm256_max_ps(ty0, ty1));

    __m256 tz0 = _mm256_mul_ps(_mm256_sub_ps(aabb->min.z, ray->o.z), ray->inv_v.z);
    __m256 tz1 = _mm256_mul_ps(_mm256_sub_ps(aabb->max.z, ray->o.z), ray->inv_v.z);

    t_min = _mm256_max_ps(t_min, _mm256_min_ps(tz0, tz1));
    t_max = _mm256_min_ps(t_max, _mm256_max_ps(tz0, tz1));

    const __m256 zero = _mm256_setzero_ps();
    const __m256 miss = _mm256_set1_ps(-1.0f);

    __m256 miss_mask = _mm256_cmp_ps(t_min, t_max, _CMP_GT_OQ);
    miss_mask = _mm256_or_ps(miss_mask, _mm256_cmp_ps(t_max, zero, _CMP_LT_OQ));

    __m256 result = _mm256_max_ps(t_min, zero);

    return _mm256_blendv_ps(result, miss, miss_mask);
}

#endif