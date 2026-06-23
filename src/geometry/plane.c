#include <math.h>
#include "geometry/plane.h"

#define EPSILON 1e-8

double plane_ray_intersection(Plane *plane, Ray *ray) {
    double denom;

    denom = dot(plane->n, ray->v);
    if (fabs(denom) < EPSILON) return -1.0;
    
    return dot(plane->n, v_sub(plane->o, ray->o)) / denom;
}