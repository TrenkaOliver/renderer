#ifndef TRACE_H
#define TRACE_H

#define EPSILON 1e-3

#include "geometry/hit.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "accel/bvh.h"

typedef struct TraceResult {
    ps_Vec c;
    ps_Vec ref_mul;
    __m256 valid_ref;
} TraceResult;

PackedHitResult packed_get_first_object(const Ray * restrict ray, const BVH8Tree * restrict bvh, const DynArray * restrict materials, const DynArray *restrict textures);
//int is_shaded_by_object(Ray * restrict ray, BVH8Tree * restrict bvh);

TraceResult packed_trace_ray(Ray * restrict ray, const Scene * restrict scene, const Camera * restrict cam, const BVH8Tree * restrict bvh, const __m256i depth);

#endif