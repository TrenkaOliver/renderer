#ifndef MESH_H
#define MESH_H

#include "geometry/object.h"

typedef struct VertexArray {
    Vec *vertices;
    size_t capacity;
    size_t count;
} VertexArray;

typedef struct Mesh {
    Vec position;
    Vec rotation;
    Vec size;
    size_t first_triangle;
    size_t triangle_count;
} Mesh;

void add_vertex(VertexArray *v_arr, Vec vertex);

#endif