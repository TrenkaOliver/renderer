#ifndef MESH_H
#define MESH_H

#include "geometry/object.h"
#include "light/material.h"

typedef struct Face {
    uint32_t v;
    uint32_t vt;
    uint32_t vn;
} Face;

typedef struct Mesh {
    Vec position;
    Vec rotation;
    Vec size;
    AABB aabb;
    uint32_t first_triangle;
    uint32_t triangle_count;
    uint32_t first_vertex_pos;
    uint32_t vertex_pos_count;
    uint32_t first_vertex_normal;
    uint32_t vertex_normal_count;
    uint32_t first_vertex_texcoord;
    uint32_t vertex_texcoord_count;
} Mesh;

#endif