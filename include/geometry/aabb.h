#ifndef AABB_H
#define AABB_H

#include "math/vec.h"
#include "math/ray.h"

typedef struct AABB {
    float min[3];
    float max[3];
} AABB;

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

#endif