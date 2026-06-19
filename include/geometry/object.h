#ifndef OBJECT_H
#define OBJECT_H

#include "vec.h"
#include "graphics/light.h"

typedef struct Line {
    Vec o;
    Vec v;
} Line;

typedef struct Sphere {
    Vec o;
    double r;
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
    Material *m;
} Triangle;

Line line(Vec o, Vec v);
Line line_from_origo(double x, double y, double z);
void move_triangle(Triangle *triangle, Vec v);

#endif