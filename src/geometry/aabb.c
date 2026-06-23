#include <math.h>
#include "geometry/aabb.h"

AABB aabb_merge(AABB a, AABB b) {
    return (AABB){
        .min = v_min(a.min, b.min),
        .max = v_max(a.max, b.max),
    };
}

Vec calc_centroid(AABB aabb) {
    return scale(v_add(aabb.min, aabb.max), 0.5);
}

double aabb_ray_intersection(AABB *aabb, Ray *ray) {
    double tx0, tx1, ty0, ty1, tz0, tz1, t_min, t_max;

    tx0 = (aabb->min.x - ray->o.x) * ray->inv_v.x;
    tx1 = (aabb->max.x - ray->o.x) * ray->inv_v.x;

    t_min = fmin(tx0, tx1);
    t_max = fmax(tx0, tx1);

    ty0 = (aabb->min.y - ray->o.y) * ray->inv_v.y;
    ty1 = (aabb->max.y - ray->o.y) * ray->inv_v.y;

    t_min = fmax(t_min, fmin(ty0, ty1));
    t_max = fmin(t_max, fmax(ty0, ty1));

    if (t_min > t_max) return NAN;
    
    tz0 = (aabb->min.z - ray->o.z) * ray->inv_v.z;
    tz1 = (aabb->max.z - ray->o.z) * ray->inv_v.z;

    t_min = fmax(t_min, fmin(tz0, tz1));
    t_max = fmin(t_max, fmax(tz0, tz1));

    if (t_min > t_max || t_max < 0.0) return NAN;

    return t_min >= 0.0 ? t_min : t_max;
}