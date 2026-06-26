#include "render/trace.h"

HitResult get_first_object(Ray *ray, BVH *bvh) {
    int sp;
    size_t i;
    double t_left, t_right, t_near, t_far, t, t_min;
    Object *object, *closest;
    Info info, closest_info;
    BVHNode *stack[128], *n, *near, *far;

    t_min = INFINITY;
    closest = NULL;
    sp = 0;
    stack[sp++] = bvh->root;

    if (aabb_ray_intersection(&bvh->root->aabb, ray) < 0.0) return (HitResult){.t = -1.0};

    while (sp) {
        n = stack[--sp];

        if (n->first_primitive) {
            for (i = 0; i < n->primitive_count; i++) {
                object = n->first_primitive[i];
                t = object->get_ray_intersection(object, ray, &info);
                if (t >= 0.0 && t < t_min) {
                    closest = object;
                    closest_info = info;
                    t_min = t;
                }
            }
            continue;
        }

        t_left = aabb_ray_intersection(&n->left->aabb, ray);
        t_right = aabb_ray_intersection(&n->right->aabb, ray);
        
        if (t_left < t_right) {
            near = n->left;
            far = n->right;
            t_near = t_left;
            t_far = t_right;
        } else {
            near = n->right;
            far = n->left;
            t_near = t_right;
            t_far = t_left;
        }

        if (t_far > 0.0 && t_far < t_min)
            stack[sp++] = far;

        if (t_near > 0.0 && t_near < t_min)
            stack[sp++] = near;

    }

    if (!closest)
        return (HitResult){.t = -1.0};
    else
        return closest->get_hit_result(ray, closest, &closest_info, t_min);
}

int is_shaded_by_object(Ray *ray, BVH *bvh) {
    int sp;
    size_t i;
    Object *object;
    Info info;
    BVHNode *stack[128], *n;

    sp = 0;
    stack[sp++] = bvh->root;

    while (sp) {
        n = stack[--sp];

        if (aabb_ray_intersection(&n->aabb, ray) < 0.0) continue;

        if (n->first_primitive) {
            for (i = 0; i < n->primitive_count; i++) {
                object = n->first_primitive[i];
                if (object->get_ray_intersection(object, ray, &info) > EPSILON) return 1;
            }
            continue;
        }

        stack[sp++] = n->left;
        stack[sp++] = n->right;
    }

    return 0;
}