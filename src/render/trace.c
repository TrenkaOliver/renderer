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

HitResult get_first_object(Ray *ray, Objects *objects) {
    int i;
    double t, t_min;
    Object *object, *closest;
    
    t_min = INFINITY;
    closest = NULL;

    for (i = 0; i < objects->count; i++) {
        object = objects->ptr + i;
        if (isnan(aabb_ray_intersection(&object->aabb, ray))) continue;

        t = object->get_ray_intersection(object, ray);
        if (t < t_min) {
            closest = object;
            t_min = t;
        }
    }

    if (!closest)
        return (HitResult){.t = NAN};
    else
        return closest->get_hit_result(ray, closest, t_min);
}

HitResult get_first_hit(Ray *ray, Scene *scene) {
    HitResult plane_result, object_result;

    plane_result = get_first_plane(ray, &scene->planes);
    object_result = get_first_object(ray, &scene->objects);

    if(isnan(plane_result.t)) return object_result;
    if(isnan(object_result.t)) return plane_result;

    return object_result.t < plane_result.t ? object_result : plane_result;
}

Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, int depth) {
    Ray shadow_ray, reflection_ray;
    HitResult hit, shadow_hit;
    Vec light_reflection, ray_reflection;
    Vec c, c_local, c_reflected, c_diffuse, c_specular, c_ambient;
    double intensity;

    hit = get_first_hit(ray, scene);

    if (isnan(hit.t)) {
        return vec(0.0, 0.0, 0.0);
    } else {
        shadow_ray = create_ray(v_add(hit.point, scale(hit.normal, EPSILON)), neg(scene->dir_light.direction));
        shadow_hit = get_first_hit(&shadow_ray, scene);
        
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
        c_reflected = trace_ray(&reflection_ray, scene, cam, depth - 1);

        c = v_add(scale(c_local, 1.0 - hit.material->reflectivity), scale(c_reflected, hit.material->reflectivity));

        return c;
    }
}
