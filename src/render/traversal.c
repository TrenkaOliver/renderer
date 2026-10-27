#include "render/trace.h"
#include <float.h>
#include <stdio.h>
#include "render/render.h"
#include "geometry/triangle.h"
#include "math/ray.h"

typedef struct ScalarHitResult {
    float t;
    float u;
    float v;
    uint32_t idx;
}  ScalarHitResult;

static inline void move_near_fw(uint32_t idx[], float t[], int count) {
    for (int i = 1; i < count; i++) {
        float key_t = t[i];
        uint32_t key_idx = idx[i];
        int j = i;

        while (j > 0 && t[j - 1] < key_t) {
            t[j] = t[j - 1];
            idx[j] = idx[j - 1];
            j--;
        }

        t[j] = key_t;
        idx[j] = key_idx;
    }
}

static inline ScalarHitResult scalar_get_first_object(const SoARay * restrict ray, const uint32_t lane, const BVH8Tree * restrict bvh) {
    PackedInfo info;
    uint32_t sp, idx_stack[128], idx, best_idx;
    __m256 ps_t_aabb, ps_t_triangle, ps_t_min, valid;
    float t_min, t_values[8], t_stack[128], u, v;
    int valid_mask;

    ps_t_min = _mm256_set1_ps(FLT_MAX);
    t_min = FLT_MAX;
    sp = 0;
    idx_stack[sp] = 0;
    t_stack[sp] = 0.0;
    sp++;
    best_idx = -1;

    ps_Vec ray_o = {
        .x = _mm256_set1_ps(ray->ox[lane]),
        .y = _mm256_set1_ps(ray->oy[lane]),
        .z = _mm256_set1_ps(ray->oz[lane]),

    };
    ps_Vec ray_v = {
        .x = _mm256_set1_ps(ray->vx[lane]),
        .y = _mm256_set1_ps(ray->vy[lane]),
        .z = _mm256_set1_ps(ray->vz[lane]),
    };
    ps_Vec inv_v = (ps_Vec){
        .x = _mm256_set1_ps(1.0f / ray->vx[lane]),
        .y = _mm256_set1_ps(1.0f / ray->vy[lane]),
        .z = _mm256_set1_ps(1.0f / ray->vz[lane]),
    };

    const __m256 zero = _mm256_setzero_ps();
    const __m256 ps_max = _mm256_set1_ps(FLT_MAX);

    while (sp) {
        sp--;
        if (t_min < t_stack[sp]) continue;
        idx = idx_stack[sp];

        const BVH8Node *node = bvh->nodes + idx;

        ps_t_aabb = packed_aabb_ray_intersection(&node->bounds, ray_o, inv_v);

        valid = _mm256_and_ps(
            _mm256_cmp_ps(ps_t_aabb, zero, _CMP_GE_OQ),
            _mm256_cmp_ps(ps_t_aabb, ps_t_min, _CMP_LT_OQ)
        );

        valid_mask = _mm256_movemask_ps(valid);

        for (int i = node->internal_count; i < node->internal_count + node->leaf_count; i++) {
            if (!(valid_mask & (1 << i))) continue;
            ps_t_triangle = packed_triangle_ray_intersection(node->idx[i], node->primitive_count[i], ray_o, ray_v, bvh->runtime_triangles.arr, &info);

            __m256 valid_hits = _mm256_and_ps(
                _mm256_cmp_ps(ps_t_triangle, zero, _CMP_GT_OQ),
                _mm256_cmp_ps(ps_t_triangle, ps_t_min, _CMP_LT_OQ)
            );

            const __m256i lane_mask = _mm256_cmpgt_epi32(
                _mm256_set1_epi32(node->primitive_count[i]),
                _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7)
            );

            valid_hits = _mm256_and_ps(
                valid_hits,
                _mm256_castsi256_ps(lane_mask)
            );

            unsigned valid_bits = _mm256_movemask_ps(valid_hits);

            if (valid_bits) {
                __m256 candidates = _mm256_blendv_ps(ps_max, ps_t_triangle, valid_hits);

                __m256 m = _mm256_min_ps(candidates, _mm256_permute2f128_ps(candidates, candidates, 1));
                m = _mm256_min_ps(m, _mm256_permute_ps(m, _MM_SHUFFLE(1, 0, 3, 2)));
                m = _mm256_min_ps(m, _mm256_permute_ps(m, _MM_SHUFFLE(2, 3, 0, 1)));

                t_min =  _mm256_cvtss_f32(m);
                ps_t_min = _mm256_set1_ps(t_min);

                __m256 nearest_mask = _mm256_cmp_ps(candidates, ps_t_min, _CMP_EQ_OQ);

                int lane = __builtin_ctz(_mm256_movemask_ps(nearest_mask));
                best_idx = node->triangle_idx[i] + lane;

                __m256i lane_idx = _mm256_set1_epi32(lane);

                u = _mm256_cvtss_f32(_mm256_permutevar8x32_ps(info.u, lane_idx));
                v = _mm256_cvtss_f32(_mm256_permutevar8x32_ps(info.v, lane_idx));             
            }
        }

        _mm256_storeu_ps(t_values, ps_t_aabb);
        
        int count = 0;

        for (int i = 0; i < node->internal_count; i++) {
            if (valid_mask & (1 << i)) {
                idx_stack[sp + count] = node->idx[i];
                t_stack[sp + count] = t_values[i];
                count++;
            }
        }

        move_near_fw(idx_stack + sp, t_stack + sp, count);

        sp += count;
    }

    // printf("best_idx: %u, t_min: %f, u: %f, v: %f\n", best_idx, t_min, u, v);

    if (best_idx == (uint32_t)-1) 
        return (ScalarHitResult){.t = -1.0};
    else
        return (ScalarHitResult){.t = t_min, .u = u, .v = v, .idx = best_idx};
}

