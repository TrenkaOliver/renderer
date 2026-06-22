#ifndef TRACE_H
#define TRACE_H

#define EPSILON 1e-8

#include "geometry/object.h"
#include "geometry/hit.h"
#include "scene/scene.h"
#include "camera/camera.h"

HitResult get_first_plane(Ray *ray, Planes *planes);
HitResult get_first_object(Ray *ray, Objects *objects);
HitResult get_first_hit(Ray *ray, Scene *scene);

Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, int depth);

#endif