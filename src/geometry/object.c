#include "render/trace.h"
#include <float.h>
#include <stdio.h>
#include "render/render.h"

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

ScalarHitResult scalar_get_first_object(Ray *ray, BVH8Tree *bvh, DynArray *materials);

PackedHitResult packed_get_first_object(Ray ray[8], BVH8Tree *bvh, DynArray *materials, DynArray *textures) {
    ScalarHitResult res;
    float t[8];
    float u[8];
    float v[8];
    uint32_t indices[8];
    
    for (int i = 0; i < 8; i++) {
        res = scalar_get_first_object(ray + i, bvh, materials);
        t[i] = res.t;
        u[i] = res.u;
        v[i] = res.v;
        indices[i] = res.idx;
    }

    PackedRay packed_ray = {
        .o = (ps_Vec){
            .x = _mm256_set_ps(ray[7].o.x, ray[6].o.x, ray[5].o.x, ray[4].o.x, ray[3].o.x, ray[2].o.x, ray[1].o.x, ray[0].o.x),
            .y = _mm256_set_ps(ray[7].o.y, ray[6].o.y, ray[5].o.y, ray[4].o.y, ray[3].o.y, ray[2].o.y, ray[1].o.y, ray[0].o.y),
            .z = _mm256_set_ps(ray[7].o.z, ray[6].o.z, ray[5].o.z, ray[4].o.z, ray[3].o.z, ray[2].o.z, ray[1].o.z, ray[0].o.z)
        },
        .v = (ps_Vec){
            .x = _mm256_set_ps(ray[7].v.x, ray[6].v.x, ray[5].v.x, ray[4].v.x, ray[3].v.x, ray[2].v.x, ray[1].v.x, ray[0].v.x),
            .y = _mm256_set_ps(ray[7].v.y, ray[6].v.y, ray[5].v.y, ray[4].v.y, ray[3].v.y, ray[2].v.y, ray[1].v.y, ray[0].v.y),
            .z = _mm256_set_ps(ray[7].v.z, ray[6].v.z, ray[5].v.z, ray[4].v.z, ray[3].v.z, ray[2].v.z, ray[1].v.z, ray[0].v.z)
        },
        .inv_v = (ps_Vec){
            .x = _mm256_set_ps(ray[7].inv_v.x, ray[6].inv_v.x, ray[5].inv_v.x, ray[4].inv_v.x, ray[3].inv_v.x, ray[2].inv_v.x, ray[1].inv_v.x, ray[0].inv_v.x),
            .y = _mm256_set_ps(ray[7].inv_v.y, ray[6].inv_v.y, ray[5].inv_v.y, ray[4].inv_v.y, ray[3].inv_v.y, ray[2].inv_v.y, ray[1].inv_v.y, ray[0].inv_v.y),
            .z = _mm256_set_ps(ray[7].inv_v.z, ray[6].inv_v.z, ray[5].inv_v.z, ray[4].inv_v.z, ray[3].inv_v.z, ray[2].inv_v.z, ray[1].inv_v.z, ray[0].inv_v.z)
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

ScalarHitResult scalar_get_first_object(Ray *ray, BVH8Tree *bvh, DynArray *materials) {
    PackedInfo info;
    uint32_t sp, idx_stack[128], idx, best_idx;
    __m256 ps_t_aabb, ps_t_triangle, ps_t_min, valid;
    float t_min, t_values[8], t_stack[128], u, u_values[8], v, v_values[8];
    int valid_mask;

    ps_t_min = _mm256_set1_ps(FLT_MAX);
    t_min = FLT_MAX;
    sp = 0;
    idx_stack[sp] = 0;
    t_stack[sp] = 0.0;
    sp++;
    best_idx = -1;

    PackedRay packed_ray = {
        .o = ps_from_vec(ray->o),
        .v = ps_from_vec(ray->v),
        .inv_v = ps_from_vec(ray->inv_v),
    };

    __m256 zero = _mm256_setzero_ps();

    while (sp) {
        sp--;
        if (t_min < t_stack[sp]) continue;
        idx = idx_stack[sp];

        ps_t_aabb = packed_aabb_ray_intersection(&bvh->nodes[idx].bounds, &packed_ray);

        valid = _mm256_and_ps(
            _mm256_cmp_ps(ps_t_aabb, zero, _CMP_GE_OQ),
            _mm256_cmp_ps(ps_t_aabb, ps_t_min, _CMP_LT_OQ)
        );

        valid_mask = _mm256_movemask_ps(valid);

        for (int i = bvh->nodes[idx].internal_count; i < bvh->nodes[idx].internal_count + bvh->nodes[idx].leaf_count; i++) {
            if (!(valid_mask & (1 << i))) continue;
            ps_t_triangle = packed_triangle_ray_intersection(bvh->nodes[idx].idx[i], &packed_ray, &bvh->runtime_triangles, &info);

            __m256 eps = _mm256_set1_ps(EPSILON);
            __m256 valid_hits = _mm256_and_ps(
                _mm256_cmp_ps(ps_t_triangle, eps, _CMP_GE_OQ),
                _mm256_cmp_ps(ps_t_triangle, ps_t_min, _CMP_LT_OQ)
            );

            unsigned lane_bits = (1u << bvh->nodes[idx].primitive_count[i]) - 1u;
            unsigned valid_bits = _mm256_movemask_ps(valid_hits) & lane_bits;

            if (valid_bits) {
                __m256 candidates = _mm256_blendv_ps(_mm256_set1_ps(FLT_MAX), ps_t_triangle, valid_hits);

                __m256 m = _mm256_min_ps(candidates, _mm256_permute2f128_ps(candidates, candidates, 1));
                m = _mm256_min_ps(m, _mm256_permute_ps(m, _MM_SHUFFLE(1, 0, 3, 2)));
                m = _mm256_min_ps(m, _mm256_permute_ps(m, _MM_SHUFFLE(2, 3, 0, 1)));

                t_min =  _mm256_cvtss_f32(m);
                ps_t_min = _mm256_set1_ps(t_min);

                __m256 nearest_mask = _mm256_cmp_ps(candidates, ps_t_min, _CMP_EQ_OQ);

                int lane = __builtin_ctz(_mm256_movemask_ps(nearest_mask));
                best_idx = bvh->nodes[idx].idx[i] + lane;

                __m256i lane_idx = _mm256_set1_epi32(lane);

                u = _mm256_cvtss_f32(_mm256_permutevar8x32_ps(info.u, lane_idx));
                v = _mm256_cvtss_f32(_mm256_permutevar8x32_ps(info.v, lane_idx));             
            }
        }

        _mm256_storeu_ps(t_values, ps_t_aabb);
        
        int count = 0;

        for (int i = 0; i < bvh->nodes[idx].internal_count; i++) {
            if (valid_mask & (1 << i)) {
                idx_stack[sp + count] = bvh->nodes[idx].idx[i];
                t_stack[sp + count] = t_values[i];
                count++;
            }
        }

        move_near_fw(idx_stack + sp, t_stack + sp, count);

        sp += count;
    }

    if (best_idx == -1) 
        return (ScalarHitResult){.t = -1.0};
    else
        return (ScalarHitResult){.t = t_min, u, v, best_idx};
}

HitResult get_first_object(Ray *ray, BVH8Tree *bvh, DynArray *materials) {
    PackedInfo info;
    uint32_t sp, idx_stack[128], idx, best_idx;
    __m256 ps_t_aabb, ps_t_triangle, ps_t_min, valid;
    float t_min, t_values[8], t_stack[128], u, u_values[8], v, v_values[8];
    int valid_mask;

    ps_t_min = _mm256_set1_ps(FLT_MAX);
    t_min = FLT_MAX;
    sp = 0;
    idx_stack[sp] = 0;
    t_stack[sp] = 0.0;
    sp++;
    best_idx = -1;

    PackedRay packed_ray = {
        .o = ps_from_vec(ray->o),
        .v = ps_from_vec(ray->v),
        .inv_v = ps_from_vec(ray->inv_v),
    };

    __m256 zero = _mm256_setzero_ps();

    while (sp) {
        sp--;
        if (t_min < t_stack[sp]) continue;
        idx = idx_stack[sp];

        ps_t_aabb = packed_aabb_ray_intersection(&bvh->nodes[idx].bounds, &packed_ray);
        valid = _mm256_and_ps(
            _mm256_cmp_ps(ps_t_aabb, zero, _CMP_GE_OQ),
            _mm256_cmp_ps(ps_t_aabb, ps_t_min, _CMP_LT_OQ)
        );

        valid_mask = _mm256_movemask_ps(valid);

        for (int i = bvh->nodes[idx].internal_count; i < bvh->nodes[idx].internal_count + bvh->nodes[idx].leaf_count; i++) {
            if (!(valid_mask & (1 << i))) continue;
            ps_t_triangle = packed_triangle_ray_intersection(bvh->nodes[idx].idx[i], &packed_ray, &bvh->runtime_triangles, &info);

            _mm256_storeu_ps(t_values, ps_t_triangle);
            _mm256_storeu_ps(u_values, info.u);
            _mm256_storeu_ps(v_values, info.v);

            for (int lane = 0; lane < bvh->nodes[idx].primitive_count[i]; lane++) {
                if (t_values[lane] >= EPSILON && t_values[lane] < t_min) {
                    t_min = t_values[lane];
                    u = u_values[lane];
                    v = v_values[lane];
                    ps_t_min = _mm256_set1_ps(t_min);
                    best_idx = bvh->nodes[idx].idx[i] + lane;
                }
            }
        }

        _mm256_storeu_ps(t_values, ps_t_aabb);
        
        int count = 0;

        for (int i = 0; i < bvh->nodes[idx].internal_count; i++) {
            if (valid_mask & (1 << i)) {
                idx_stack[sp + count] = bvh->nodes[idx].idx[i];
                t_stack[sp + count] = t_values[i];
                count++;
            }
        }

        move_near_fw(idx_stack + sp, t_stack + sp, count);

        sp += count;
    }

    if (best_idx == -1) 
        return (HitResult){.t = -1.0};
    else
        return triangle_result(t_min, u, v, best_idx, ray, &bvh->runtime_triangles, &bvh->building_triangles, bvh->vertex_data, materials);
}

int is_shaded_by_object(Ray *ray, BVH8Tree *bvh) {
    PackedInfo info;
    uint32_t sp, idx_stack[128], idx;
    Object *object;
    __m256 ps_t_aabb, ps_t_triangle, valid;
    float t, t_values[8];
    int valid_mask;

    sp = 0;
    idx_stack[sp++] = 0;

    PackedRay packed_ray = {
        .o = ps_from_vec(ray->o),
        .v = ps_from_vec(ray->v),
        .inv_v = ps_from_vec(ray->inv_v),
    };

    __m256 zero = _mm256_setzero_ps();
    __m256 epsilon = _mm256_set1_ps(EPSILON);

    while (sp) {
        idx = idx_stack[--sp];

        ps_t_aabb = packed_aabb_ray_intersection(&bvh->nodes[idx].bounds, &packed_ray);
        valid = _mm256_cmp_ps(ps_t_aabb, zero, _CMP_GE_OQ);
        valid_mask = _mm256_movemask_ps(valid);

        for (int i = bvh->nodes[idx].internal_count; i < bvh->nodes[idx].internal_count + bvh->nodes[idx].leaf_count; i++) {
            if (!(valid_mask & (1 << i))) continue;

            ps_t_triangle = packed_triangle_ray_intersection(bvh->nodes[idx].idx[i], &packed_ray, &bvh->runtime_triangles, &info);
            _mm256_storeu_ps(t_values, ps_t_triangle);

            for (int lane = 0; lane < bvh->nodes[idx].primitive_count[i]; lane++) {
                if (t_values[lane] >= EPSILON) {
                    return 1;
                }
            }
        }

        for (int i = 0; i < bvh->nodes[idx].internal_count; i++) {
            if (valid_mask & (1 << i)) {
                idx_stack[sp++] = bvh->nodes[idx].idx[i];
            }
        }
    }

    return 0;
}



        // uint32_t leaf_begin = bvh->nodes[idx].internal_count;
        // uint32_t leaf_end = leaf_begin + bvh->nodes[idx].leaf_count;

        // for (uint32_t i = leaf_begin; i < leaf_end;) {
        //     if (!(valid_mask & (1u << i))) {
        //         i++;
        //         continue;
        //     }

        //     uint32_t start = bvh->nodes[idx].idx[i];
        //     uint32_t count = bvh->nodes[idx].primitive_count[i];
        //     i++;

        //     while (i < leaf_end && (valid_mask & (1u << i))) {
        //         count += bvh->nodes[idx].primitive_count[i];
        //         i++;
        //     }

        //     while (count > 0) {
        //         uint32_t len = count < 8 ? count : 8;

        //         ps_t_triangle = packed_triangle_ray_intersection(start, len, &packed_ray, &bvh->runtime_triangles, &info);
        //         triangle_test++;

        //         _mm256_storeu_ps(t_values, ps_t_triangle);
        //         _mm256_storeu_ps(u_values, info.u);
        //         _mm256_storeu_ps(v_values, info.v);

        //         for (int lane = 0; lane < len; lane++) {
        //             if (t_values[lane] >= EPSILON && t_values[lane] < t_min) {
        //                 t_min = t_values[lane];
        //                 u = u_values[lane];
        //                 v = v_values[lane];
        //                 ps_t_min = _mm256_set1_ps(t_min);
        //                 best_idx = start + lane;
        //             }
        //         }

        //         start += len;
        //         count -= len;

        //     }
        // }