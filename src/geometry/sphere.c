#include <math.h>
#include "geometry/object.h"

double sphere_ray_intersection(Object *object, Ray *ray) {
    double a, b, c, d, t1, t2, t;
    Vec m;

    m = v_sub(ray->o, object->type.sphere.o);

    a = 1.0;
    b = 2.0 * dot(m, ray->v);
    c = dot(m, m) - object->type.sphere.r * object->type.sphere.r;

    d = b * b - 4 * a * c;

    if (d < 0.0) return NAN;

    t1 = (-b + sqrt(d)) / (2.0 * a);
    t2 = (-b - sqrt(d)) / (2.0 * a);

    t = INFINITY;

    if (t1 > 0.0 && t1 < t) t = t1;
    if (t2 > 0.0 && t2 < t) t = t2;

    if (t == INFINITY) return NAN;

    return t;
}

HitResult get_sphere_result(Ray *ray, Object *object, double t) {
    Vec p, n;

    p = v_add(ray->o, scale(ray->v, t));
    n = normalize(v_sub(p, object->type.sphere.o));

    return (HitResult){.point = p, .normal = n, .t = t, .material = object->m};
}

HitResult get_triangle_result(Ray *ray, Object *object, double t) {
    Vec p;

    p = v_add(ray->o, scale(ray->v, t));

    return (HitResult){.point = p, .normal = object->type.triangle.n, .t = t, .material = object->m};
}