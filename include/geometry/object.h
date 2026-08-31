#ifndef OBJECT_H
#define OBJECT_H

#include "light/material.h"
#include "math/ray.h"
#include "math/packed_ray.h"
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
    Material *material;
    __m256 (*get_ray_intersection)(struct Object *, PackedRay *, PackedInfo *);
    HitResult (*get_hit_result)(Ray *, struct Object *, Info *, double);
} Object;

double sphere_ray_intersection(Object *object, Ray *ray, Info *info);
HitResult get_sphere_result(Ray *ray, Object *object, Info *info, double t);

double triangle_ray_intersection(Object *object, Ray *ray, Info *info);
HitResult get_triangle_result(Ray *ray, Object *object, Info *info, double t);

double box_ray_intersection(Object *object, Ray *ray, Info *info);
HitResult get_box_result(Ray *ray, Object *object, Info *info, double t);

__m256 packed_sphere_ray_intersection(Object *object, PackedRay *ray, PackedInfo *info);
__m256 packed_triangle_ray_intersection(Object *object, PackedRay *ray, PackedInfo *info);
__m256 packed_box_ray_intersection(Object *object, PackedRay *ray, PackedInfo *info);

#endif