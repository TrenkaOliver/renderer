#ifndef AABB_H
#define AABB_H

#include "math/vec.h"
#include "math/ray.h"

typedef struct AABB {
    Vec min;
    Vec max;
} AABB;

double aabb_ray_intersection(AABB *aabb, Ray *ray);

#endif