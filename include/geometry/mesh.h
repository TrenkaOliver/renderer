#ifndef MESH_H
#define MESH_H

#include "geometry/object.h"

typedef struct Face {
    size_t v;
    size_t vt;
    size_t vn;
} Face;

typedef struct Mesh {
    Vec position;
    Vec rotation;
    Vec size;
    AABB aabb;
    size_t first_triangle;
    size_t triangle_count;
} Mesh;

#endif