#ifndef MESH_H
#define MESH_H

#include "geometry/object.h"

typedef struct VecDynArray {
    Vec *ptr;
    size_t capacity;
    size_t count;
} VecDynArray;

typedef struct Face {
    size_t v;
    size_t vt;
    size_t vn;
} Face;

typedef struct Mesh {
    Vec position;
    Vec rotation;
    Vec size;
    size_t first_triangle;
    size_t triangle_count;
} Mesh;

void add_element(VecDynArray *v_arr, Vec vertex);

#endif