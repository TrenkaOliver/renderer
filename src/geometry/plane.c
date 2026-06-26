#include <math.h>

#include "geometry/plane.h"
#include "render/trace.h"

#define EPSILON 1e-8

double plane_ray_intersection(Plane *plane, Ray *ray) {
    double denom;

    denom = dot(plane->n, ray->v);
    if (fabs(denom) < EPSILON) return -1.0;
    
    return dot(plane->n, v_sub(plane->o, ray->o)) / denom;
}

int is_shaded_by_plane(Ray *ray, Planes *planes) {
    size_t i;

    for (i = 0; i < planes->count; i++)
        if (plane_ray_intersection(planes->ptr + i, ray) > EPSILON) return 1;

    return 0;
}

HitResult get_first_plane(Ray *ray, Planes *planes) {
    double t, tc;
    size_t i;
    Plane *plane;

    t = INFINITY;
    plane = NULL;
    
    for (i = 0; i < planes->count; i++) {
        tc = plane_ray_intersection(planes->ptr + i, ray);
        if (tc >= 0.0 && tc < t) {
            plane = planes->ptr + i;
            t = tc;
        }
    }

    if (!plane) return (HitResult){.point = vec(0.0, 0.0, 0.0), .ng = vec(0.0, 0.0, 0.0), .t = -1.0};

    Vec p = v_add(ray->o, scale(ray->v, t));

    return (HitResult){.point = p, .ng = plane->n, .ns = plane->n, .t = t, .material = plane->m};
}