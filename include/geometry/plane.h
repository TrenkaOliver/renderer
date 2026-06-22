#ifndef PLANE_H
#define PLANE_H

#include "math/vec.h"
#include "math/ray.h"
#include "light/material.h"

typedef struct Plane {
    Vec o;
    Vec n;
    Material *m;
} Plane;

double plane_ray_intersection(Plane *plane, Ray *ray);

#endif