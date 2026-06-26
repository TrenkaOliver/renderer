#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "render/trace.h"
#include "render/render.h"
#include "scene/scene.h"

HitResult get_first_plane(Ray *ray, Planes *planes) {
    double t, tc;
    size_t i;
    Plane *plane;

    t = INFINITY;
    plane = NULL;
    
    for (i = 0; i < planes->count; i++) {
        tc = plane_ray_intersection(planes->ptr + i, ray);
        if (tc >= 0.0 && tc < t) {
            plane = planes->ptr + i;
            t = tc;
        }
    }

    if (!plane) return (HitResult){.point = vec(0.0, 0.0, 0.0), .ng = vec(0.0, 0.0, 0.0), .t = -1.0};

    Vec p = v_add(ray->o, scale(ray->v, t));

    return (HitResult){.point = p, .ng = plane->n, .ns = plane->n, .t = t, .material = plane->m};
}

int is_shaded_by_plane(Ray *ray, Planes *planes) {
    size_t i;

    for (i = 0; i < planes->count; i++)
        if (plane_ray_intersection(planes->ptr + i, ray) > EPSILON) return 1;

    return 0;
}

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

HitResult get_first_hit(Ray *ray, Scene *scene, BVH *bvh) {
    HitResult plane_result, object_result;

    plane_result = get_first_plane(ray, &scene->planes);
    object_result = get_first_object(ray, bvh);

    if(plane_result.t < 0.0) return object_result;
    if(object_result.t < 0.0) return plane_result;

    return object_result.t < plane_result.t ? object_result : plane_result;
}

int is_shaded(Ray *ray, Scene *scene, BVH *bvh) {
    return  is_shaded_by_plane(ray, &scene->planes) ||
            is_shaded_by_object(ray, bvh);
}



Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, BVH *bvh, int depth) {
    Ray shadow_ray, reflection_ray;
    HitResult hit;
    Vec light_reflection, ray_reflection;
    Vec c, c_local, c_reflected, c_diffuse, c_specular, c_ambient;
    double intensity;

    hit = get_first_hit(ray, scene, bvh);

    if (hit.t < 0.0) {
        return vec(0.0, 0.0, 0.0);
    } else {
        shadow_ray = create_ray(v_add(hit.point, scale(hit.ng, EPSILON)), scene->dir_light.dir);
        
        intensity = dot(hit.ns, scene->dir_light.dir);
        light_reflection = normalize(v_sub(scene->dir_light.dir, scale(hit.ns, 2.0 * dot(hit.ns, scene->dir_light.dir))));

        if (is_shaded(&shadow_ray, scene, bvh) || intensity < 0.0) {
            c_diffuse = vec(0.0, 0.0, 0.0);
            c_specular = vec(0.0, 0.0, 0.0);
        }
        else {
            c_diffuse = 
            scale(
                hadamard(
                    scene->dir_light.scaled_color, 
                    hit.material->diffuse
                ),
                intensity
            );

            c_specular = 
            scale(
                hadamard(
                    scene->dir_light.scaled_color,
                    hit.material->specular
                ), 
                pow(
                    fmax(0.0, dot(normalize(v_sub(cam->position, hit.point)), light_reflection)), 
                    hit.material->shininess
                )
            );
        }

        c_ambient = hadamard(scene->global_ambient, hit.material->diffuse);

        c_local = v_add(v_add(c_ambient, c_diffuse), c_specular);

        if (hit.material->reflectivity == 0.0 || depth == 0) return c_local;

        ray_reflection = normalize(v_sub(ray->v, scale(hit.ng, 2 * dot(hit.ng, ray->v))));
        reflection_ray = create_ray(v_add(hit.point, scale(hit.ng, EPSILON)), ray_reflection);
        c_reflected = trace_ray(&reflection_ray, scene, cam, bvh, depth - 1);

        c = v_add(scale(c_local, 1.0 - hit.material->reflectivity), scale(c_reflected, hit.material->reflectivity));

        return c;
    }
}
