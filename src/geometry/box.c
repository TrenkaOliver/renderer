#include <math.h>
#include "geometry/object.h"

#define EPSILON 1e-8

double box_ray_intersection(Object *object, Ray *ray) {
    int i;
    double t_min, t_max, t1, t2, e, f, h, temp;
    Vec p;

    p = v_sub(object->type.box.center, ray->o);

    t_min = -INFINITY;
    t_max = INFINITY;

    for (i = 0; i < 3; i++) {
        e = dot(object->type.box.axes[i], p);
        f = dot(object->type.box.axes[i], ray->v);
        h = i == 0 ? object->type.box.half_size.x : i == 1 ? object->type.box.half_size.y : object->type.box.half_size.z;

        if (fabs(f) < EPSILON) {
            if (fabs(e) > h)
                return -1.0;
            else
                continue;
        }

        t1 = (e - h) / f;
        t2 = (e + h) / f;

        if (t1 > t2) {
            temp = t1;
            t1 = t2;
            t2 = temp;
        }

        t_min = fmax(t1, t_min);
        t_max = fmin(t2, t_max);

        if (t_min > t_max) return -1.0;
    }

    if (t_max < 0.0) return -1.0;
    
    return t_min >= 0.0 ? t_min : t_max;
}

HitResult get_box_result(Ray *ray, Object *object, double t) {
    Vec p, dist, n;
    double x, y, z, ax, ay, az;

    p = v_add(ray->o, scale(ray->v, t));
    dist = v_sub(p, object->type.box.center);

    x = dot(object->type.box.axes[0], dist);
    y = dot(object->type.box.axes[1], dist);
    z = dot(object->type.box.axes[2], dist);

    ax = fabs(x) / object->type.box.half_size.x;
    ay = fabs(y) / object->type.box.half_size.y;
    az = fabs(z) / object->type.box.half_size.z;

    if (ax >= ay && ax >= az)
        n = (x > 0.0) ? object->type.box.axes[0] : neg(object->type.box.axes[0]);
    else if (ay >= az)
        n = (y > 0.0) ? object->type.box.axes[1] : neg(object->type.box.axes[1]);
    else
        n = (z > 0.0) ? object->type.box.axes[2] : neg(object->type.box.axes[2]);

    return (HitResult){.point = p, .normal = n, .t = t, .material = object->m};
}