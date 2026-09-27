#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "render/trace.h"
#include "render/render.h"
#include "scene/scene.h"

static inline int clampi(int x, int min, int max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

HitResult get_first_hit(Ray *ray, Scene *scene, BVH8Tree *bvh) {
    HitResult plane_result, object_result;

    plane_result = get_first_plane(ray, &scene->planes);
    object_result = get_first_object(ray, bvh, &scene->materials);

    if(plane_result.t < 0.0) return object_result;
    if(object_result.t < 0.0) return plane_result;

    return object_result.t < plane_result.t ? object_result : plane_result;
}

int is_shaded(Ray *ray, Scene *scene, BVH8Tree *bvh) {
    return  is_shaded_by_plane(ray, &scene->planes) ||
            is_shaded_by_object(ray, bvh);
}

Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, BVH8Tree *bvh, int depth) {
    Ray shadow_ray, reflection_ray;
    HitResult hit;
    Vec light_reflection, ray_reflection;
    Vec c, c_local, c_reflected, c_diffuse, c_specular, c_ambient, c_base;
    double intensity;
    int x, y, w, h, idx;
    unsigned char *ptr;


    hit = get_first_hit(ray, scene, bvh);

    if (hit.t < 0.0) {
        return vec(0.3, 0.3, 0.3);
    } else {
        double u = hit.d_u - floor(hit.d_u);
        double v = hit.d_v - floor(hit.d_v);

        Texture *base_color_texture = (Texture *)get_element(hit.material->base_color_map, &scene->textures);
        w = base_color_texture->w;
        h = base_color_texture->h;
        ptr = base_color_texture->ptr;
        x = clampi((int)(u * (w - 1)), 0, w - 1);
        y = clampi((int)(v * (h - 1)), 0, h - 1);
        idx = (y * w + x) * 3;
        c_base = vec(
            ptr[idx + 0] / 255.0,
            ptr[idx + 1] / 255.0,
            ptr[idx + 2] / 255.0
        );
        c_base = hadamard(c_base, hit.material->base_color_factor);

        Texture *normal_map = (Texture *)get_element(hit.material->normal_map, &scene->textures);
        w = normal_map->w;
        h = normal_map->h;
        ptr = normal_map->ptr;
        x = clampi((int)(u * (w - 1)), 0, w - 1);
        y = clampi((int)(v * (h - 1)), 0, h - 1);
        idx = (y * w + x) * 3;

        Vec nmap = vec(
            ptr[idx + 0] / 255.0 * 2 - 1.0f,
            ptr[idx + 1] / 255.0 * 2 - 1.0f,
            ptr[idx + 2] / 255.0 * 2 - 1.0f
        );
        nmap.x *= hit.material->normal_scale;
        nmap.y *= hit.material->normal_scale;

        Vec ns;
        if (hit.valid_tangent) {
            ns = normalize(
                v_add(
                    v_add(
                        scale(hit.tangent, nmap.x),
                        scale(hit.bitangent, nmap.y)
                    ),
                    scale(hit.ns, nmap.z)
                )
            );
        } else {
            ns = hit.ns;
        }

        shadow_ray = create_ray(v_add(hit.point, scale(hit.ng, EPSILON)), scene->dir_light.dir);
        
        intensity = fmax(0.0, dot(ns, scene->dir_light.dir));
        light_reflection = normalize(v_sub(scene->dir_light.dir, scale(ns, 2.0 * dot(ns, scene->dir_light.dir))));
        
        c_diffuse = 
        scale(
            hadamard(
                scene->dir_light.scaled_color, 
                c_base
            ),
            intensity
        );

        c_specular = vec(0.0, 0.0, 0.0);
        // scale(
        //     hadamard(
        //         scene->dir_light.scaled_color,
        //         hit.material->specular
        //     ), 
        //     pow(
        //         fmax(0.0, dot(normalize(v_sub(cam->position, hit.point)), light_reflection)), 
        //         hit.material->shininess
        //     )
        // );

        c_ambient = hadamard(scene->global_ambient, c_base);

        c_local = v_add(v_add(c_ambient, c_diffuse), c_specular);

        return c_local;

        // if (hit.material->reflectivity == 0.0 || depth == 0) return c_local;

        // ray_reflection = normalize(v_sub(ray->v, scale(hit.ng, 2 * dot(hit.ng, ray->v))));
        // reflection_ray = create_ray(v_add(hit.point, scale(hit.ng, EPSILON)), ray_reflection);
        // c_reflected = trace_ray(&reflection_ray, scene, cam, bvh, depth - 1);

        // c = v_add(scale(c_local, 1.0 - hit.material->reflectivity), scale(c_reflected, hit.material->reflectivity));

        return c;
    }
}
