#include <stdlib.h>
#include <math.h>
#include "accel/bvh.h"
#include "accel/cmp.h"

BVH create_bvh(Object *first, size_t count) {
    size_t i;
    BVH bvh;
    bvh.ptr_array = calloc(count, sizeof(Object **));
    for (i = 0; i < count; i++) *(bvh.ptr_array + i) = first + i;
    bvh.root = build_tree(bvh.ptr_array, count);
    return bvh;
}

BVHNode *create_node(BVHNode *left, BVHNode *right, Object **first, size_t count) {
    int i;
    AABB aabb;
    BVHNode *node;

    aabb.min = vec(INFINITY, INFINITY, INFINITY);
    aabb.max = vec(-INFINITY, -INFINITY, -INFINITY);
    
    if (first) 
        for (i = 0; i < count; i++) aabb = aabb_merge(aabb, (*(first + i))->aabb);
    else
        aabb = aabb_merge(left->aabb, right->aabb);

    node = malloc(sizeof(BVHNode));

    node->left = left;
    node->right = right;
    node->aabb = aabb;
    node->first_primitive = first;
    node->primitive_count = count;

    return node;
}

BVHNode *build_tree(Object **arr, size_t count) {
    int (*cmp)(const void *, const void *);
    double dx, dy, dz;
    size_t i, mid;
    Vec min, max, center;

    if (count <= 4) return create_node(NULL, NULL, arr, count);

    min = vec(INFINITY, INFINITY, INFINITY);
    max = vec(-INFINITY, -INFINITY, -INFINITY);

    for (i = 0; i < count; i++) {
        center = calc_centroid((*(arr + i))->aabb);
        min = v_min(min, center);
        max = v_max(max, center);
    }

    dx = max.x - min.x;
    dy = max.y - min.y;
    dz = max.z - min.z;

    if (dx >= dy && dx >= dz)
        cmp = x_compare;
    else if (dy >= dz)
        cmp = y_compare;
    else
        cmp = z_compare;

    qsort(arr, count, sizeof(Object *), cmp);

    mid = count / 2;

    return create_node(
        build_tree(arr, mid),
        build_tree(arr + mid, count - mid),
        NULL,
        0
    );
}