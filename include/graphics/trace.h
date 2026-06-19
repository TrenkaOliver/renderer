#ifndef TRACE_H
#define TRACE_H

#define EPSILON 1e-8

#include "geometry/vec.h"
#include "geometry/object.h"
#include "graphics/light.h"
#include "scene.h"
#include "camera.h"

typedef struct HitResult {
    Vec point;
    Vec normal;
    double t;
    Material *material;
} HitResult;

// returns smallest t in front of camera
double sphere_ray_intersection(Sphere *sphere, Line *ray);
double plane_ray_intersection(Plane *plane, Line *ray);
double triangle_ray_intersection(Triangle *triangle, Line *ray);

HitResult get_first_sphere(Line *ray, Spheres *spheres);
HitResult get_first_plane(Line *ray, Planes *planes);
HitResult get_first_triangle(Line *ray, Triangles *triangles);

HitResult get_first_hit(Line *ray, Scene *scene);

Vec trace_ray(Line *ray, Scene *scene, Camera *cam);
#endif