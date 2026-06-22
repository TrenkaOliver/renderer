#ifndef BVH_H
#define BVH_H

#include "geometry/aabb.h"
#include "geometry/object.h"

typedef struct BVHNode {
    struct BVHNode *left;
    struct BVHNode *right;
    AABB aabb;
    Object **first_primitive;
    size_t primitive_count;
} BVHNode;

typedef struct BVH {
    Object **ptr_array;
    BVHNode *root;
} BVH;

BVH create_bvh(Object *first, size_t count);
BVHNode *create_node(BVHNode *left, BVHNode *right, Object **first, size_t count);
BVHNode *build_tree(Object **arr, size_t count);

#endif