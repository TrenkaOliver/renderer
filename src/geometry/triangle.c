#include <math.h>
#include "geometry/object.h"

#define EPSILON 1e-8

double triangle_ray_intersection(Object *object, Ray *ray) {
    Vec ab, ac, ao;
    double denom, u, v, t;

    ab = v_sub(object->type.triangle.b, object->type.triangle.a);
    ac = v_sub(object->type.triangle.c, object->type.triangle.a);
    ao = v_sub(ray->o, object->type.triangle.a);

    denom = dot(ab, cross(ray->v, ac));
    if (fabs(denom) < EPSILON) return -1.0;

    u = dot(ao, cross(ray->v, ac)) / denom;
    if (u < 0.0) return -1.0;

    v = dot(ray->v, cross(ao, ab)) / denom;
    if (v < 0.0) return -1.0;

    if (u + v > 1.0) return -1.0;

    t = dot(ac, cross(ao, ab)) / denom;
    
    return t;
}

HitResult get_triangle_result(Ray *ray, Object *object, double t) {
    Vec p;

    p = v_add(ray->o, scale(ray->v, t));

    return (HitResult){.point = p, .normal = object->type.triangle.ng, .t = t, .material = object->m};
}