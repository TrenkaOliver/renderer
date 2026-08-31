#include "render/trace.h"
#include <float.h>
#include <stdio.h>
#include "math/packed_vec.h"
#include "math/packed_ray.h"

HitResult get_first_object(Ray *ray, BVH *bvh) {
    uint32_t sp, idx_stack[128], idx, i, near, far;
    double t_left, t_right, t_near, t_far, t, t_min;
    Object *object, *closest;
    Info info, closest_info;

    t_min = DBL_MAX;
    closest = NULL;
    sp = 0;
    idx_stack[sp++] = 0;

    if (aabb_ray_intersection(&bvh->nodes->aabb, ray) < 0.0) return (HitResult){.t = -1.0};

    while (sp) {
        idx = idx_stack[--sp];

        if (bvh->nodes[idx].primitive_count) {
            for (i = 0; i < bvh->nodes[idx].primitive_count; i++) {
                object = bvh->objects[bvh->nodes[idx].first_primitive_or_right_child + i];
                //t = object->get_ray_intersection(object, ray, &info);
                if (t >= 0.0 && t < t_min) {
                    closest = object;
                    closest_info = info;
                    t_min = t;
                }
            }
            continue;
        }

        t_left = aabb_ray_intersection(&bvh->nodes[idx + 1].aabb, ray);
        t_right = aabb_ray_intersection(&bvh->nodes[bvh->nodes[idx].first_primitive_or_right_child].aabb, ray);

        near = t_left < t_right ? idx + 1 : bvh->nodes[idx].first_primitive_or_right_child;
        far = t_left < t_right ? bvh->nodes[idx].first_primitive_or_right_child : idx + 1;
        t_near = t_left < t_right ? t_left : t_right;
        t_far = t_left < t_right ? t_right : t_left;

        if (t_far > 0.0 && t_far < t_min)
            idx_stack[sp++] = far;

        if (t_near > 0.0 && t_near < t_min)
            idx_stack[sp++] = near;

    }

    if (!closest)
        return (HitResult){.t = -1.0};
    else
        return closest->get_hit_result(ray, closest, &closest_info, t_min);
}

PackedHitResult packed_get_first_object(PackedRay *ray, BVH *bvh) {
    PackedInfo closest_info, info;
    uint32_t sp;
    uint32_t idx_stack[128];
    uint32_t idx;
    PackedHitResult result;
    
    __m256 zero = _mm256_setzero_ps();

    __m256 t, t_min, t_left, t_right;
    __m256 active = _mm256_cmp_ps(zero, zero, _CMP_EQ_OQ);
    
    
    t_min = _mm256_set1_ps(FLT_MAX);
    Object *closest[8] = {NULL};
    sp = 0;
    idx_stack[sp++] = 0;

    active = _mm256_and_ps(active, _mm256_cmp_ps(packed_aabb_ray_intersection(&bvh->nodes->aabb, ray), zero, _CMP_GE_OQ));

    while (sp) {
        idx = idx_stack[sp--];

        if (bvh->nodes[idx].primitive_count) {
            for (int i = 0; i < bvh->nodes[idx].primitive_count; i++) {
                Object *object = bvh->objects[bvh->nodes[idx].first_primitive_or_right_child + i];
                t = object->get_ray_intersection(object, ray, &info);
                __m256 valid = _mm256_and_ps(
                    _mm256_cmp_ps(t, zero, _CMP_GE_OQ),
                    _mm256_cmp_ps(t, t_min, _CMP_LT_OQ)
                );
                t_min = _mm256_blendv_ps(t_min, t, valid);
                closest_info.u = _mm256_blendv_ps(closest_info.u, info.u, valid);
                closest_info.v = _mm256_blendv_ps(closest_info.v, info.v, valid);
                closest_info.w = _mm256_blendv_ps(closest_info.w, info.w, valid);

                int mask = _mm256_movemask_ps(valid);
                for (int j = 0; j < 8; i ++) {
                    if (mask & (1 << i)) {
                        closest[i] = object;
                    }
                }
            }
        }

        t_left = packed_aabb_ray_intersection(&bvh->nodes[idx + 1].aabb, ray);
        t_right = packed_aabb_ray_intersection(&bvh->nodes[bvh->nodes[idx].first_primitive_or_right_child].aabb, ray);

        __m256 travel = _mm256_and_ps(
            _mm256_cmp_ps(t_left, zero, _CMP_GE_OQ),
            _mm256_cmp_ps(t_left, t_min, _CMP_LT_OQ)
        );

        if (_mm256_movemask_ps(travel)) {
            idx_stack[sp++] = idx + 1;
        }

        travel = _mm256_and_ps(
            _mm256_cmp_ps(t_right, zero, _CMP_GE_OQ),
            _mm256_cmp_ps(t_right, t_min, _CMP_LT_OQ)
        );

        if (_mm256_movemask_ps(travel)) {
            idx_stack[sp++] = bvh->nodes[idx].first_primitive_or_right_child;
        }
    }

    HitResult scalar_res;
    float o_x[8];
    float o_y[8];
    float o_z[8];
    float v_x[8];
    float v_y[8];
    float v_z[8];    
    float invv_x[8];
    float invv_y[8];
    float invv_z[8];

    _mm256_storeu_ps(o_x, ray->o.x);
    _mm256_storeu_ps(o_y, ray->o.y);
    _mm256_storeu_ps(o_z, ray->o.z);
    _mm256_storeu_ps(v_x, ray->v.x);
    _mm256_storeu_ps(v_y, ray->v.y);
    _mm256_storeu_ps(v_z, ray->v.z);
    _mm256_storeu_ps(invv_x, ray->inv_v.x);
    _mm256_storeu_ps(invv_y, ray->inv_v.y);
    _mm256_storeu_ps(invv_z, ray->inv_v.z);

    Ray rays[8];

    for (int i = 0; i < 8; i++) {
        rays[i] = (Ray) {
            .o = vec(o_x[i], o_y[i], o_z[i]),
            .v = vec(v_x[i], v_y[i], v_z[i]),
            .inv_v = vec(invv_x[i], invv_y[i], invv_z[i])
        };
    }

}

int is_shaded_by_object(Ray *ray, BVH *bvh) {
    uint32_t sp, idx_stack[128], idx, i;
    Object *object;
    Info info;

    sp = 0;
    idx_stack[sp++] = 0;

    while (sp) {
        idx = idx_stack[--sp];

        if (aabb_ray_intersection(&bvh->nodes[idx].aabb, ray) < 0.0) continue;

        if (bvh->nodes[idx].primitive_count) {
            for (i = 0; i < bvh->nodes[idx].primitive_count; i++) {
                object = bvh->objects[bvh->nodes[idx].first_primitive_or_right_child + i];
                //if (object->get_ray_intersection(object, ray, &info) > EPSILON) return 1;
            }
            continue;
        }

        idx_stack[sp++] = idx + 1;
        idx_stack[sp++] = bvh->nodes[idx].first_primitive_or_right_child;
    }

    return 0;
}