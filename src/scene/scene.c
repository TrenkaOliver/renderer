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

    scene.planes = create_dyn_array(sizeof(Plane), 16);
    scene.objects = create_dyn_array(sizeof(Object), 1024);
    scene.meshes = create_dyn_array(sizeof(Mesh), 16);
    scene.materials = create_dyn_array(sizeof(Material), 16);
    scene.textures = create_dyn_array(1, 1024);

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

size_t add_plane(Scene *scene, Vec o, Vec n, Material *m) {
    size_t i = grow_dyn_array(&scene->planes);

    *(Plane *)get_element(i, &scene->planes) = (Plane){
        .o = o,
        .n = n,
        .m = m
    };

    return i;
}


size_t add_sphere(Scene *scene, Vec o, double r, Material *m) {
    size_t i = grow_dyn_array(&scene->objects);

    *(Object *)get_element(i, &scene->objects) = (Object){
        .type.sphere = {
            .o = o,
            .r = r
        },
        .aabb = {
            .min = v_sub(o, vec(r, r, r)),
            .max = v_add(o, vec(r, r, r))
        },
        .material = m,
        .get_ray_intersection = sphere_ray_intersection,
        .get_hit_result = get_sphere_result
    };

    return i;
}

size_t add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *m) {
    Object *ptr;
    size_t i;

    i = grow_dyn_array(&scene->objects);
    ptr = get_element(i, &scene->objects);

    *ptr = (Object) {
        .type.triangle = {
            .const_normal = 1,
            .has_texture = 0,
            .a = a,
            .b = b,
            .c = c,
            .ng = normalize(cross(v_sub(b, a), v_sub(c, a)))
        },
        .aabb = {
            .min = v_min(v_min(a, b), c),
            .max = v_max(v_max(a, b), c)
        },
        .material = m,
        .get_ray_intersection = triangle_ray_intersection,
        .get_hit_result = get_triangle_result
    };

    return i;
}

size_t add_triangle_ns(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Material *m) {
    Object *ptr;
    size_t i;

    i = add_triangle(scene, a, b, c, m);
    ptr = get_element(i, &scene->objects);

    ptr->type.triangle.const_normal = 0;

    ptr->type.triangle.na = na;
    ptr->type.triangle.nb = nb;
    ptr->type.triangle.nc = nc;

    return i;
}

size_t add_triangle_t(Scene *scene, Vec a, Vec b, Vec c, Vec ta, Vec tb, Vec tc, Material *m) {
    Object *ptr;
    size_t i;

    i = add_triangle(scene, a, b, c, m);
    ptr = get_element(i, &scene->objects);

    ptr->type.triangle.has_texture = 1;

    ptr->type.triangle.ta = ta;
    ptr->type.triangle.tb = tb;
    ptr->type.triangle.tc = tc;

    return i;
}

size_t add_triangle_complete(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Vec ta, Vec tb, Vec tc, Material *m) {
    Object *ptr;
    size_t i;

    i = add_triangle(scene, a, b, c, m);
    ptr = get_element(i, &scene->objects);


    ptr->type.triangle.const_normal = 0;
    ptr->type.triangle.has_texture = 1;

    ptr->type.triangle.na = na;
    ptr->type.triangle.nb = nb;
    ptr->type.triangle.nc = nc;

    ptr->type.triangle.ta = ta;
    ptr->type.triangle.tb = tb;
    ptr->type.triangle.tc = tc;

    return i;
}

size_t add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *m) {
    size_t i;
    Vec r, x, y, z, z_guess;
    Object *ptr;

    i = grow_dyn_array(&scene->objects);
    ptr = get_element(i, &scene->objects);

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

    ptr->material = m;

    ptr->get_ray_intersection = box_ray_intersection;
    ptr->get_hit_result = get_box_result;

    return i;
}