#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "render/trace.h"
#include "render/render.h"
#include "scene/scene.h"

HitResult get_first_plane(Ray *ray, Planes *planes) {
    int i;
    double t, tc;
    Plane *plane;

    t = INFINITY;
    plane = NULL;
    
    for (i = 0; i < planes->count; i++) {
        tc = plane_ray_intersection(planes->pp + i, ray);
        if (tc < t) {
            plane = planes->pp + i;
            t = tc;
        }
    }

    if (!plane) return (HitResult){.point = vec(0.0, 0.0, 0.0), .normal = vec(0.0, 0.0, 0.0), .t = NAN};

    Vec p = v_add(ray->o, scale(ray->v, t));

    return (HitResult){.point = p, .normal = plane->n, .t = t, .material = plane->m};
}

HitResult get_first_object(Ray *ray, Objects *objects, BVH *bvh) {
    int sp, i;
    double t_left, t_right, t_near, t_far, t, t_min;
    Object *object, *closest;
    BVHNode *stack[128], *n, *near, *far;

    t_min = INFINITY;
    closest = NULL;
    sp = 0;
    stack[sp++] = bvh->root;

    while (sp) {
        n = stack[--sp];

        t = aabb_ray_intersection(&n->aabb, ray);
        if (isnan(t) || t > t_min) continue;

        if (n->first_primitive) {
            for (i = 0; i < n->primitive_count; i++) {
                object = n->first_primitive[i];
                t = object->get_ray_intersection(object, ray);
                if (t < t_min) {
                    closest = object;
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

        if (t_far < t_min)
            stack[sp++] = far;

        if (t_near < t_min)
            stack[sp++] = near;

    }

    if (!closest)
        return (HitResult){.t = NAN};
    else
        return closest->get_hit_result(ray, closest, t_min);
}

HitResult get_first_hit(Ray *ray, Scene *scene, BVH *bvh) {
    HitResult plane_result, object_result;

    plane_result = get_first_plane(ray, &scene->planes);
    object_result = get_first_object(ray, &scene->objects, bvh);

    if(isnan(plane_result.t)) return object_result;
    if(isnan(object_result.t)) return plane_result;

    return object_result.t < plane_result.t ? object_result : plane_result;
}

Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, BVH *bvh, int depth) {
    Ray shadow_ray, reflection_ray;
    HitResult hit, shadow_hit;
    Vec light_reflection, ray_reflection;
    Vec c, c_local, c_reflected, c_diffuse, c_specular, c_ambient;
    double intensity;

    hit = get_first_hit(ray, scene, bvh);

    if (isnan(hit.t)) {
        return vec(0.0, 0.0, 0.0);
    } else {
        shadow_ray = create_ray(v_add(hit.point, scale(hit.normal, EPSILON)), neg(scene->dir_light.direction));
        shadow_hit = get_first_hit(&shadow_ray, scene, bvh);
        
        intensity = dot(hit.normal, neg(scene->dir_light.direction));
        light_reflection = normalize(v_sub(scene->dir_light.direction, scale(hit.normal, 2 * dot(hit.normal, scene->dir_light.direction))));

        if (!isnan(shadow_hit.t) || intensity < 0) {
            c_diffuse = vec(0.0, 0.0, 0.0);
            c_specular = vec(0.0, 0.0, 0.0);
        }
        else {
            c_diffuse = 
            scale(
                hadamard(
                    scale(scene->dir_light.color, scene->dir_light.intensity), 
                    hit.material->diffuse
                ),
                intensity
            );

            c_specular = 
            scale(
                hadamard(
                    scale(scene->dir_light.color, scene->dir_light.intensity),
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

        ray_reflection = normalize(v_sub(ray->v, scale(hit.normal, 2 * dot(hit.normal, ray->v))));
        reflection_ray = create_ray(v_add(hit.point, scale(hit.normal, EPSILON)), ray_reflection);
        c_reflected = trace_ray(&reflection_ray, scene, cam, bvh, depth - 1);

        c = v_add(scale(c_local, 1.0 - hit.material->reflectivity), scale(c_reflected, hit.material->reflectivity));

        return c;
    }
}
