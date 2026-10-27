#ifndef BVH_H
#define BVH_H

#include <stdint.h>

#include "geometry/aabb.h"
#include "geometry/triangle.h"

#define C_TRAVERSAL 1.0f
#define C_INTERSECT 1.0f
#define BIN_COUNT 32
#define LEAF_SIZE 8

extern uint32_t next;

typedef struct BVHNode {
    AABB aabb;
    uint32_t first_primitive_or_right_child;
    uint32_t primitive_count;
} BVHNode;

typedef struct BVH {
    BVHNode *nodes;
    OriginalTriangle triangles;
} BVH;

typedef struct BVH8Node {
    PackedAABB bounds;
    uint32_t idx[8];
    uint32_t triangle_idx[8];
    uint8_t primitive_count[8];
    uint8_t internal_count;
    uint8_t leaf_count;
} BVH8Node;

typedef struct BVH8Tree {
    BVH8Node *nodes;
    float **vertex_data;
    OriginalTriangle building_triangles;
    RuntimeTriangle runtime_triangles;
} BVH8Tree;

BVH create_bvh(OriginalTriangle *triangles, size_t count);
BVH8Tree create_bvh8_tree(OriginalTriangle *triangles, float **vertex_data, size_t count);

#endif