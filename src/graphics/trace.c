#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "graphics/trace.h"
#include "scene.h"
#include "graphics/render.h"

double sphere_ray_intersection(Sphere *sphere, Line *ray) {
    double a, b, c, d, t1, t2, t;
    Vec m;

    m = v_sub(ray->o, sphere->o);

    a = dot(ray->v, ray->v);
    b = 2.0 * dot(m, ray->v);
    c = dot(m, m) - sphere->r * sphere->r;

    d = b * b - 4 * a * c;

    if (d < 0.0) return NAN;

    t1 = (-b + sqrt(d)) / (2.0 * a);
    t2 = (-b - sqrt(d)) / (2.0 * a);

    t = INFINITY;

    if (t1 > 0.0 && t1 < t) t = t1;
    if (t2 > 0.0 && t2 < t) t = t2;

    if (t == INFINITY) return NAN;

    return t;
}

double plane_ray_intersection(Plane *plane, Line *ray) {
    double denom, t;

    denom = dot(plane->n, ray->v);
    if (fabs(denom) < EPSILON) return NAN;
    
    t = dot(plane->n, v_sub(plane->o, ray->o)) / denom;
    return t < 0.0 ? NAN : t;
}

double triangle_ray_intersection(Triangle *triangle, Line *ray) {
    Vec ab, ac, ao;
    double denom, u, v, t;

    ab = v_sub(triangle->b, triangle->a);
    ac = v_sub(triangle->c, triangle->a);
    ao = v_sub(ray->o, triangle->a);

    denom = dot(ab, cross(ray->v, ac));
    if (fabs(denom) < EPSILON) return NAN;

    u = dot(ao, cross(ray->v, ac)) / denom;
    if (u < 0.0) return NAN;

    v = dot(ray->v, cross(ao, ab)) / denom;
    if (v < 0.0) return NAN;

    if (u + v > 1.0) return NAN;

    t = dot(ac, cross(ao, ab)) / denom;
    
    return t < 0.0 ? NAN : t;
}

HitResult get_first_sphere(Line *ray, Spheres *spheres) {
    int i;
    double t, tc;
    Sphere *sphere;

    t = INFINITY;
    sphere = NULL;
    
    for (i = 0; i < spheres->count; i++) {
        tc = sphere_ray_intersection(spheres->sp + i, ray);
        if (tc < t) {
            sphere = spheres->sp + i;
            t = tc;
        }
    }

    if (!sphere) return (HitResult){.point = vec(0.0, 0.0, 0.0), .normal = vec(0.0, 0.0, 0.0), .t = NAN};

    Vec p = v_add(ray->o, scale(ray->v, t));
    Vec n = normalize(v_sub(p, sphere->o));

    return (HitResult){.point = p, .normal = n, .t = t, .material = sphere->m};
}

HitResult get_first_plane(Line *ray, Planes *planes) {
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

HitResult get_first_triangle(Line *ray, Triangles *triangles) {
    int i;
    double t, tc;
    Triangle *triangle;

    t = INFINITY;
    triangle = NULL;
    
    for (i = 0; i < triangles->count; i++) {
        tc = triangle_ray_intersection(triangles->tp + i, ray);
        if (tc < t) {
            triangle = triangles->tp + i;
            t = tc;
        }
    }

    if (!triangle) return (HitResult){.point = vec(0.0, 0.0, 0.0), .normal = vec(0.0, 0.0, 0.0), .t = NAN};

    Vec p = v_add(ray->o, scale(ray->v, t));

    return (HitResult){.point = p, .normal = triangle->n, .t = t, .material = triangle->m};
}


HitResult get_first_hit(Line *ray, Scene *scene) {
    HitResult sphere_hit, plane_hit, triangle_hit, *smallest;

    sphere_hit = get_first_sphere(ray, &scene->spheres);
    plane_hit = get_first_plane(ray, &scene->planes);
    triangle_hit = get_first_triangle(ray, &scene->triangles);

    if (isnan(sphere_hit.t) || (!isnan(plane_hit.t) && plane_hit.t < sphere_hit.t))
        smallest = &plane_hit;
    else
        smallest = &sphere_hit;

    if (isnan(smallest->t) || (!isnan(triangle_hit.t) && triangle_hit.t < smallest->t))
        smallest = &triangle_hit;
    
    return *smallest;
}

Vec trace_ray(Line *ray, Scene *scene, Camera *cam, int depth) {
    Line shadow_ray, reflection_ray;
    HitResult hit, shadow_hit;
    Vec light_reflection, ray_reflection;
    Vec c, c_local, c_reflected, c_diffuse, c_specular, c_ambient;
    double intensity;

    hit = get_first_hit(ray, scene);

    if (isnan(hit.t)) {
        return vec(0.0, 0.0, 0.0);
    } else {
        shadow_ray = create_line(v_add(hit.point, scale(hit.normal, EPSILON)), neg(scene->dir_light.direction));
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
        reflection_ray = create_line(v_add(hit.point, scale(hit.normal, EPSILON)), ray_reflection);
        c_reflected = trace_ray(&reflection_ray, scene, cam, depth - 1);

        c = v_add(scale(c_local, 1.0 - hit.material->reflectivity), scale(c_reflected, hit.material->reflectivity));

        return c;
    }
}
