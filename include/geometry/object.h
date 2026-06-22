#ifndef OBJECT_H
#define OBJECT_H

#include "light/material.h"
#include "math/ray.h"
#include "geometry/hit.h"
#include "geometry/aabb.h"
#include "geometry/sphere.h"
#include "geometry/triangle.h"
#include "geometry/box.h"

typedef struct Object {
    union {
        Sphere sphere;
        Triangle triangle;
        Box box;
    } type;
    AABB aabb;
    Material *m;
    double (*get_ray_intersection)(struct Object *, Ray *);
    HitResult (*get_hit_result)(Ray *, struct Object *, double);
} Object;

double sphere_ray_intersection(Object *object, Ray *ray);
HitResult get_sphere_result(Ray *ray, Object *object, double t);

double triangle_ray_intersection(Object *object, Ray *ray);
HitResult get_triangle_result(Ray *ray, Object *object, double t);

double box_ray_intersection(Object *object, Ray *ray);
HitResult get_box_result(Ray *ray, Object *object, double t);

#endif