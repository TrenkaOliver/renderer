#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include "scene/scene.h"
#include "light/light.h"
#include "light/material.h"
#include "render/trace.h"

Vec RIGHT = {.x = 1.0, .y = 0.0, .z = 0.0};
Vec FORWARD = {.x = 0.0, .y = 1.0, .z = 0.0};
Vec UP = {.x = 0.0, .y = 0.0, .z = 1.0};

Scene create_scene() {
    Scene scene;

    scene.planes = (Planes){
        .ptr = calloc(16, sizeof(Sphere)),
        .count = 0,
        .capacity = 16,
    };

    scene.objects = (Objects){
        .ptr = calloc(128, sizeof(Object)),
        .count = 0,
        .capacity = 128,
    };

    scene.dir_light = (DirectionalLight){
        .dir = normalize(vec(0.0, 0.0, 1.0)),
        .color = vec(1.0, 0.95, 0.9),
        .intensity = 1,
    };

    scene.dir_light.neg_dir = neg(scene.dir_light.dir);
    scene.dir_light.scaled_color = scale(scene.dir_light.color, scene.dir_light.intensity);

    scene.global_ambient = vec(0.33, 0.33, 0.33);

    return scene;
}

Plane *add_plane(Scene *scene, Vec o, Vec n, Material *m) {
    Plane *ptr;

    if (scene->planes.count == scene->planes.capacity) {
        scene->planes.capacity += 16;
        scene->planes.ptr = realloc(scene->planes.ptr, scene->planes.capacity * sizeof(Plane));
    }

    *((ptr = scene->planes.ptr + scene->planes.count++)) = (Plane){
        .o = o,
        .n = n,
        .m = m
    };

    return ptr;
}

Object *add_object(Scene *scene) {
    if (scene->objects.count == scene->objects.capacity) {
        scene->objects.capacity *= 2;
        scene->objects.ptr = realloc(scene->objects.ptr, scene->objects.capacity * sizeof(Object));
    }

    return scene->objects.ptr + scene->objects.count++;
}

Mesh *add_mesh(Scene *scene) {
    if (scene->meshes.count == scene->meshes.capacity) {
        scene->meshes.capacity += 16;
        scene->meshes.ptr = realloc(scene->meshes.ptr, scene->meshes.capacity * sizeof(Mesh));
    }

    return scene->meshes.ptr + scene->meshes.count++;
}


Object *add_sphere(Scene *scene, Vec o, double r, Material *m) {
    Object *ptr;

    ptr = add_object(scene);

    ptr->type.sphere.o = o;
    ptr->type.sphere.r = r;

    ptr->aabb.min = v_sub(o, vec(r, r, r));
    ptr->aabb.max = v_add(o, vec(r, r, r));

    ptr->m = m;

    ptr->get_ray_intersection = sphere_ray_intersection;
    ptr->get_hit_result = get_sphere_result;

    return ptr;
}

Object *add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *m) {
    Object *ptr;
    Vec n;
    
    ptr = add_object(scene);

    ptr->type.triangle.const_normal = 1;
    ptr->type.triangle.has_texture = 0;

    ptr->type.triangle.a = a;
    ptr->type.triangle.b = b;
    ptr->type.triangle.c = c;

    ptr->type.triangle.na = vec(0.0, 0.0, 0.0);
    ptr->type.triangle.nb = vec(0.0, 0.0, 0.0);
    ptr->type.triangle.nc = vec(0.0, 0.0, 0.0);

    n = normalize(cross(v_sub(b, a), v_sub(c, a)));

    ptr->type.triangle.ng = n;

    ptr->aabb.min = vec(
        fmin(a.x, fmin(b.x, c.x)),
        fmin(a.y, fmin(b.y, c.y)),
        fmin(a.z, fmin(b.z, c.z))
    );

    ptr->aabb.max = vec(
        fmax(a.x, fmax(b.x, c.x)),
        fmax(a.y, fmax(b.y, c.y)),
        fmax(a.z, fmax(b.z, c.z))
    );

    ptr->m = m;

    ptr->get_ray_intersection = triangle_ray_intersection;
    ptr->get_hit_result = get_triangle_result;

    return ptr;
}

Object *add_triangle_ns(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Material *m) {
    Object *ptr;

    ptr = add_triangle(scene, a, b, c, m);

    ptr->type.triangle.const_normal = 0;

    ptr->type.triangle.na = na;
    ptr->type.triangle.nb = nb;
    ptr->type.triangle.nc = nc;

    return ptr;
}

Object *add_triangle_t(Scene *scene, Vec a, Vec b, Vec c, Vec ta, Vec tb, Vec tc, Material *m) {
    Object *ptr;

    ptr = add_triangle(scene, a, b, c, m);

    ptr->type.triangle.has_texture = 1;

    ptr->type.triangle.ta = ta;
    ptr->type.triangle.tb = tb;
    ptr->type.triangle.tc = tc;

    return ptr;
}

Object *add_triangle_complete(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Vec ta, Vec tb, Vec tc, Material *m) {
    Object *ptr;

    ptr = add_triangle(scene, a, b, c, m);

    ptr->type.triangle.const_normal = 0;
    ptr->type.triangle.has_texture = 1;

    ptr->type.triangle.na = na;
    ptr->type.triangle.nb = nb;
    ptr->type.triangle.nc = nc;

    ptr->type.triangle.ta = ta;
    ptr->type.triangle.tb = tb;
    ptr->type.triangle.tc = tc;

    return ptr;
}

Object *add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *m) {
    Object *ptr;
    Vec r, x, y, z, z_guess;

    ptr = add_object(scene);

    ptr->type.box.center = position;
    ptr->type.box.half_size = scale(size, 0.5);

    x = normalize(rotate(RIGHT, rotation));
    z_guess = normalize(rotate(UP, rotation));

    y = normalize(cross(z_guess, x));
    z = cross(x, y);
    
    ptr->type.box.axes[0] = x;
    ptr->type.box.axes[1] = y;
    ptr->type.box.axes[2] = z;


    r.x = fabs(ptr->type.box.axes[0].x) *  ptr->type.box.half_size.x +
        fabs(ptr->type.box.axes[1].x) * ptr->type.box.half_size.y +
        fabs(ptr->type.box.axes[2].x) * ptr->type.box.half_size.z;

    r.y = fabs(ptr->type.box.axes[0].y) * ptr->type.box.half_size.x +
        fabs(ptr->type.box.axes[1].y) * ptr->type.box.half_size.y +
        fabs(ptr->type.box.axes[2].y) * ptr->type.box.half_size.z;

    r.z = fabs(ptr->type.box.axes[0].z) * ptr->type.box.half_size.x +
        fabs(ptr->type.box.axes[1].z) * ptr->type.box.half_size.y +
        fabs(ptr->type.box.axes[2].z) * ptr->type.box.half_size.z;

    ptr->aabb.min = v_sub(ptr->type.box.center, r);
    ptr->aabb.max = v_add(ptr->type.box.center, r);

    ptr->m = m;

    ptr->get_ray_intersection = box_ray_intersection;
    ptr->get_hit_result = get_box_result;

    return ptr;
}

void add_element(VecDynArray *v_arr, Vec vertex) {
    if (v_arr->count == v_arr->capacity) {
        v_arr->capacity *= 2;
        v_arr->ptr = realloc(v_arr->ptr, v_arr->capacity * sizeof(Vec));
    }

    v_arr->ptr[v_arr->count++] = vertex;
}