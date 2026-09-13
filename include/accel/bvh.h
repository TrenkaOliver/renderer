#ifndef BVH_H
#define BVH_H

#include <stdint.h>

#include "geometry/aabb.h"
//#include "geometry/object.h"
#include "geometry/triangle.h"

typedef struct BVHNode {
    AABB aabb;
    uint32_t first_primitive_or_right_child;
    uint32_t primitive_count;
} BVHNode;

typedef struct BVH {
    BVHNode *nodes;
    BuildingTriangle triangles;
} BVH;

typedef struct BVH8Node {
    PackedAABB bounds;
    uint32_t idx[8];
    uint8_t primitive_count[8];
    uint8_t internal_count;
    uint8_t leaf_count;
} BVH8Node;

typedef struct BVH8Tree {
    BVH8Node *nodes;
    BuildingTriangle building_triangles;
    RuntimeTriangle runtime_triangles;
} BVH8Tree;

BVH create_bvh(BuildingTriangle *triangles, size_t count);
BVH8Tree create_bvh8_tree(BuildingTriangle *triangles, Vertex *vertices, size_t count);

#endif