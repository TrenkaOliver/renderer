#include <math.h>
#include <stdio.h>
#include "geometry/object.h"

#define EPSILON 1e-8

double triangle_ray_intersection(Object *object, Ray *ray, Info *info) {
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
    info->u = u;
    info->v = v;
    info->w = 1 - u - v;
    
    return t;
}

HitResult get_triangle_result(Ray *ray, Object *object, Info *info, double t) {
    Vec p, ns;
    double d_u, d_v;

    p = v_add(ray->o, scale(ray->v, t));
    
    ns = object->type.triangle.const_normal
    ? object->type.triangle.ng 
    : normalize(v_add(v_add(scale(object->type.triangle.na, info->w), scale(object->type.triangle.nb, info->u)), scale(object->type.triangle.nc, info->v)));

    if (object->material->diffuse_map != (size_t)-1) {
        d_u = object->type.triangle.ta.x * info->w + object->type.triangle.tb.x * info->u + object->type.triangle.tc.x * info->v;
        d_v = object->type.triangle.ta.y * info->w + object->type.triangle.tb.y * info->u + object->type.triangle.tc.y * info->v;
    } else {
        d_u = NAN;
        d_v = NAN;
    }
    
    return (HitResult){.point = p, .ng = object->type.triangle.ng, .ns = ns, .t = t, .material = object->material, .d_u = d_u, .d_v = d_v};
}