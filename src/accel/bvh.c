#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "accel/bvh.h"
#include "accel/cmp.h"

#define LEAF_SIZE 4
#define C_TRAVEL 1

BVH create_bvh(Object *first, size_t count) {
    size_t i;
    BVH bvh;
    bvh.ptr_array = calloc(count, sizeof(Object *));
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

double get_surface_area(Object **arr, size_t count) {
    AABB aabb;
    size_t i;

    aabb = (AABB){
        .min = vec(INFINITY, INFINITY, INFINITY),
        .max = vec(-INFINITY, -INFINITY, -INFINITY)
    };

    for (i = 0; i < count; i++) {
        aabb = aabb_merge(aabb, arr[i]->aabb);
    }

    return SA(aabb);
}

double SA(AABB aabb) {
    Vec d;

    d = v_sub(aabb.max, aabb.min);

    return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
}

void build_boxes(Object **arr, size_t count, AABB *left_box, AABB *right_box) {
    size_t i;

    left_box[0] = arr[0]->aabb;
    for (i = 1; i < count - 1; i++) {
        left_box[i] = aabb_merge(left_box[i - 1], arr[i]->aabb);
    }

    right_box[count - 1] = arr[count - 1]->aabb;
    for (i = count - 2; i != (size_t)-1; i--) {
        right_box[i] = aabb_merge(right_box[i + 1], arr[i]->aabb);
    }
}

BVHNode *build_tree(Object **arr, size_t count) {
    int (*cmp)(const void *, const void *);
    size_t i, c_left, c_right, split;
    AABB *left_box, *right_box;
    double cost, min_cost;

    left_box = calloc(count, sizeof(AABB));
    right_box = calloc(count, sizeof(AABB));

    double sa_parent = get_surface_area(arr, count);

    if (count <= 4) return create_node(NULL, NULL, arr, count);

    min_cost = INFINITY;
    split = 0;

    qsort(arr, count, sizeof(Object *), x_compare);
    build_boxes(arr, count, left_box, right_box);

    for (i = 0; i < count - 1; i++) {
        c_left = i + 1;
        c_right = count - c_left;

        cost = 
        1
        + (SA(left_box[i]) / sa_parent) * c_left
        + (SA(right_box[i + 1]) / sa_parent) * c_right;

        
        if (cost < min_cost) {
            min_cost = cost;
            cmp = x_compare;
            split = c_left;
        }
    }

    qsort(arr, count, sizeof(Object *), y_compare);
    build_boxes(arr, count - 1, left_box, right_box);

    for (i = 0; i < count - 1; i++) {
        c_left = i + 1;
        c_right = count - c_left;

        cost = 
        1
        + (SA(left_box[i]) / sa_parent) * c_left
        + (SA(right_box[i + 1]) / sa_parent) * c_right;

        if (cost < min_cost) {
            min_cost = cost;
            cmp = y_compare;
            split = c_left;
        }
    }

    qsort(arr, count, sizeof(Object *), z_compare);
    build_boxes(arr, count - 1, left_box, right_box);

    for (i = 0; i < count - 1; i++) {
        c_left = i + 1;
        c_right = count - c_left;

        cost = 
        1
        + (SA(left_box[i]) / sa_parent) * c_left
        + (SA(right_box[i + 1]) / sa_parent) * c_right;

        if (cost < min_cost) {
            // printf("c_left: %d, split: %d", c_left, split);
            // printf("min_cost: %f, cost: %f\n", min_cost, cost);
            // printf("p: %f, left: %f, right: %fz\n\n", sa_parent, SA(left_box[i]), SA(right_box[i + 1]));
            min_cost = cost;
            cmp = z_compare;
            split = c_left;
        }
    }

    free(left_box);
    free(right_box);

    qsort(arr, count, sizeof(Object *), cmp);
    
    if (split == count) return create_node(NULL, NULL, arr, count);

    return create_node(
        build_tree(arr, split),
        build_tree(arr + split, count - split),
        NULL,
        0
    );

}

// BVHNode *build_tree(Object **arr, size_t count) {
//     int (*cmp)(const void *, const void *);
//     double dx, dy, dz;
//     size_t i, mid;
//     Vec min, max, center;

//     if (count <= 4) return create_node(NULL, NULL, arr, count);

//     min = vec(INFINITY, INFINITY, INFINITY);
//     max = vec(-INFINITY, -INFINITY, -INFINITY);

//     for (i = 0; i < count; i++) {
//         center = calc_centroid((*(arr + i))->aabb);
//         min = v_min(min, center);
//         max = v_max(max, center);
//     }

//     dx = max.x - min.x;
//     dy = max.y - min.y;
//     dz = max.z - min.z;

//     if (dx >= dy && dx >= dz)
//         cmp = x_compare;
//     else if (dy >= dz)
//         cmp = y_compare;
//     else
//         cmp = z_compare;

//     qsort(arr, count, sizeof(Object *), cmp);

//     mid = count / 2;

//     return create_node(
//         build_tree(arr, mid),
//         build_tree(arr + mid, count - mid),
//         NULL,
//         0
//     );
// }