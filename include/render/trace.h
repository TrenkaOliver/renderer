#ifndef TRACE_H
#define TRACE_H

#define EPSILON 1e-3

#include "geometry/object.h"
#include "geometry/hit.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "accel/bvh.h"

typedef struct TraceResult {
    ps_Vec c;
    ps_Vec ref_mul;
    __m256 valid_ref;
} TraceResult;

HitResult get_first_plane(Ray *ray, DynArray *planes);
int is_shaded_by_plane(Ray *ray, DynArray *planes);

HitResult get_first_object(Ray *ray, BVH8Tree *bvh, DynArray *materials);
PackedHitResult packed_get_first_object(Ray ray[8], BVH8Tree *bvh, DynArray *materials, DynArray *textures);
int is_shaded_by_object(Ray *ray, BVH8Tree *bvh);

HitResult get_first_hit(Ray *ray, Scene *scene, BVH8Tree *bvh);
int is_shaded(Ray *ray, Scene *scene, BVH8Tree *bvh);

Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, BVH8Tree *bvh, int depth);
TraceResult packed_trace_ray(Ray ray[8], Scene *scene, Camera *cam, BVH8Tree *bvh, __m256i depth);

#endif