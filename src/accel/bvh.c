#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
#include "accel/bvh.h"
#include "accel/cmp.h"

#define C_TRAVELSAL 1
#define C_INTERSECT 1
#define BIN_COUNT 32
#define LEAF_SIZE 4

typedef struct Bin {
    AABB aabb;
    uint32_t count;
} Bin;

static Object **ptr_array;
static BVHNode *nodes;

float get_surface_area(uint32_t start, uint32_t end);
float SA(AABB aabb);
uint32_t create_node(uint32_t first_or_right, uint32_t count, uint32_t idx);
uint32_t build_tree(uint32_t start, uint32_t end, uint32_t idx);

BVH create_bvh(Object *first, size_t count) {
    size_t i;
    BVH bvh;

    bvh.nodes = nodes = calloc(count * 2 - 1, sizeof(BVHNode));
    bvh.objects = ptr_array = calloc(count, sizeof(Object *));

    for (i = 0; i < count; i++) ptr_array[i] = first + i;

    build_tree(0, count, 0);

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
    uint32_t count = end - start;
    
    if (count <= LEAF_SIZE) return create_node(start, count, idx);
    
    Bin bins[BIN_COUNT];
    uint32_t i;
    float parent_sa = get_surface_area(start, end);
    float best_cost = FLT_MAX;
    float best_inv_width;
    float best_cent_min;
    int best_split;
    int best_axis;

    for (int axis = 0; axis < 3; axis++) {
        for (i = 0; i < BIN_COUNT; i++) {
            bins[i] = (Bin) {
                .aabb = (AABB) {
                    .min = {FLT_MAX, FLT_MAX, FLT_MAX},
                    .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
                },
                .count = 0
            };
        }

        float cent_min = FLT_MAX;
        float cent_max = -FLT_MAX;
        
        for (i = start; i < end; i++) {
            cent_min = fminf(cent_min, ptr_array[i]->centroid[axis]);
            cent_max = fmaxf(cent_max, ptr_array[i]->centroid[axis]);
        }
        
        if (cent_min == cent_max) continue;

        float inv_width = BIN_COUNT / (cent_max - cent_min);

        for (i = start; i < end; i++) {
            int bin = (int)((ptr_array[i]->centroid[axis] - cent_min) * inv_width);
            if (bin >= BIN_COUNT) bin = BIN_COUNT - 1;
            bins[bin].aabb = aabb_merge(bins[bin].aabb, ptr_array[i]->aabb);
            bins[bin].count++;
        }

        AABB left_bounds[BIN_COUNT];
        uint32_t left_count[BIN_COUNT];

        left_bounds[0] = bins[0].aabb;
        left_count[0] = bins[0].count;

        for (i = 1; i < BIN_COUNT; i++) {
            left_bounds[i] = aabb_merge(left_bounds[i - 1], bins[i].aabb);
            left_count[i] = left_count[i - 1] + bins[i].count;
        }

        AABB right_bounds[BIN_COUNT];
        uint32_t right_count[BIN_COUNT];

        right_bounds[BIN_COUNT - 1] = bins[BIN_COUNT - 1].aabb;
        right_count[BIN_COUNT - 1] = bins[BIN_COUNT - 1].count;

        for (i = BIN_COUNT - 2; i != (uint32_t)-1; i--) {
            right_bounds[i] = aabb_merge(right_bounds[i + 1], bins[i].aabb);
            right_count[i] = right_count[i + 1] + bins[i].count;
        }

        for (i = 0; i < BIN_COUNT - 1; i++) {
            if (left_count[i] == 0 || right_count[i + 1] == 0) continue;

            float cost = 
            C_TRAVELSAL +
            (SA(left_bounds[i]) / parent_sa) * left_count[i] * C_INTERSECT +
            (SA(right_bounds[i + 1]) / parent_sa) * right_count[i + 1] * C_INTERSECT;
            
            if (cost < best_cost) {
                best_cost = cost;
                best_cent_min = cent_min;
                best_inv_width = inv_width;
                best_split = i;
                best_axis = axis;                
            }
        }
    }

    uint32_t left = start;
    uint32_t right = end - 1;
    int bin;

    while (left <= right) {
        while (left < end) {
            bin = (int)((ptr_array[left]->centroid[best_axis] - best_cent_min) * best_inv_width);
            if (bin > best_split) break;

            left++;
        }

        while (right >= start) {
            bin = (int)((ptr_array[right]->centroid[best_axis] - best_cent_min) * best_inv_width);
            if (bin <= best_split) break;

            right--;
        }

        if (left < right) {
            Object *tmp = ptr_array[left];
            ptr_array[left] = ptr_array[right];
            ptr_array[right] = tmp;
            
            left++;
            right--;
        }
    }

    uint32_t right_child = build_tree(start, left, idx + 1);
    uint32_t next_free = build_tree(left, end, right_child);

    create_node(right_child, 0, idx);
    return next_free;
}