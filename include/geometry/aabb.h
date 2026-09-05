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

void vecf_min(float *src, float *dst);
void vecf_max(float *src, float *dst);
void vecf_min3(float *src1, float *src2, float *dst);
void vecf_max3(float *src1, float *src2, float *dst);
void vecf_add(float *src, float *dst);
void vecf_sub(float *src, float *dst);

AABB aabb_merge(AABB a, AABB b);
Vec calc_centroid(AABB aabb);

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

static __m256 packed_aabb_ray_intersection(PackedAABB *aabb, PackedRay *ray) {
    __m256 tx0 = _mm256_mul_ps(_mm256_sub_ps(aabb->min.x, ray->o.x), ray->inv_v.x);
    __m256 tx1 = _mm256_mul_ps(_mm256_sub_ps(aabb->max.x, ray->o.x), ray->inv_v.x);

    __m256 t_min = _mm256_min_ps(tx0, tx1);
    __m256 t_max = _mm256_max_ps(tx0, tx1);

    __m256 ty0 = _mm256_mul_ps(_mm256_sub_ps(aabb->min.y, ray->o.y), ray->inv_v.y);
    __m256 ty1 = _mm256_mul_ps(_mm256_sub_ps(aabb->max.y, ray->o.y), ray->inv_v.y);

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

    //__m256 result = _mm256_blendv_ps(t_min, t_max, _mm256_cmp_ps(t_min, zero, _CMP_LT_OQ));
    __m256 result = _mm256_max_ps(t_min, zero);

    return _mm256_blendv_ps(result, miss, miss_mask);

}

#endif