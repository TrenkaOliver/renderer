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

static BuildingTriangle building_triangles;
static BVHNode *nodes;
uint32_t node_count;
uint32_t leaf_count = 0;

float get_surface_area(uint32_t start, uint32_t end);
float SA(AABB aabb);
uint32_t create_node(uint32_t first_or_right, uint32_t count, uint32_t idx);
uint32_t build_tree(uint32_t start, uint32_t end, uint32_t idx);
uint32_t collapse_bvh_node(BVH *bvh, Vertex *vertices, uint32_t idx);

BVH create_bvh(BuildingTriangle *first, size_t count) {
    size_t i;
    BVH bvh;

    bvh.nodes = nodes = calloc(count * 2 - 1, sizeof(BVHNode));
    //bvh.triangles = ptr_array = calloc(count, sizeof(BuildingTriangle *));
    building_triangles = (BuildingTriangle) {
        .ai = malloc(count * sizeof(uint32_t)),
        .bi = malloc(count * sizeof(uint32_t)),
        .ci = malloc(count * sizeof(uint32_t)),
        .nai = malloc(count * sizeof(uint32_t)),
        .nbi = malloc(count * sizeof(uint32_t)),
        .nci = malloc(count * sizeof(uint32_t)),
        .tai = malloc(count * sizeof(uint32_t)),
        .tbi = malloc(count * sizeof(uint32_t)),
        .tci = malloc(count * sizeof(uint32_t)),
        .nx = malloc(count * sizeof(float)),
        .ny = malloc(count * sizeof(float)),
        .nz = malloc(count * sizeof(float)),
        .aabb = malloc(count * sizeof(AABB)),
        .centroid = malloc(count * 3 * sizeof(float)),
        .material = malloc(count * sizeof(void *))
    };
    
    bvh.triangles = (BuildingTriangle) {
        .ai = building_triangles.ai,
        .bi = building_triangles.bi,
        .ci = building_triangles.ci,
        .nai = building_triangles.nai,
        .nbi = building_triangles.nbi,
        .nci = building_triangles.nci,
        .tai = building_triangles.tai,
        .tbi = building_triangles.tbi,
        .tci = building_triangles.tci,
        .nx = building_triangles.nx,
        .ny = building_triangles.ny,
        .nz = building_triangles.nz,
        .aabb = building_triangles.aabb,
        .centroid = building_triangles.centroid,
        .material = building_triangles.material
    };

    //for (i = 0; i < count; i++) ptr_array[i] = first + i;
    for (i = 0; i < count; i++) {
        bvh.triangles.ai[i] = first->ai[i];
        bvh.triangles.bi[i] = first->bi[i];
        bvh.triangles.ci[i] = first->ci[i];

        bvh.triangles.nx[i] = first->nx[i];
        bvh.triangles.ny[i] = first->ny[i];
        bvh.triangles.nz[i] = first->nz[i];

        bvh.triangles.aabb[i] = first->aabb[i];
        bvh.triangles.centroid[i * 3 + 0] = first->centroid[i * 3 + 0];
        bvh.triangles.centroid[i * 3 + 1] = first->centroid[i * 3 + 1];
        bvh.triangles.centroid[i * 3 + 2] = first->centroid[i * 3 + 2];

        bvh.triangles.material[i] = first->material[i];
    }

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
        aabb = aabb_merge(aabb, building_triangles.aabb[start]);
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
        for (i = 0; i < count; i++) aabb = aabb_merge(aabb, building_triangles.aabb[first_or_right + i]);
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
            cent_min = fminf(cent_min, building_triangles.centroid[i * 3 + axis]);
            cent_max = fmaxf(cent_max, building_triangles.centroid[i * 3 + axis]);
        }
        
        if (cent_min == cent_max) continue;

        float inv_width = BIN_COUNT / (cent_max - cent_min);

        for (i = start; i < end; i++) {
            int bin = (int)((building_triangles.centroid[i * 3 + axis] - cent_min) * inv_width);
            if (bin >= BIN_COUNT) bin = BIN_COUNT - 1;
            bins[bin].aabb = aabb_merge(bins[bin].aabb, building_triangles.aabb[i]);
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
            bin = (int)((building_triangles.centroid[left * 3 + best_axis] - best_cent_min) * best_inv_width);
            if (bin > best_split) break;

            left++;
        }

        while (right >= start) {
            bin = (int)((building_triangles.centroid[right * 3 + best_axis] - best_cent_min) * best_inv_width);
            if (bin <= best_split) break;

            right--;
        }

        if (left < right) {
            // Object *tmp = ptr_array[left];
            // ptr_array[left] = ptr_array[right];
            // ptr_array[right] = tmp;

            uint32_t tmp_ai = building_triangles.ai[left];
            uint32_t tmp_bi = building_triangles.bi[left];
            uint32_t tmp_ci = building_triangles.ci[left];

            uint32_t tmp_nai = building_triangles.nai[left];
            uint32_t tmp_nbi = building_triangles.nbi[left];
            uint32_t tmp_nci = building_triangles.nci[left];

            uint32_t tmp_tai = building_triangles.tai[left];
            uint32_t tmp_tbi = building_triangles.tbi[left];
            uint32_t tmp_tci = building_triangles.tci[left];

            float tmp_nx = building_triangles.nx[left];
            float tmp_ny = building_triangles.ny[left];
            float tmp_nz = building_triangles.nz[left];

            AABB tmp_aabb = building_triangles.aabb[left];

            float tmp_centroid_x = building_triangles.centroid[left * 3 + 0];
            float tmp_centroid_y = building_triangles.centroid[left * 3 + 1];
            float tmp_centroid_z = building_triangles.centroid[left * 3 + 2];

            void *tmp_material = building_triangles.material[left];

            building_triangles.ai[left] = building_triangles.ai[right];
            building_triangles.bi[left] = building_triangles.bi[right];
            building_triangles.ci[left] = building_triangles.ci[right];

            building_triangles.nai[left] = building_triangles.nai[right];
            building_triangles.nbi[left] = building_triangles.nbi[right];
            building_triangles.nci[left] = building_triangles.nci[right];

            building_triangles.tai[left] = building_triangles.tai[right];
            building_triangles.tbi[left] = building_triangles.tbi[right];
            building_triangles.tci[left] = building_triangles.tci[right];

            building_triangles.nx[left] = building_triangles.nx[right];
            building_triangles.ny[left] = building_triangles.ny[right];
            building_triangles.nz[left] = building_triangles.nz[right];

            building_triangles.aabb[left] = building_triangles.aabb[right];

            building_triangles.centroid[left * 3 + 0] = building_triangles.centroid[right * 3 + 0];
            building_triangles.centroid[left * 3 + 1] = building_triangles.centroid[right * 3 + 1];
            building_triangles.centroid[left * 3 + 2] = building_triangles.centroid[right * 3 + 2];

            building_triangles.material[left] = building_triangles.material[right];

            building_triangles.ai[right] = tmp_ai;
            building_triangles.bi[right] = tmp_bi;
            building_triangles.ci[right] = tmp_ci;

            building_triangles.nai[right] = tmp_nai;
            building_triangles.nbi[right] = tmp_nbi;
            building_triangles.nci[right] = tmp_nci;

            building_triangles.tai[right] = tmp_tai;
            building_triangles.tbi[right] = tmp_tbi;
            building_triangles.tci[right] = tmp_tci;

            building_triangles.nx[right] = tmp_nx;
            building_triangles.ny[right] = tmp_ny;
            building_triangles.nz[right] = tmp_nz;

            building_triangles.aabb[right] = tmp_aabb;

            building_triangles.centroid[right * 3 + 0] = tmp_centroid_x;
            building_triangles.centroid[right * 3 + 1] = tmp_centroid_y;
            building_triangles.centroid[right * 3 + 2] = tmp_centroid_z;

            building_triangles.material[right] = tmp_material;
            
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

RuntimeTriangle *triangles_ptr;
uint32_t next = 0;
uint32_t triangles_offset = 0;


BVH8Tree create_bvh8_tree(BuildingTriangle *first, Vertex *vertices, size_t count) {
    BVH bvh = create_bvh(first, count);

    bvh8nodes = malloc(bvh8node_capacity * sizeof(BVH8Node) + 32);
    bvh8nodes_offset = (32 -(size_t)bvh8nodes % 32);
    bvh8nodes = (BVH8Node *)((char *)bvh8nodes + bvh8nodes_offset);

    RuntimeTriangle runtime_triangles;

    triangles_offset = 8 * leaf_count;
    runtime_triangles.offset = triangles_offset;

    runtime_triangles.arr = malloc(25 * triangles_offset * sizeof(float));
    runtime_triangles.building_idx = malloc(leaf_count * 8 * sizeof(uint32_t));
    runtime_triangles.mat_ptr_arr = malloc(leaf_count * 8 * sizeof(void *));

    triangles_ptr = &runtime_triangles;

    collapse_bvh_node(&bvh, vertices, 0);

    BVH8Tree bvh8tree = {
        .nodes = bvh8nodes,
        .building_triangles = (BuildingTriangle) {
            .ai = bvh.triangles.ai,
            .bi = bvh.triangles.bi,
            .ci = bvh.triangles.ci,
            .nai = bvh.triangles.nai,
            .nbi = bvh.triangles.nbi,
            .nci = bvh.triangles.nci,
            .tai = bvh.triangles.tai,
            .tbi = bvh.triangles.tbi,
            .tci = bvh.triangles.tci,
            .nx = bvh.triangles.nx,
            .ny = bvh.triangles.ny,
            .nz = bvh.triangles.nz,
            .aabb = bvh.triangles.aabb,
            .centroid = bvh.triangles.centroid,
            .material = bvh.triangles.material
        },
        .runtime_triangles = runtime_triangles
    };

    free(nodes);

    return bvh8tree;
}
void setup_leaf(BVH *bvh, Vertex *vertices, uint32_t first_triangle, uint32_t count, int src) {
    int i;
    for (i = 0; i < count; i++) {
        triangles_ptr->arr[next + triangles_offset * 0 + i] = vertices->x[building_triangles.ai[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 1 + i] = vertices->y[building_triangles.ai[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 2 + i] = vertices->z[building_triangles.ai[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 3 + i] = vertices->x[building_triangles.bi[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 4 + i] = vertices->y[building_triangles.bi[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 5 + i] = vertices->z[building_triangles.bi[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 6 + i] = vertices->x[building_triangles.ci[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 7 + i] = vertices->y[building_triangles.ci[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 8 + i] = vertices->z[building_triangles.ci[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 9 + i] = vertices->nx[building_triangles.nai[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 10 + i] = vertices->ny[building_triangles.nai[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 11 + i] = vertices->nz[building_triangles.nai[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 12 + i] = vertices->nx[building_triangles.nbi[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 13 + i] = vertices->ny[building_triangles.nbi[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 14 + i] = vertices->nz[building_triangles.nbi[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 15 + i] = vertices->nx[building_triangles.nci[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 16 + i] = vertices->ny[building_triangles.nci[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 17 + i] = vertices->nz[building_triangles.nci[first_triangle + i]];
        triangles_ptr->arr[next + triangles_offset * 18 + i] = building_triangles.nx[first_triangle + i];
        triangles_ptr->arr[next + triangles_offset * 19 + i] = building_triangles.ny[first_triangle + i];
        triangles_ptr->arr[next + triangles_offset * 20 + i] = building_triangles.nz[first_triangle + i];
        triangles_ptr->arr[next + triangles_offset * 21 + i] = 0.0f;
        triangles_ptr->arr[next + triangles_offset * 22 + i] = 0.0f;
        triangles_ptr->arr[next + triangles_offset * 23 + i] = 0.0f;
        triangles_ptr->arr[next + triangles_offset * 24 + i] = 0.0f;

        triangles_ptr->building_idx[next + i] = first_triangle + i;
        triangles_ptr->mat_ptr_arr[next + i] = building_triangles.material[first_triangle + i];
    }
}

uint32_t collapse_bvh_node(BVH *bvh, Vertex *vertices, uint32_t idx) {
    AABB bounds[8];
    uint32_t indices[8];
    uint8_t primitive_count[8] = {0};
    uint8_t count;

    indices[0] = idx + 1;
    bounds[0] = bvh->nodes[indices[0]].aabb;
    primitive_count[0] = bvh->nodes[indices[0]].primitive_count;

    if (primitive_count[0]) {
        setup_leaf(bvh, vertices, bvh->nodes[indices[0]].first_primitive_or_right_child, primitive_count[0], 0);
        indices[0] = next;
        next += primitive_count[0];
    }
    
    indices[1] = bvh->nodes[idx].first_primitive_or_right_child;
    bounds[1] = bvh->nodes[indices[1]].aabb;
    primitive_count[1] = bvh->nodes[indices[1]].primitive_count;

    if (primitive_count[1]) {
        setup_leaf(bvh, vertices, bvh->nodes[indices[1]].first_primitive_or_right_child, primitive_count[1], 1);
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
            setup_leaf(bvh, vertices, bvh->nodes[right].first_primitive_or_right_child, primitive_count[count], 2);
            indices[count] = next;
            next += primitive_count[count];
        }

        indices[best_split] += 1;
        bounds[best_split] = bvh->nodes[indices[best_split]].aabb;
        primitive_count[best_split] = bvh->nodes[indices[best_split]].primitive_count;
        //if (primitive_count[best_split]) indices[best_split] = bvh->nodes[indices[best_split]].first_primitive_or_right_child;

        if (primitive_count[best_split]) {
            setup_leaf(bvh, vertices, bvh->nodes[indices[best_split]].first_primitive_or_right_child, primitive_count[best_split], 3);
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
        child_indices[i] = collapse_bvh_node(bvh, vertices, indices[i]);
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