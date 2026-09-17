#include "render/trace.h"
#include <float.h>
#include <stdio.h>
#include "render/render.h"

static inline void move_near_fw(uint32_t idx[], float t[], int count) {
    for (int iter = 0; iter < 2; iter++) {
        for (int i = 1; i < count; i++) {
            if (t[i] > t[i - 1]) {
                float t_tmp = t[i - 1];
                t[i - 1] = t[i];
                t[i] = t_tmp;

                uint32_t idx_tmp = idx[i - 1];
                idx[i - 1] = idx[i];
                idx[i] = idx_tmp;
            }
        }
    }
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
            ps_t_triangle = packed_triangle_ray_intersection(bvh->nodes[idx].idx[i], bvh->nodes[idx].primitive_count[i], &packed_ray, &bvh->runtime_triangles, &info);

            _mm256_storeu_ps(t_values, ps_t_triangle);
            _mm256_storeu_ps(u_values, info.u);
            _mm256_storeu_ps(v_values, info.v);

            for (int lane = 0; lane < bvh->nodes[idx].primitive_count[i]; lane++) {
                if (t_values[lane] >= 0.0f && t_values[lane] < t_min) {
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
        return triangle_result(t_min, u, v, best_idx, ray, &bvh->runtime_triangles, &bvh->building_triangles, bvh->vertices, materials);
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

            ps_t_triangle = packed_triangle_ray_intersection(bvh->nodes[idx].idx[i], bvh->nodes[idx].primitive_count[i], &packed_ray, &bvh->runtime_triangles, &info);
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