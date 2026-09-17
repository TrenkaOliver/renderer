#ifndef OBJECT_H
#define OBJECT_H

#include <immintrin.h>
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
    float centroid[3];
    Material *material;
    double (*get_ray_intersection)(struct Object *, Ray *, Info *);
    HitResult (*get_hit_result)(Ray *, struct Object *, Info *, double);
} Object;

double sphere_ray_intersection(Object *object, Ray *ray, Info *info);
HitResult get_sphere_result(Ray *ray, Object *object, Info *info, double t);

double triangle_ray_intersection(Object *object, Ray *ray, Info *info);
__m256 packed_triangle_ray_intersection(uint32_t idx, uint8_t count, PackedRay *ray, RuntimeTriangle *array, PackedInfo *info);
HitResult triangle_result(float t, float u, float v, uint32_t idx, Ray *ray, RuntimeTriangle *runtime_triangles, BuildingTriangle *building_triangles, Vertex *vertices);
HitResult get_triangle_result(Ray *ray, Object *object, Info *info, double t);

double box_ray_intersection(Object *object, Ray *ray, Info *info);
HitResult get_box_result(Ray *ray, Object *object, Info *info, double t);

#endif