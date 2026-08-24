#include "render/trace.h"
#include <float.h>
#include <stdio.h>

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
                t = object->get_ray_intersection(object, ray, &info);
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
                if (object->get_ray_intersection(object, ray, &info) > EPSILON) return 1;
            }
            continue;
        }

        idx_stack[sp++] = idx + 1;
        idx_stack[sp++] = bvh->nodes[idx].first_primitive_or_right_child;
    }

    return 0;
}