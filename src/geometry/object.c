#include "render/trace.h"
#include <float.h>
#include <stdio.h>ú
#include "render/render.h"

int counter = 0;

HitResult get_first_object(Ray *ray, BVH8Tree *bvh) {
    uint32_t sp, idx_stack[128], idx;
    Object *object, *closest;
    Info info, closest_info;
    __m256 ps_t, ps_t_min, valid;
    float t, t_min;
    int valid_mask;

    first_object_called++;

    ps_t_min = _mm256_set1_ps(FLT_MAX);
    t_min = FLT_MAX;
    closest = NULL;
    sp = 0;
    idx_stack[sp++] = 0;

    PackedRay packed_ray = {
        .o = ps_from_vec(ray->o),
        .v = ps_from_vec(ray->v),
        .inv_v = ps_from_vec(ray->inv_v),
    };

    __m256 zero = _mm256_setzero_ps();

    uint32_t nodes_visited = 0;

    while (sp) {
        nodes_visited++;
        idx = idx_stack[--sp];
        visited_nodes++;

        ps_t = packed_aabb_ray_intersection(&bvh->nodes[idx].bounds, &packed_ray);
        valid = _mm256_and_ps(
            _mm256_cmp_ps(ps_t, zero, _CMP_GE_OQ),
            _mm256_cmp_ps(ps_t, ps_t_min, _CMP_LT_OQ)
        );

        valid_mask = _mm256_movemask_ps(valid);

        for (int i = bvh->nodes[idx].internal_count; i < bvh->nodes[idx].internal_count + bvh->nodes[idx].leaf_count; i++) {
            if (!(valid_mask & (1 << i))) continue;
            for (int j = 0; j < bvh->nodes[idx].primitive_count[i]; j++) {
                primitive_tests++;
                object = bvh->objects[bvh->nodes[idx].idx[i] + j];
                t = triangle_ray_intersection(object, ray, &info);
                if (t >= 0.0 && t < t_min) {
                    closest = object;
                    closest_info = info;
                    t_min = t;
                    ps_t_min = _mm256_set1_ps(t_min);
                }
            } 
        }

        valid = _mm256_and_ps(
            valid,
            _mm256_cmp_ps(ps_t, ps_t_min, _CMP_LT_OQ)
        );

        valid_mask = _mm256_movemask_ps(valid);

        for (int i = 0; i < bvh->nodes[idx].internal_count; i++) {
            if (valid_mask & (1 << i)) {
                idx_stack[sp++] = bvh->nodes[idx].idx[i];
            }
        }
    }

    if (!closest)
        return (HitResult){.t = -1.0};
    else
        return get_triangle_result(ray, closest, &closest_info, t_min);
}

int is_shaded_by_object(Ray *ray, BVH8Tree *bvh) {
    uint32_t sp, idx_stack[128], idx;
    Object *object;
    Info info;    
    __m256 ps_t, valid;
    float t;
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

    s_first_object_called++;

    while (sp) {
        idx = idx_stack[--sp];
        s_visited_nodes++;

        ps_t = packed_aabb_ray_intersection(&bvh->nodes[idx].bounds, &packed_ray);
        valid = _mm256_cmp_ps(ps_t, zero, _CMP_GE_OQ);
        valid_mask = _mm256_movemask_ps(valid);

        for (int i = bvh->nodes[idx].internal_count; i < bvh->nodes[idx].internal_count + bvh->nodes[idx].leaf_count; i++) {
            if (!(valid_mask & (1 << i))) continue;
            for (int j = 0; j < bvh->nodes[idx].primitive_count[i]; j++) {
                object = bvh->objects[bvh->nodes[idx].idx[i] + j];
                t = triangle_ray_intersection(object, ray, &info);
                if (t >= EPSILON) {
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