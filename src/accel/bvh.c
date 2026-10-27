#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
#include <string.h>

#include "accel/bvh.h"
#include "geometry/triangle.h"

#define C_TRAVERSAL 1.0f
#define C_INTERSECT 1.0f
#define BIN_COUNT 32
#define LEAF_SIZE 8

typedef struct Bin {
    AABB aabb;
    uint32_t count;
} Bin;

static OriginalTriangle building_triangles;
static BVHNode *nodes;
static float *sah_cost;
uint32_t node_count;
//uint32_t leaf_count = 0;

uint32_t triangles_offset = 0;

float get_surface_area(uint32_t start, uint32_t end);
float SA(AABB aabb);
uint32_t create_node(uint32_t first_or_right, uint32_t count, uint32_t idx);
uint32_t build_tree(uint32_t start, uint32_t end, uint32_t idx);
uint32_t collapse_bvh_node(BVH *bvh, float **vertex_data, uint32_t idx);

BVH create_bvh(OriginalTriangle *first, size_t count) {
    size_t i;
    BVH bvh;

    bvh.nodes = nodes = calloc(count * 2 - 1, sizeof(BVHNode));
    building_triangles = (OriginalTriangle) {
        .ai = malloc(count * sizeof(uint32_t)),
        .bi = malloc(count * sizeof(uint32_t)),
        .ci = malloc(count * sizeof(uint32_t)),
        .nx = malloc(count * sizeof(float)),
        .ny = malloc(count * sizeof(float)),
        .nz = malloc(count * sizeof(float)),
        .aabb = malloc(count * sizeof(AABB)),
        .centroid = malloc(count * 3 * sizeof(float)),
        .material = malloc(count * sizeof(void *))
    };

    bvh.triangles = (OriginalTriangle) {
        .ai = building_triangles.ai,
        .bi = building_triangles.bi,
        .ci = building_triangles.ci,
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
    triangles_offset += count;


    return idx + 1;
}

// AABB bound_cache[BIN_COUNT][BIN_COUNT];
// uint32_t count_cache[BIN_COUNT][BIN_COUNT];
// float cost_cache[BIN_COUNT][BIN_COUNT];
// float best_cost;
// float best_cent_min;
// float best_inv_width;
// int best_split[7] = {0};
// int current_split[7] = {0};
// int best_axis;

// void find_best_split(int start, int groups_left, float accumulated_cost, int axis, float cent_min, float inv_width);
// void swap_triangles(uint32_t a, uint32_t b);
// void setup_leaf(BVH *bvh, float **vertex_data, uint32_t first_triangle, uint32_t count, int src);

// uint32_t build_wide_tree(uint32_t start, uint32_t end, uint32_t idx) {
//     uint32_t count = end - start;

//     if (count < LEAF_SIZE) {
//         //leaf creation logic
//     }

//     Bin bins[BIN_COUNT];
//     float parent_sa = get_surface_area(start, end);
//     best_cost = FLT_MAX;
//     best_axis = -1;

//     for (int axis = 0; axis < 3; axis++) {
//         for (int i = 0; i < BIN_COUNT; i++) {
//             bins[i] = (Bin) {
//                 .aabb = (AABB) {
//                     .min = {FLT_MAX, FLT_MAX, FLT_MAX},
//                     .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
//                 },
//                 .count = 0
//             };
//         }

//         float cent_min = FLT_MAX;
//         float cent_max = -FLT_MAX;

//         for (int i = start; i < end; i++) {
//             cent_min = fminf(cent_min, building_triangles.centroid[i * 3 + axis]);
//             cent_max = fmaxf(cent_max, building_triangles.centroid[i * 3 + axis]);
//         }

//         if (cent_min == cent_max) continue;

//         float inv_width = BIN_COUNT / (cent_max - cent_min);

//         for (uint32_t i = start; i < end; i++) {
//             int bin = (int)((building_triangles.centroid[i * 3 + axis] - cent_min) * inv_width);
//             if (bin >= BIN_COUNT) bin = BIN_COUNT - 1;
//             bins[bin].aabb = aabb_merge(bins[bin].aabb, building_triangles.aabb[i]);
//             bins[bin].count++;
//         }

//         for (int i = 0; i < BIN_COUNT; i++) {
//             bound_cache[i][i] = bins[i].aabb;
//             count_cache[i][i] = bins[i].count;
//             for (int j = i + 1; j < BIN_COUNT; j++) {
//                 bound_cache[i][j] = aabb_merge(bound_cache[i][j - 1], bins[j].aabb);
//                 count_cache[i][j] = count_cache[i][j - 1] + bins[j].count;

//                 cost_cache[i][j] =
//                 C_TRAVERSAL +
//                 C_INTERSECT * (
//                     (SA(bound_cache[i][j]) / parent_sa) * count_cache[i][j]
//                 ); 
//             }
//         }

//         find_best_split(0, 8, 0, axis, cent_min, inv_width);
//     }

//     uint32_t bin_partition_map[BIN_COUNT];
//     uint32_t next_indices[8];
//     AABB aabbs[8];

//     for (int i = 0; i < BIN_COUNT; i++) {
//         bins[i] = (Bin) {
//             .aabb = (AABB) {
//                 .min = {FLT_MAX, FLT_MAX, FLT_MAX},
//                 .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
//             },
//             .count = 0
//         };
//     }

//     int i, j;
//     for (i = 0, j = 0; i < 7; i++) {
//         while (j < best_split[i]) {
//             bin_partition_map[j++] = i;
//             aabbs[i] = aabb_merge(aabbs[i], bins[j].aabb);
//         }
//     }

//     i++;
//     while (j < BIN_COUNT) {
//         bin_partition_map[j++] = 7;
//         aabbs[i] = aabb_merge(aabbs[i], bins[j].aabb);
//     }

//     next_indices[0] = 0;
//     for (int i = 0; i < 7; i++) {
//         next_indices[i + 1] = best_split[i];
//     }

//     for (uint32_t i = start; i < end; i++) {
//         int bin = (int)((building_triangles.centroid[i * 3 + best_axis] - best_cent_min) * best_inv_width);
//         swap_triangles(i, next_indices[bin_partition_map[bin]]++);
//     }

//     uint32_t child_indices[8];

//     int prev = 0;
//     for (int i = 0; i < 8; i++) {
//         child_indices[i] = build_wide_tree(prev, next_indices[i], 0);
//         prev = next_indices[i];
//     }

//     //recursing logic
// }

// void find_best_split(int start, int groups_left, float accumulated_cost, int axis, float cent_min, float inv_width) {
//     if (groups_left == 1) {
//         float total_cost = accumulated_cost + cost_cache[start][BIN_COUNT - 1];

//         if (total_cost < best_cost) {
//             best_cost = total_cost;
//             best_cent_min = cent_min;
//             best_inv_width = inv_width;

//             for (int i = 0; i < 7; i++) best_split[i] = current_split[i];
//         }        
//         return;
//     }

//     int max_end = BIN_COUNT - groups_left;

//     for (int end = start; end < max_end; end++) {
//         float group_cost = cost_cache[start][end];

//         current_split[8 - groups_left] = end + 1;

//         find_best_split(end + 1, groups_left - 1, accumulated_cost + group_cost, axis, cent_min, inv_width);
//     }
// }

// void swap_triangles(uint32_t a, uint32_t b) {
//     uint32_t tmp_ai = building_triangles.ai[a];
//     uint32_t tmp_bi = building_triangles.bi[a];
//     uint32_t tmp_ci = building_triangles.ci[a];

//     float tmp_nx = building_triangles.nx[a];
//     float tmp_ny = building_triangles.ny[a];
//     float tmp_nz = building_triangles.nz[a];

//     AABB tmp_aabb = building_triangles.aabb[a];

//     float tmp_centroid_x = building_triangles.centroid[a * 3 + 0];
//     float tmp_centroid_y = building_triangles.centroid[a * 3 + 1];
//     float tmp_centroid_z = building_triangles.centroid[a * 3 + 2];

//     uint32_t tmp_material = building_triangles.material[a];

//     building_triangles.ai[a] = building_triangles.ai[b];
//     building_triangles.bi[a] = building_triangles.bi[b];
//     building_triangles.ci[a] = building_triangles.ci[b];

//     building_triangles.nx[a] = building_triangles.nx[b];
//     building_triangles.ny[a] = building_triangles.ny[b];
//     building_triangles.nz[a] = building_triangles.nz[b];

//     building_triangles.aabb[a] = building_triangles.aabb[b];

//     building_triangles.centroid[a * 3 + 0] = building_triangles.centroid[b * 3 + 0];
//     building_triangles.centroid[a * 3 + 1] = building_triangles.centroid[b * 3 + 1];
//     building_triangles.centroid[a * 3 + 2] = building_triangles.centroid[b * 3 + 2];

//     building_triangles.material[a] = building_triangles.material[b];

//     building_triangles.ai[b] = tmp_ai;
//     building_triangles.bi[b] = tmp_bi;
//     building_triangles.ci[b] = tmp_ci;

//     building_triangles.nx[b] = tmp_nx;
//     building_triangles.ny[b] = tmp_ny;
//     building_triangles.nz[b] = tmp_nz;

//     building_triangles.aabb[b] = tmp_aabb;

//     building_triangles.centroid[b * 3 + 0] = tmp_centroid_x;
//     building_triangles.centroid[b * 3 + 1] = tmp_centroid_y;
//     building_triangles.centroid[b * 3 + 2] = tmp_centroid_z;

//     building_triangles.material[b] = tmp_material;
// }

uint32_t build_tree(uint32_t start, uint32_t end, uint32_t idx) {
    uint32_t count = end - start;

    if (count <= LEAF_SIZE) return create_node(start, count, idx);

    Bin bins[BIN_COUNT];
    uint32_t i;
    float parent_sa = get_surface_area(start, end);
    float best_cost = FLT_MAX;
    float best_inv_width;
    float best_cent_min;
    int best_split = -1;
    int best_axis = -1;

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
            C_TRAVERSAL + 
            C_INTERSECT * (
                (SA(left_bounds[i]) / parent_sa) * left_count[i] +
                (SA(right_bounds[i + 1]) / parent_sa) * right_count[i + 1]
            );

            if (cost < best_cost) {
                best_cost = cost;
                best_cent_min = cent_min;
                best_inv_width = inv_width;
                best_split = i;
                best_axis = axis;
            }
        }
    }

    if (best_axis == -1 || best_split == -1) return create_node(start, count, idx);

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

            float tmp_nx = building_triangles.nx[left];
            float tmp_ny = building_triangles.ny[left];
            float tmp_nz = building_triangles.nz[left];

            AABB tmp_aabb = building_triangles.aabb[left];

            float tmp_centroid_x = building_triangles.centroid[left * 3 + 0];
            float tmp_centroid_y = building_triangles.centroid[left * 3 + 1];
            float tmp_centroid_z = building_triangles.centroid[left * 3 + 2];

            uint32_t tmp_material = building_triangles.material[left];

            building_triangles.ai[left] = building_triangles.ai[right];
            building_triangles.bi[left] = building_triangles.bi[right];
            building_triangles.ci[left] = building_triangles.ci[right];

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
uint32_t f_next = 0;
uint32_t u_next = 0;

