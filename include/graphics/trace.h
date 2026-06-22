#ifndef TRACE_H
#define TRACE_H

#define EPSILON 1e-8

#include "geometry/vec.h"
#include "geometry/object.h"
#include "geometry/aabb.h"
#include "graphics/light.h"
#include "hitresult.h"
#include "scene.h"
#include "camera.h"



// returns smallest t in front of camera
double plane_ray_intersection(Plane *plane, Line *ray);
double aabb_ray_intersection(AABB *aabb, Line *ray);

double sphere_ray_intersection(Object *object, Line *ray);
double triangle_ray_intersection(Object *object, Line *ray);
double box_ray_intersection(Object *object, Line *ray);

HitResult get_first_plane(Line *ray, Planes *planes);

HitResult get_sphere_result(Line *ray, Object *object, double t);
HitResult get_triangle_result(Line *ray, Object *object, double t);
HitResult get_box_result(Line *ray, Object *object, double t);
HitResult get_first_object(Line *ray, Objects *objects);

// HitResult get_first_sphere(Line *ray, Spheres *spheres);
// HitResult get_first_triangle(Line *ray, Triangles *triangles);
// HitResult get_first_box(Line *ray, Boxes *boxes);

HitResult get_first_hit(Line *ray, Scene *scene);

Vec trace_ray(Line *ray, Scene *scene, Camera *cam, int depth);
#endif