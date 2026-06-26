#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "render/trace.h"
#include "render/render.h"
#include "scene/scene.h"

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
