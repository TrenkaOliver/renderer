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

double aabb_ray_intersection(AABB *aabb, Line *ray) {
    Vec inv_v, t0, t1, t_min, t_max;
    double t_enter, t_exit;

    inv_v = vec(1.0 / ray->v.x, 1.0 / ray->v.y, 1.0 / ray->v.z);

    t0 = hadamard(v_sub(aabb->min, ray->o), inv_v);
    t1 = hadamard(v_sub(aabb->max, ray->o), inv_v);

    t_min = vec(
        fmin(t0.x, t1.x),
        fmin(t0.y, t1.y),
        fmin(t0.z, t1.z)
    );

    t_max = vec(
        fmax(t0.x, t1.x),
        fmax(t0.y, t1.y),
        fmax(t0.z, t1.z)
    );

    t_enter = fmax(t_min.x, fmax(t_min.y, t_min.z));
    t_exit = fmin(t_max.x, fmin(t_max.y, t_max.z));

    if (t_exit < t_enter || t_exit < EPSILON)
        return NAN;
    else
        return t_enter >= 0.0 ? t_enter : t_exit;
}

double box_ray_intersection(Box *box, Line *ray) {
    int i;
    double t_min, t_max, t1, t2, e, f, h, temp;
    Vec p;

    p = v_sub(box->center, ray->o);

    t_min = -1e30;
    t_max = 1e30;

    for (i = 0; i < 3; i++) {
        e = dot(box->axes[i], p);
        f = dot(box->axes[i], ray->v);
        h = i == 0 ? box->half_size.x : i == 1 ? box->half_size.y : box->half_size.z;

        if (fabs(f) < EPSILON) {
            if (fabs(e) > h)
                return NAN;
            else
                continue;
        }

        t1 = (e - h) / f;
        t2 = (e + h) / f;

        if (t1 > t2) {
            temp = t1;
            t1 = t2;
            t2 = temp;
        }

        t_min = fmax(t1, t_min);
        t_max = fmin(t2, t_max);

        if (t_min > t_max) return NAN;
    }

    if (t_max < 0.0) return NAN;
    
    return t_min >= 0.0 ? t_min : t_max;
}


HitResult get_first_sphere(Line *ray, Spheres *spheres) {
    int i;
    double t, tc;
    Sphere *sphere, *cs;

    t = INFINITY;
    sphere = NULL;
    
    for (i = 0; i < spheres->count; i++) {
        cs = spheres->sp + i;
        if (isnan(aabb_ray_intersection(&cs->aabb, ray))) continue;
        tc = sphere_ray_intersection(cs, ray);
        if (tc < t) {
            sphere = cs;
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
    Triangle *triangle, *ct;

    t = INFINITY;
    triangle = NULL;
    
    for (i = 0; i < triangles->count; i++) {
        ct = triangles->tp + i;
        if (isnan(aabb_ray_intersection(&ct->aabb, ray))) continue;
        tc = triangle_ray_intersection(ct, ray);
        if (tc < t) {
            triangle = ct;
            t = tc;
        }
    }

    if (!triangle) return (HitResult){.point = vec(0.0, 0.0, 0.0), .normal = vec(0.0, 0.0, 0.0), .t = NAN};

    Vec p = v_add(ray->o, scale(ray->v, t));

    return (HitResult){.point = p, .normal = triangle->n, .t = t, .material = triangle->m};
}

HitResult get_first_box(Line *ray, Boxes *boxes) {
    int i, negate;
    double t, tc, d, d_min;
    Vec dist, p, n;
    Box *box, *cb;

    t = INFINITY;
    box = NULL;
    
    for (i = 0; i < boxes->count; i++) {
        cb = boxes->bp + i;
        if (isnan(aabb_ray_intersection(&cb->aabb, ray))) continue;
        tc = box_ray_intersection(cb, ray);
        if (tc < t) {
            box = cb;
            t = tc;
        }
    }
    
    if (!box) return (HitResult){.point = vec(0.0, 0.0, 0.0), .normal = vec(0.0, 0.0, 0.0), .t = NAN};
    
    p = v_add(ray->o, scale(ray->v, t));
    
    dist = v_sub(p, box->center);

    double x = dot(box->axes[0], dist);
    double y = dot(box->axes[1], dist);
    double z = dot(box->axes[2], dist);

    double ax = fabs(x) / box->half_size.x;
    double ay = fabs(y) / box->half_size.y;
    double az = fabs(z) / box->half_size.z;

    if (ax >= ay && ax >= az)
        n = (x > 0.0) ? box->axes[0] : neg(box->axes[0]);
    else if (ay >= az)
        n = (y > 0.0) ? box->axes[1] : neg(box->axes[1]);
    else
        n = (z > 0.0) ? box->axes[2] : neg(box->axes[2]);

    return (HitResult){.point = p, .normal = n, .t = t, .material = box->m};
}



HitResult get_first_hit(Line *ray, Scene *scene) {
    int i;
    double t_min;
    HitResult results[4], best;

    results[0] = get_first_sphere(ray, &scene->spheres);
    results[1] = get_first_plane(ray, &scene->planes);
    results[2] = get_first_triangle(ray, &scene->triangles);
    results[3] = get_first_box(ray, &scene->boxes);

    best = (HitResult){
        .t = NAN
    };

    t_min = INFINITY;

    for (i = 0; i < 4; i++) {
        if (!isnan(results[i].t) && results[i].t < t_min) {
            best = results[i];
            t_min = results[i].t;
        }
    }

    return best;
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
