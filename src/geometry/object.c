#include "render/trace.h"
#include <float.h>
#include <stdio.h>

HitResult get_first_object(Ray *ray, BVH8Tree *bvh) {
    uint32_t sp, idx_stack[128], idx;
    float t, t_min;
    Object *object, *closest;
    Info info, closest_info;

    t_min = FLT_MAX;
    closest = NULL;
    sp = 0;
    idx_stack[sp++] = 0;

    while (sp) {
        idx = idx_stack[--sp];

        for (int i = bvh->nodes[idx].internal_count; i < bvh->nodes[idx].internal_count + bvh->nodes[idx].leaf_count; i++) {
            t = aabb_ray_intersection(&bvh->nodes[idx].bounds[i], ray);
            if (t < 0.0 || t > t_min) continue;

            for (int j = 0; j < bvh->nodes[idx].primitive_count[i]; j++) {
                object = bvh->objects[bvh->nodes[idx].idx[i] + j];
                t = object->get_ray_intersection(object, ray, &info);
                if (t >= 0.0 && t < t_min) {
                    closest = object;
                    closest_info = info;
                    t_min = t;
                }
            }
        }

        for (int i = 0; i < bvh->nodes[idx].internal_count; i++) {
            t = aabb_ray_intersection(&bvh->nodes[idx].bounds[i], ray);
            if (t >= 0.0 && t < t_min) {
                idx_stack[sp++] = bvh->nodes[idx].idx[i];
            }
        }

    }

    if (!closest)
        return (HitResult){.t = -1.0};
    else
        return closest->get_hit_result(ray, closest, &closest_info, t_min);
}

int is_shaded_by_object(Ray *ray, BVH8Tree *bvh) {
    uint32_t sp, idx_stack[128], idx;
    Object *object;
    Info info;
    float t;

    sp = 0;
    idx_stack[sp++] = 0;

    while (sp) {
        idx = idx_stack[--sp];

        for (int i = bvh->nodes[idx].internal_count; i < bvh->nodes[idx].internal_count + bvh->nodes[idx].leaf_count; i++) {
            t = aabb_ray_intersection(&bvh->nodes[idx].bounds[i], ray);
            if (t < 0.0) continue;

            for (int j = 0; j < bvh->nodes[idx].primitive_count[i]; j++) {
                object = bvh->objects[bvh->nodes[idx].idx[i] + j];
                t = object->get_ray_intersection(object, ray, &info);

                if (t >= EPSILON) return 1;
            }
        }

        for (int i = 0; i < bvh->nodes[idx].internal_count; i++) {
            t = aabb_ray_intersection(&bvh->nodes[idx].bounds[i], ray);
            if (t >= 0.0) {
                idx_stack[sp++] = bvh->nodes[idx].idx[i];
            }
        }
    }

    return 0;
}