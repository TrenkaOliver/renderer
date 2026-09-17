#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
#include <string.h>

#include "accel/bvh.h"
#include "accel/cmp.h"
#include "geometry/triangle.h"

#define C_TRAVELSAL 1
#define C_INTERSECT 1
#define BIN_COUNT 32
#define LEAF_SIZE 8

typedef struct Bin {
    AABB aabb;
    uint32_t count;
} Bin;

static Object **ptr_array;
static BVHNode *nodes;
uint32_t node_count;
uint32_t leaf_count = 0;
uint32_t object_count;

float get_surface_area(uint32_t start, uint32_t end);
float SA(AABB aabb);
uint32_t create_node(uint32_t first_or_right, uint32_t count, uint32_t idx);
uint32_t build_tree(uint32_t start, uint32_t end, uint32_t idx);
uint32_t collapse_bvh_node(BVH *bvh, uint32_t idx);

BVH create_bvh(Object *first, size_t count) {
    size_t i;
    BVH bvh;

    bvh.nodes = nodes = calloc(count * 2 - 1, sizeof(BVHNode));
    bvh.objects = ptr_array = calloc(count, sizeof(Object *));
    object_count = count;

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

    node_count++;
    if (count) leaf_count++;


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
            (SA(left_bounds[i]) / parent_sa) * left_count[i] +
            (SA(right_bounds[i + 1]) / parent_sa) * right_count[i + 1];
            
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

uint32_t bvh8node_count = 0;
uint32_t bvh8node_capacity = 32;
uint32_t bvh8nodes_offset = 0;
BVH8Node *bvh8nodes;

SoATriangle *triangles_ptr;
uint32_t next = 0;
uint32_t triangles_offset = 0;


BVH8Tree create_bvh8_tree(Object *first, size_t count) {
    BVH bvh = create_bvh(first, count);

    bvh8nodes = malloc(bvh8node_capacity * sizeof(BVH8Node) + 32);
    bvh8nodes_offset = (32 -(size_t)bvh8nodes % 32);
    bvh8nodes = (BVH8Node *)((char *)bvh8nodes + bvh8nodes_offset);

    SoATriangle triangles;

    triangles_offset = 8 * leaf_count;
    triangles.offset = triangles_offset;

    triangles.arr = malloc(25 * triangles_offset * sizeof(float));

    // triangles.ax = malloc(leaf_count * 8 * sizeof(float));
    // triangles.ay = malloc(leaf_count * 8 * sizeof(float));
    // triangles.az = malloc(leaf_count * 8 * sizeof(float));
    // triangles.bx = malloc(leaf_count * 8 * sizeof(float));
    // triangles.by = malloc(leaf_count * 8 * sizeof(float));
    // triangles.bz = malloc(leaf_count * 8 * sizeof(float));
    // triangles.cx = malloc(leaf_count * 8 * sizeof(float));
    // triangles.cy = malloc(leaf_count * 8 * sizeof(float));
    // triangles.cz = malloc(leaf_count * 8 * sizeof(float));
    // triangles.nax = malloc(leaf_count * 8 * sizeof(float));
    // triangles.nay = malloc(leaf_count * 8 * sizeof(float));
    // triangles.naz = malloc(leaf_count * 8 * sizeof(float));
    // triangles.nbx = malloc(leaf_count * 8 * sizeof(float));
    // triangles.nby = malloc(leaf_count * 8 * sizeof(float));
    // triangles.nbz = malloc(leaf_count * 8 * sizeof(float));
    // triangles.ncx = malloc(leaf_count * 8 * sizeof(float));
    // triangles.ncy = malloc(leaf_count * 8 * sizeof(float));
    // triangles.ncz = malloc(leaf_count * 8 * sizeof(float));
    // triangles.ngx = malloc(leaf_count * 8 * sizeof(float));
    // triangles.ngy = malloc(leaf_count * 8 * sizeof(float));
    // triangles.ngz = malloc(leaf_count * 8 * sizeof(float));
    // triangles.tax = malloc(leaf_count * 8 * sizeof(float));
    // triangles.tay = malloc(leaf_count * 8 * sizeof(float));
    // triangles.tbx = malloc(leaf_count * 8 * sizeof(float));
    // triangles.tby = malloc(leaf_count * 8 * sizeof(float));
    triangles.obj_idx = malloc(leaf_count * 8 * sizeof(uint32_t));
    triangles.mat_ptr_arr = malloc(leaf_count * 8 * sizeof(void *));

    triangles_ptr = &triangles;

    collapse_bvh_node(&bvh, 0);

    BVH8Tree bvh8tree = {
        .nodes = bvh8nodes,
        .objects = ptr_array,
        .triangles = triangles
    };

    free(nodes);

    return bvh8tree;
}
void setup_leaf(BVH *bvh, uint32_t first_triangle, uint32_t count, int src) {
    int i;
    for (i = 0; i < count; i++) {
        // triangles_ptr->ax[next + i] = ptr_array[first_triangle + i]->type.triangle.a.x;
        // triangles_ptr->ay[next + i] = ptr_array[first_triangle + i]->type.triangle.a.y;
        // triangles_ptr->az[next + i] = ptr_array[first_triangle + i]->type.triangle.a.z;
        // triangles_ptr->bx[next + i] = ptr_array[first_triangle + i]->type.triangle.b.x;
        // triangles_ptr->by[next + i] = ptr_array[first_triangle + i]->type.triangle.b.y;
        // triangles_ptr->bz[next + i] = ptr_array[first_triangle + i]->type.triangle.b.z;
        // triangles_ptr->cx[next + i] = ptr_array[first_triangle + i]->type.triangle.c.x;
        // triangles_ptr->cy[next + i] = ptr_array[first_triangle + i]->type.triangle.c.y;
        // triangles_ptr->cz[next + i] = ptr_array[first_triangle + i]->type.triangle.c.z;
        // triangles_ptr->nax[next + i] = ptr_array[first_triangle + i]->type.triangle.na.x;
        // triangles_ptr->nay[next + i] = ptr_array[first_triangle + i]->type.triangle.na.y;
        // triangles_ptr->naz[next + i] = ptr_array[first_triangle + i]->type.triangle.na.z;
        // triangles_ptr->nbx[next + i] = ptr_array[first_triangle + i]->type.triangle.nb.x;
        // triangles_ptr->nby[next + i] = ptr_array[first_triangle + i]->type.triangle.nb.y;
        // triangles_ptr->nbz[next + i] = ptr_array[first_triangle + i]->type.triangle.nb.z;
        // triangles_ptr->ncx[next + i] = ptr_array[first_triangle + i]->type.triangle.nc.x;
        // triangles_ptr->ncy[next + i] = ptr_array[first_triangle + i]->type.triangle.nc.y;
        // triangles_ptr->ncz[next + i] = ptr_array[first_triangle + i]->type.triangle.nc.z;
        // triangles_ptr->ngx[next + i] = ptr_array[first_triangle + i]->type.triangle.ng.x;
        // triangles_ptr->ngy[next + i] = ptr_array[first_triangle + i]->type.triangle.ng.y;
        // triangles_ptr->ngz[next + i] = ptr_array[first_triangle + i]->type.triangle.ng.z;
        // triangles_ptr->tax[next + i] = ptr_array[first_triangle + i]->type.triangle.ta.x;
        // triangles_ptr->tay[next + i] = ptr_array[first_triangle + i]->type.triangle.ta.y;
        // triangles_ptr->tbx[next + i] = ptr_array[first_triangle + i]->type.triangle.tb.x;
        // triangles_ptr->tby[next + i] = ptr_array[first_triangle + i]->type.triangle.tb.y;

        triangles_ptr->arr[next + triangles_offset * 0 + i] = ptr_array[first_triangle + i]->type.triangle.a.x;
        triangles_ptr->arr[next + triangles_offset * 1 + i] = ptr_array[first_triangle + i]->type.triangle.a.y;
        triangles_ptr->arr[next + triangles_offset * 2 + i] = ptr_array[first_triangle + i]->type.triangle.a.z;
        triangles_ptr->arr[next + triangles_offset * 3 + i] = ptr_array[first_triangle + i]->type.triangle.b.x;
        triangles_ptr->arr[next + triangles_offset * 4 + i] = ptr_array[first_triangle + i]->type.triangle.b.y;
        triangles_ptr->arr[next + triangles_offset * 5 + i] = ptr_array[first_triangle + i]->type.triangle.b.z;
        triangles_ptr->arr[next + triangles_offset * 6 + i] = ptr_array[first_triangle + i]->type.triangle.c.x;
        triangles_ptr->arr[next + triangles_offset * 7 + i] = ptr_array[first_triangle + i]->type.triangle.c.y;
        triangles_ptr->arr[next + triangles_offset * 8 + i] = ptr_array[first_triangle + i]->type.triangle.c.z;
        triangles_ptr->arr[next + triangles_offset * 9 + i] = ptr_array[first_triangle + i]->type.triangle.na.x;
        triangles_ptr->arr[next + triangles_offset * 10 + i] = ptr_array[first_triangle + i]->type.triangle.na.y;
        triangles_ptr->arr[next + triangles_offset * 11 + i] = ptr_array[first_triangle + i]->type.triangle.na.z;
        triangles_ptr->arr[next + triangles_offset * 12 + i] = ptr_array[first_triangle + i]->type.triangle.nb.x;
        triangles_ptr->arr[next + triangles_offset * 13 + i] = ptr_array[first_triangle + i]->type.triangle.nb.y;
        triangles_ptr->arr[next + triangles_offset * 14 + i] = ptr_array[first_triangle + i]->type.triangle.nb.z;
        triangles_ptr->arr[next + triangles_offset * 15 + i] = ptr_array[first_triangle + i]->type.triangle.nc.x;
        triangles_ptr->arr[next + triangles_offset * 16 + i] = ptr_array[first_triangle + i]->type.triangle.nc.y;
        triangles_ptr->arr[next + triangles_offset * 17 + i] = ptr_array[first_triangle + i]->type.triangle.nc.z;
        triangles_ptr->arr[next + triangles_offset * 18 + i] = ptr_array[first_triangle + i]->type.triangle.ng.x;
        triangles_ptr->arr[next + triangles_offset * 19 + i] = ptr_array[first_triangle + i]->type.triangle.ng.y;
        triangles_ptr->arr[next + triangles_offset * 20 + i] = ptr_array[first_triangle + i]->type.triangle.ng.z;
        triangles_ptr->arr[next + triangles_offset * 21 + i] = ptr_array[first_triangle + i]->type.triangle.ta.x;
        triangles_ptr->arr[next + triangles_offset * 22 + i] = ptr_array[first_triangle + i]->type.triangle.ta.y;
        triangles_ptr->arr[next + triangles_offset * 23 + i] = ptr_array[first_triangle + i]->type.triangle.tb.x;
        triangles_ptr->arr[next + triangles_offset * 24 + i] = ptr_array[first_triangle + i]->type.triangle.tb.y;

        triangles_ptr->obj_idx[next + i] = first_triangle + i;
        triangles_ptr->mat_ptr_arr[next + i] = ptr_array[first_triangle + i ]->material;
    }
    // for (i++; i < 8; i++) {
    //     triangles_ptr->ax[next + i] = 0.0f;
    //     triangles_ptr->ay[next + i] = 0.0f;
    //     triangles_ptr->az[next + i] = 0.0f;
    //     triangles_ptr->bx[next + i] = 0.0f;
    //     triangles_ptr->by[next + i] = 0.0f;
    //     triangles_ptr->bz[next + i] = 0.0f;
    //     triangles_ptr->cx[next + i] = 0.0f;
    //     triangles_ptr->cy[next + i] = 0.0f;
    //     triangles_ptr->cz[next + i] = 0.0f;
    //     triangles_ptr->nax[next + i] = 0.0f;
    //     triangles_ptr->nay[next + i] = 0.0f;
    //     triangles_ptr->naz[next + i] = 0.0f;
    //     triangles_ptr->nbx[next + i] = 0.0f;
    //     triangles_ptr->nby[next + i] = 0.0f;
    //     triangles_ptr->nbz[next + i] = 0.0f;
    //     triangles_ptr->ncx[next + i] = 0.0f;
    //     triangles_ptr->ncy[next + i] = 0.0f;
    //     triangles_ptr->ncz[next + i] = 0.0f;
    //     triangles_ptr->ngx[next + i] = 0.0f;
    //     triangles_ptr->ngy[next + i] = 0.0f;
    //     triangles_ptr->ngz[next + i] = 0.0f;
    //     triangles_ptr->tax[next + i] = 0.0f;
    //     triangles_ptr->tay[next + i] = 0.0f;
    //     triangles_ptr->tbx[next + i] = 0.0f;
    //     triangles_ptr->tby[next + i] = 0.0f;
    //     triangles_ptr->mat_ptr_arr[next + i] = NULL;
    // }
}

uint32_t collapse_bvh_node(BVH *bvh, uint32_t idx) {
    AABB bounds[8];
    uint32_t indices[8];
    uint8_t primitive_count[8] = {0};
    uint8_t count;

    indices[0] = idx + 1;
    bounds[0] = bvh->nodes[indices[0]].aabb;
    primitive_count[0] = bvh->nodes[indices[0]].primitive_count;

    if (primitive_count[0]) {
        setup_leaf(bvh, bvh->nodes[indices[0]].first_primitive_or_right_child, primitive_count[0], 0);
        indices[0] = next;
        next += primitive_count[0];
    }
    
    indices[1] = bvh->nodes[idx].first_primitive_or_right_child;
    bounds[1] = bvh->nodes[indices[1]].aabb;
    primitive_count[1] = bvh->nodes[indices[1]].primitive_count;

    if (primitive_count[1]) {
        setup_leaf(bvh, bvh->nodes[indices[1]].first_primitive_or_right_child, primitive_count[1], 1);
        indices[1] = next;
        next += primitive_count[1];
    }

    count = 2;

    while (count < 8) {
        float lowes_cost = FLT_MAX;
        float current_cost = 0.0;
        int best_split = -1;

        for (uint32_t i = 0; i < count; i++)
            current_cost += SA(bounds[i]);

        for (int i = 0; i < count; i++) {
            float cost = 0.0;

            if (primitive_count[i]) continue;

            cost = 
            current_cost
            - SA(bounds[i])
            + SA(bvh->nodes[indices[i] + 1].aabb)
            + SA(bvh->nodes[bvh->nodes[indices[i]].first_primitive_or_right_child].aabb);

            if (cost < lowes_cost) {
                best_split = i;
                lowes_cost = cost;
            }
        }

        if (best_split == -1) break;

        uint32_t right = bvh->nodes[indices[best_split]].first_primitive_or_right_child;
        indices[count] = right;
        bounds[count] = bvh->nodes[right].aabb;
        primitive_count[count] = bvh->nodes[right].primitive_count;

        if (primitive_count[count]) {
            setup_leaf(bvh, bvh->nodes[right].first_primitive_or_right_child, primitive_count[count], 2);
            indices[count] = next;
            next += primitive_count[count];
        }

        indices[best_split] += 1;
        bounds[best_split] = bvh->nodes[indices[best_split]].aabb;
        primitive_count[best_split] = bvh->nodes[indices[best_split]].primitive_count;
        //if (primitive_count[best_split]) indices[best_split] = bvh->nodes[indices[best_split]].first_primitive_or_right_child;

        if (primitive_count[best_split]) {
            setup_leaf(bvh, bvh->nodes[indices[best_split]].first_primitive_or_right_child, primitive_count[best_split], 3);
            indices[best_split] = next;
            next += primitive_count[best_split];
        }

        count++;
    }

    int left = 0;
    int right = count - 1;

    while (left <= right) {
        while (left < count && !primitive_count[left]) left++;
        while (right >= 0 && primitive_count[right]) right--;

        if (left < right) {
            uint32_t tmp_index = indices[left];
            indices[left] = indices[right];
            indices[right] = tmp_index;

            AABB tmp_bounds = bounds[left];
            bounds[left] = bounds[right];
            bounds[right] = tmp_bounds;

            uint8_t tmp_leaf = primitive_count[left];
            primitive_count[left] = primitive_count[right];
            primitive_count[right] = tmp_leaf;

            left++;
            right--;
        }
    }
    
    if (bvh8node_count == bvh8node_capacity) {
        bvh8node_capacity *= 2;
        
        char *new_ptr = malloc(bvh8node_capacity * sizeof(BVH8Node) + 32);
        uint32_t new_offset = 32 - (size_t)new_ptr % 32;
        new_ptr += new_offset;
        
        memcpy(new_ptr, bvh8nodes, bvh8node_count * sizeof(BVH8Node));

        free((char *)bvh8nodes - bvh8nodes_offset);
        
        bvh8nodes = (BVH8Node *)new_ptr;
        bvh8nodes_offset = new_offset;

        // bvh8nodes = realloc((char *)bvh8nodes - bvh8nodes_offset, bvh8node_capacity * sizeof(BVH8Node) + 32);
        // printf("offset: %u, new_offset: %u\n", bvh8nodes_offset, (32 -(size_t)bvh8nodes % 32));
        // bvh8nodes_offset = (32 -(size_t)bvh8nodes % 32);
        // bvh8nodes = (BVH8Node *)((char *)bvh8nodes + bvh8nodes_offset);
    }

    uint32_t node_index = bvh8node_count++;    
    uint32_t child_indices[8];
    
    for (int i = 0; i < left; i++) {
        child_indices[i] = collapse_bvh_node(bvh, indices[i]);
    }    
    
    BVH8Node *node_ptr = bvh8nodes + node_index;

    for (int i = 0; i < left; i++) {
        indices[i] = child_indices[i];
    }    

    float min_x[8];
    float min_y[8];
    float min_z[8];
    float max_x[8];
    float max_y[8];
    float max_z[8];

    int i;

    for (i = 0; i < count; i++) {
        node_ptr->idx[i] = indices[i];
        node_ptr->primitive_count[i] = primitive_count[i];
        min_x[i] = bounds[i].min[0];
        min_y[i] = bounds[i].min[1];
        min_z[i] = bounds[i].min[2];
        max_x[i] = bounds[i].max[0];
        max_y[i] = bounds[i].max[1];
        max_z[i] = bounds[i].max[2];
    }

    for (; i < 8; i++) {
        min_x[i] = FLT_MAX;
        min_y[i] = FLT_MAX;
        min_z[i] = FLT_MAX;
        max_x[i] = -FLT_MAX;
        max_y[i] = -FLT_MAX;
        max_z[i] = -FLT_MAX;
    }

    node_ptr->bounds.min.x = _mm256_loadu_ps(min_x);
    node_ptr->bounds.min.y = _mm256_loadu_ps(min_y);
    node_ptr->bounds.min.z = _mm256_loadu_ps(min_z);
    node_ptr->bounds.max.x = _mm256_loadu_ps(max_x);
    node_ptr->bounds.max.y = _mm256_loadu_ps(max_y);
    node_ptr->bounds.max.z = _mm256_loadu_ps(max_z);    
    
    node_ptr->internal_count = left;
    node_ptr->leaf_count = count - left;

    return node_index;
}