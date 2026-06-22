#include <math.h>
#include "geometry/aabb.h"

double aabb_ray_intersection(AABB *aabb, Ray *ray) {
    Vec inv_v, t0, t1, t_min, t_max;
    double t_enter, t_exit;

    inv_v = vec(1.0 / ray->v.x, 1.0 / ray->v.y, 1.0 / ray->v.z);

    t0 = hadamard(v_sub(aabb->min, ray->o), inv_v);
    t1 = hadamard(v_sub(aabb->max, ray->o), inv_v);

    t_min = vec(
        fmin(t0.x, t1.x),
        fmin(t0.y, t1.y),
        fmin(t0.z, t1.z)
    );

    t_max = vec(
        fmax(t0.x, t1.x),
        fmax(t0.y, t1.y),
        fmax(t0.z, t1.z)
    );

    t_enter = fmax(t_min.x, fmax(t_min.y, t_min.z));
    t_exit = fmin(t_max.x, fmin(t_max.y, t_max.z));

    if (t_exit < t_enter || t_exit < 0.0)
        return NAN;
    else
        return t_enter >= 0.0 ? t_enter : t_exit;
}