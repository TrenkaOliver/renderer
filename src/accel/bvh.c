#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
#include "accel/bvh.h"
#include "accel/cmp.h"

#define C_TRAVEL 1

static AABB *left_box;
static AABB *right_box;
static Object **ptr_array;
static BVHNode *nodes;

float get_surface_area(uint32_t start, uint32_t end);
float SA(AABB aabb);
void build_boxes(uint32_t start, uint32_t end, AABB *left_box, AABB *right_box);
uint32_t create_node(uint32_t first_or_right, uint32_t count, uint32_t idx);
uint32_t build_tree(uint32_t start, uint32_t end, uint32_t idx);

BVH create_bvh(Object *first, size_t count) {
    size_t i;
    BVH bvh;

    bvh.nodes = nodes = calloc(count * 2 - 1, sizeof(BVHNode));
    bvh.objects = ptr_array = calloc(count, sizeof(Object *));
    left_box = calloc(count, sizeof(AABB));
    right_box = calloc(count, sizeof(AABB));

    for (i = 0; i < count; i++) ptr_array[i] = first + i;

    build_tree(0, count, 0);
    
    free(left_box);
    free(right_box);

    return bvh;
}

float get_surface_area(uint32_t start, uint32_t end) {
    AABB aabb;

    aabb = (AABB){
        .min = {FLT_MAX, FLT_MAX, FLT_MAX},
        .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
    };

    for (; start < end; start++) {
        aabb = aabb_merge(aabb, ptr_array[start]->aabb);
    }

    return SA(aabb);
}

float SA(AABB aabb) {
    float x, y, z;
    x = aabb.max[0] - aabb.min[0];
    y = aabb.max[1] - aabb.min[1];
    z = aabb.max[2] - aabb.min[2];
    

    return 2 * (x * y + x * z + y * z);
}

void build_boxes(uint32_t start, uint32_t end, AABB *left_box, AABB *right_box) {
    uint32_t i;

    left_box[start] = ptr_array[start]->aabb;
    for (i = start + 1; i < end - 1; i++) {
        left_box[i] = aabb_merge(left_box[i - 1], ptr_array[i]->aabb);
    }

    right_box[end - 1] = ptr_array[end - 1]->aabb;
    for (i = end - 1; i-- > start;) {
        right_box[i] = aabb_merge(right_box[i + 1], ptr_array[i]->aabb);
    }
}

uint32_t create_node(uint32_t first_or_right, uint32_t count, uint32_t idx) {
    uint32_t i;
    AABB aabb;
    BVHNode *node;

    aabb = (AABB){
        .min = {FLT_MAX, FLT_MAX, FLT_MAX},
        .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
    };

    if (count) 
        for (i = 0; i < count; i++) aabb = aabb_merge(aabb, ptr_array[first_or_right + i]->aabb);
    else
        aabb = aabb_merge(nodes[idx + 1].aabb, nodes[first_or_right].aabb);

    nodes[idx] = (BVHNode){
        .aabb = aabb,
        .first_primitive_or_right_child = first_or_right,
        .primitive_count = count
    };

    return idx + 1;
}

uint32_t build_tree(uint32_t start, uint32_t end, uint32_t idx) {
    int (*cmp)(const void *, const void *);
    uint32_t count, i, c_left, c_right, split, right_child, next_free;
    float cost, min_cost, sa_parent;

    count = end - start;
    sa_parent = get_surface_area(start, end);

    if (count <= 4) return create_node(start, count, idx);

    min_cost = FLT_MAX;
    split = 0;

    qsort(ptr_array + start, count, sizeof(Object *), x_compare);
    build_boxes(start, end, left_box, right_box);

    for (i = start; i < end - 1; i++) {
        c_left = i - start + 1;
        c_right = end - i - 1;

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

    qsort(ptr_array + start, count, sizeof(Object *), y_compare);
    build_boxes(start, end, left_box, right_box);

    for (i = start; i < end - 1; i++) {
        c_left = i - start + 1;
        c_right = end - i - 1;

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

    qsort(ptr_array + start, count, sizeof(Object *), z_compare);
    build_boxes(start, end, left_box, right_box);

    for (i = start; i < end - 1; i++) {
        c_left = i - start + 1;
        c_right = end - i - 1;

        cost = 
        1
        + (SA(left_box[i]) / sa_parent) * c_left
        + (SA(right_box[i + 1]) / sa_parent) * c_right;

        if (cost < min_cost) {
            min_cost = cost;
            cmp = z_compare;
            split = c_left;
        }
    }

    qsort(ptr_array + start, count, sizeof(Object *), cmp);

    right_child = build_tree(start, start + split, idx + 1);
    next_free = build_tree(start + split, end, right_child);

    create_node(right_child, 0, idx);
    return next_free;
}