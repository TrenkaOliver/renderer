#ifndef TRACE_H
#define TRACE_H

#define EPSILON 1e-8

#include "geometry/object.h"
#include "geometry/hit.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "accel/bvh.h"

HitResult get_first_plane(Ray *ray, Planes *planes);
int is_shaded_by_plane(Ray *ray, Planes *planes);

HitResult get_first_object(Ray *ray, BVH *bvh);
int is_shaded_by_object(Ray *ray, BVH *bvh);

HitResult get_first_hit(Ray *ray, Scene *scene, BVH *bvh);
int is_shaded(Ray *ray, Scene *scene, BVH *bvh);

Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, BVH *bvh, int depth);

#endif