uint32_t created_nodes = 0;

BVH8Tree create_bvh8_tree(OriginalTriangle *first, float **vertex_data, size_t count) {
    BVH bvh = create_bvh(first, count);

    bvh8nodes = malloc(bvh8node_capacity * sizeof(BVH8Node) + 32);
    bvh8nodes_offset = (32 -(size_t)bvh8nodes % 32);
    bvh8nodes = (BVH8Node *)((char *)bvh8nodes + bvh8nodes_offset);

    RuntimeTriangle runtime_triangles;

    runtime_triangles.arr = malloc(9 * triangles_offset * sizeof(float) + 7 * sizeof(float));
    runtime_triangles.building_idx = malloc(triangles_offset * sizeof(uint32_t));

    triangles_ptr = &runtime_triangles;

    sah_cost = malloc(node_count * sizeof(float));

    for (uint32_t i = node_count; i-- > 0;) {
        BVHNode *node = &nodes[i];

        if (node->primitive_count) {
            sah_cost[i] = C_INTERSECT * node->primitive_count;
        } else {
            uint32_t left = i + 1;
            uint32_t right = node->first_primitive_or_right_child;

            float parent_sa = SA(node->aabb);

            if (parent_sa > 0.0f) {
                sah_cost[i] =
                    C_TRAVERSAL
                    + (SA(nodes[left].aabb) / parent_sa) * sah_cost[left]
                    + (SA(nodes[right].aabb) / parent_sa) * sah_cost[right];
            } else {
                // Degenerate bounds: avoid division by zero.
                sah_cost[i] = C_TRAVERSAL
                    + fmaxf(sah_cost[left], sah_cost[right]);
            }
        }
    }

    collapse_bvh_node(&bvh, vertex_data, 0);

    uint64_t total_leaves = 0;
    uint64_t total_primitives = 0;
    uint32_t nodes_with_leaves = 0;

    for (uint32_t i = 0; i < created_nodes; i++) {
        BVH8Node *node = &bvh8nodes[i];

        if (node->leaf_count > 0) {
            nodes_with_leaves++;
            total_leaves += node->leaf_count;

            for (uint32_t j = node->internal_count;
                j < node->internal_count + node->leaf_count;
                j++) {
                total_primitives += node->primitive_count[j];
            }
        }
    }

    printf("Nodes containing leaves: %u\n", nodes_with_leaves);
    printf("Total leaf entries: %lu\n", (unsigned long)total_leaves);
    printf("Average leaves per node: %.2f\n",
        nodes_with_leaves
            ? (double)total_leaves / nodes_with_leaves
            : 0.0);

    printf("Total primitives in leaves: %lu\n",
        (unsigned long)total_primitives);
    printf("Average primitives per leaf: %.2f\n",
        total_leaves
            ? (double)total_primitives / total_leaves
            : 0.0);

    BVH8Tree bvh8tree = {
        .nodes = bvh8nodes,
        .vertex_data = vertex_data,
        .building_triangles = (OriginalTriangle) {
            .ai = bvh.triangles.ai,
            .bi = bvh.triangles.bi,
            .ci = bvh.triangles.ci,
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
void setup_leaf(float **vertex_data, uint32_t first_triangle, uint32_t count) {
    float ax[count], ay[count], az[count];
    for (uint32_t i = 0; i < count; i++) {
        ax[i] = triangles_ptr->arr[f_next + count * 0 + i] = vertex_data[0][3 * building_triangles.ai[first_triangle + i] + 0];
        ay[i] = triangles_ptr->arr[f_next + count * 1 + i] = vertex_data[0][3 * building_triangles.ai[first_triangle + i] + 1];
        az[i] = triangles_ptr->arr[f_next + count * 2 + i] = vertex_data[0][3 * building_triangles.ai[first_triangle + i] + 2];

        triangles_ptr->arr[f_next + count * 3 + i] = vertex_data[0][3 * building_triangles.bi[first_triangle + i] + 0] - ax[i];
        triangles_ptr->arr[f_next + count * 4 + i] = vertex_data[0][3 * building_triangles.bi[first_triangle + i] + 1] - ay[i];
        triangles_ptr->arr[f_next + count * 5 + i] = vertex_data[0][3 * building_triangles.bi[first_triangle + i] + 2] - az[i];

        triangles_ptr->arr[f_next + count * 6 + i] = vertex_data[0][3 * building_triangles.ci[first_triangle + i] + 0] - ax[i];
        triangles_ptr->arr[f_next + count * 7 + i] = vertex_data[0][3 * building_triangles.ci[first_triangle + i] + 1] - ay[i];
        triangles_ptr->arr[f_next + count * 8 + i] = vertex_data[0][3 * building_triangles.ci[first_triangle + i] + 2] - az[i];

        triangles_ptr->building_idx[u_next + count * 0 + i] = first_triangle + i;
    }
}


uint32_t collapse_bvh_node(BVH *bvh, float **vertex_data, uint32_t idx) {
    AABB bounds[8];
    uint32_t indices[8];
    uint32_t triangle_indices[8];
    uint8_t primitive_count[8] = {0};
    uint8_t count;

    created_nodes++;

    indices[0] = idx + 1;
    bounds[0] = bvh->nodes[indices[0]].aabb;
    primitive_count[0] = bvh->nodes[indices[0]].primitive_count;

    if (primitive_count[0]) {
        setup_leaf(vertex_data, bvh->nodes[indices[0]].first_primitive_or_right_child, primitive_count[0]);
        indices[0] = f_next;
        triangle_indices[0] = u_next;
        u_next += primitive_count[0];
        f_next += primitive_count[0] * 9;
    }

    indices[1] = bvh->nodes[idx].first_primitive_or_right_child;
    bounds[1] = bvh->nodes[indices[1]].aabb;
    primitive_count[1] = bvh->nodes[indices[1]].primitive_count;

    if (primitive_count[1]) {
        setup_leaf(vertex_data, bvh->nodes[indices[1]].first_primitive_or_right_child, primitive_count[1]);
        indices[1] = f_next;
        triangle_indices[1] = u_next;
        u_next += primitive_count[1];
        f_next += primitive_count[1] * 9;
    }

    count = 2;

    while (count < 8) {
        float lowes_cost = FLT_MAX;
        int best_split = -1;

        for (int i = 0; i < count; i++) {
            if (primitive_count[i])
                continue;

            uint32_t left = indices[i] + 1;
            uint32_t right = bvh->nodes[indices[i]].first_primitive_or_right_child;

            float old_cost = SA(bounds[i]) * sah_cost[indices[i]];

            float new_cost =
            SA(bvh->nodes[left].aabb) * sah_cost[left]
            + SA(bvh->nodes[right].aabb) * sah_cost[right];

            float cost = new_cost - old_cost;

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
            setup_leaf(vertex_data, bvh->nodes[right].first_primitive_or_right_child, primitive_count[count]);
            indices[count] = f_next;
            triangle_indices[count] = u_next;
            u_next += primitive_count[count];
            f_next += primitive_count[count] * 9;
        }

        indices[best_split] += 1;
        bounds[best_split] = bvh->nodes[indices[best_split]].aabb;
        primitive_count[best_split] = bvh->nodes[indices[best_split]].primitive_count;
        //if (primitive_count[best_split]) indices[best_split] = bvh->nodes[indices[best_split]].first_primitive_or_right_child;

        if (primitive_count[best_split]) {
            setup_leaf(vertex_data, bvh->nodes[indices[best_split]].first_primitive_or_right_child, primitive_count[best_split]);
            indices[best_split] = f_next;
            triangle_indices[best_split] = u_next;
            u_next += primitive_count[best_split];
            f_next += primitive_count[best_split] * 9;
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

            uint32_t tmp_triangle_index = triangle_indices[left];
            triangle_indices[left] = triangle_indices[right];
            triangle_indices[right] = tmp_triangle_index;

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
        child_indices[i] = collapse_bvh_node(bvh, vertex_data, indices[i]);
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
        node_ptr->triangle_idx[i] = triangle_indices[i];
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