PackedHitResult packed_get_first_object(const SoARay * restrict ray, const BVH8Tree * restrict bvh, const DynArray * restrict materials, const DynArray *restrict textures) {
    ScalarHitResult res;
    float t[8];
    float u[8];
    float v[8];
    uint32_t indices[8];
    
    for (int i = 0; i < 8; i++) {
        res = scalar_get_first_object(ray, i, bvh);
        t[i] = res.t;
        u[i] = res.u;
        v[i] = res.v;
        indices[i] = res.idx;
    }

    const PackedRay packed_ray = {
        .o = (ps_Vec){
            .x = _mm256_load_ps(ray->ox),
            .y = _mm256_load_ps(ray->oy),
            .z = _mm256_load_ps(ray->oz),

        },
        .v = (ps_Vec){
            .x = _mm256_load_ps(ray->vx),
            .y = _mm256_load_ps(ray->vy),
            .z = _mm256_load_ps(ray->vz),
        }
    };
    
    return packed_triangle_result(
        t,
        _mm256_loadu_ps(u),
        _mm256_loadu_ps(v),
        indices,
        packed_ray,
        &bvh->runtime_triangles,
        &bvh->building_triangles,
        bvh->vertex_data,
        materials,
        textures
    );
}

static inline int scalar_is_shaded(const SoARay * restrict ray, const int lane, const BVH8Tree * restrict bvh) {
    PackedInfo info;
    uint32_t sp, idx_stack[128], idx;
    __m256 ps_t_aabb, ps_t_triangle, valid;
    float t_values[8];
    int valid_mask;

    sp = 0;
    idx_stack[sp] = 0;
    sp++;

    ps_Vec ray_o = {
        .x = _mm256_set1_ps(ray->ox[lane]),
        .y = _mm256_set1_ps(ray->oy[lane]),
        .z = _mm256_set1_ps(ray->oz[lane]),

    };
    ps_Vec ray_v = {
        .x = _mm256_set1_ps(ray->vx[lane]),
        .y = _mm256_set1_ps(ray->vy[lane]),
        .z = _mm256_set1_ps(ray->vz[lane]),
    };
    ps_Vec inv_v = (ps_Vec){
        .x = _mm256_set1_ps(1.0f / ray->vx[lane]),
        .y = _mm256_set1_ps(1.0f / ray->vy[lane]),
        .z = _mm256_set1_ps(1.0f / ray->vz[lane]),
    };

    const __m256 zero = _mm256_setzero_ps();

    while (sp) {
        sp--;
        idx = idx_stack[sp];

        const BVH8Node *node = bvh->nodes + idx;

        ps_t_aabb = packed_aabb_ray_intersection(&node->bounds, ray_o, inv_v);

        valid = _mm256_cmp_ps(ps_t_aabb, zero, _CMP_GE_OQ);

        valid_mask = _mm256_movemask_ps(valid);

        for (int i = node->internal_count; i < node->internal_count + node->leaf_count; i++) {
            if (!(valid_mask & (1 << i))) continue;
            ps_t_triangle = packed_triangle_ray_intersection(node->idx[i], node->primitive_count[i], ray_o, ray_v, bvh->runtime_triangles.arr, &info);

            __m256 valid_hits = _mm256_cmp_ps(ps_t_triangle, zero, _CMP_GT_OQ);

            // const __m256i lane_mask = _mm256_cmpgt_epi32(
            //     _mm256_set1_epi32(node->primitive_count[i]),
            //     _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7)
            // );

            // valid_hits = _mm256_and_ps(
            //     valid_hits,
            //     _mm256_castsi256_ps(lane_mask)
            // );

            if (_mm256_movemask_ps(valid_hits)) return 0;
        }

        _mm256_storeu_ps(t_values, ps_t_aabb);
        
        int count = 0;

        for (int i = 0; i < node->internal_count; i++) {
            if (valid_mask & (1 << i)) {
                idx_stack[sp + count] = node->idx[i];
                count++;
            }
        }

        sp += count;
    }

    return 1;
}

__m256 packed_is_shaded(const SoARay * restrict ray, const BVH8Tree * restrict bvh, const __m256 valid_shadow) {
    int visibility[8];
    int valid_shadow_mask = _mm256_movemask_ps(valid_shadow);
    
    for (int i = 0; i < 8; i++) {
        visibility[i] = (valid_shadow_mask & (1u << i)) ? scalar_is_shaded(ray, i, bvh) : 0;
    }

    return _mm256_castsi256_ps(_mm256_setr_epi32(
        visibility[0] ? -1 : 0,
        visibility[1] ? -1 : 0,
        visibility[2] ? -1 : 0,
        visibility[3] ? -1 : 0,
        visibility[4] ? -1 : 0,
        visibility[5] ? -1 : 0,
        visibility[6] ? -1 : 0,
        visibility[7] ? -1 : 0
    ));
}