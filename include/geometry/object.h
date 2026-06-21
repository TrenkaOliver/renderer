#ifndef OBJECT_H
#define OBJECT_H

#include "vec.h"
#include "graphics/light.h"
#include "geometry/aabb.h"

typedef struct Line {
    Vec o;
    Vec v;
} Line;

typedef struct Sphere {
    Vec o;
    double r;
    AABB aabb;
    Material *m;
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
    AABB aabb;
    Material *m;
} Triangle;

typedef struct Box {
    Vec center;
    Vec axes[3];
    Vec half_size;
    AABB aabb;
    Material *m;
} Box;

Line create_line(Vec o, Vec v);
Line line_from_origo(double x, double y, double z);
void move_triangle(Triangle *triangle, Vec v);

#endif