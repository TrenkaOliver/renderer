#ifndef OBJECT_H
#define OBJECT_H

#include "vec.h"
#include "graphics/light.h"
#include "geometry/aabb.h"
#include "graphics/hitresult.h"

typedef struct Line {
    Vec o;
    Vec v;
} Line;

typedef struct Sphere {
    Vec o;
    double r;
} Sphere;

typedef struct Plane {
    Vec o;
    Vec n;
    Material *m;
} Plane;

typedef struct Triangle {
    Vec a;
    Vec b;
    Vec c;
    Vec n;
} Triangle;

typedef struct Box {
    Vec center;
    Vec axes[3];
    Vec half_size;
} Box;

typedef struct Object {
    union {
        Sphere sphere;
        Triangle triangle;
        Box box;
    } type;
    AABB aabb;
    Material *m;
    double (*get_ray_intersection)(struct Object *, Line *);
    HitResult (*get_hit_result)(Line *, struct Object *, double);
} Object;


Line create_line(Vec o, Vec v);
Line line_from_origo(double x, double y, double z);
void move_triangle(Triangle *triangle, Vec v);

#endif