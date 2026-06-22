#include <stdlib.h>
#include <math.h>

#include "scene.h"
#include "graphics/light.h"
#include "graphics/trace.h"
#include <stdio.h>

Vec RIGHT = {.x = 1.0, .y = 0.0, .z = 0.0};
Vec FORWARD = {.x = 0.0, .y = 1.0, .z = 0.0};
Vec UP = {.x = 0.0, .y = 0.0, .z = 1.0};

Scene create_scene() {
    Scene scene;

    // scene.spheres = (Spheres){
    //     .sp = calloc(16, sizeof(Sphere)),
    //     .count = 0,
    //     .capacity = 16,
    // };

    scene.planes = (Planes){
        .pp = calloc(16, sizeof(Sphere)),
        .count = 0,
        .capacity = 16,
    };

    scene.objects = (Objects){
        .ptr = calloc(16, sizeof(Object)),
        .count = 0,
        .capacity = 16,
    };

    // scene.triangles = (Triangles){
    //     .tp = calloc(16, sizeof(Triangle)),
    //     .count = 0,
    //     .capacity = 16,
    // };

    // scene.boxes = (Boxes){
    //     .bp = calloc(16, sizeof(Box)),
    //     .count = 0,
    //     .capacity = 16,
    // };

    scene.dir_light = (DirectionalLight){
        .direction = normalize(vec(0.6, 0.6, -1.0)),
        .color = vec(1.0, 0.95, 0.9),
        .intensity = 1,
    };

    scene.global_ambient = vec(0.33, 0.33, 0.33);

    return scene;
}

Plane *add_plane(Scene *scene, Vec o, Vec n, Material *m) {
    Plane *ptr;

    if (scene->planes.count == scene->planes.capacity) {
        scene->planes.capacity += 16;
        scene->planes.pp = realloc(scene->planes.pp, scene->planes.capacity * sizeof(Plane));
    }

    if (dot(n, scene->dir_light.direction) > 0.0) n = neg(n);

    *((ptr = scene->planes.pp + scene->planes.count++)) = (Plane){
        .o = o,
        .n = n,
        .m = m
    };

    return ptr;
}

Object *add_object(Scene *scene) {
    if (scene->objects.count == scene->objects.capacity) {
        scene->objects.capacity += 16;
        scene->objects.ptr = realloc(scene->objects.ptr, scene->objects.capacity * sizeof(Object));
    }

    return scene->objects.ptr + scene->objects.count++;
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

    ptr->type.triangle.a = a;
    ptr->type.triangle.b = b;
    ptr->type.triangle.c = c;

    n = normalize(cross(v_sub(b, a), v_sub(c, a)));
    if (dot(n, scene->dir_light.direction) > 0.0) n = neg(n);
    ptr->type.triangle.n = n;

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


// Sphere *add_sphere(Scene *scene, Vec o, double r, Material *m) {
//     Sphere *ptr;

//     if (scene->spheres.count == scene->spheres.capacity) {
//         scene->spheres.capacity += 16;
//         scene->spheres.sp = realloc(scene->spheres.sp, scene->spheres.capacity * sizeof(Sphere));
//     }


//     *((ptr = scene->spheres.sp + scene->spheres.count++)) = (Sphere){
//         .o = o,
//         .r = r,
//         .aabb = (AABB) {
//             .min = v_sub(o, vec(r, r, r)),
//             .max = v_add(o, vec(r, r, r))
//         },
//         .m = m
//     };

//     return ptr;
// }



// Triangle *add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *m) {
//     Triangle *ptr;
//     Vec n, min, max;

//     if (scene->triangles.count == scene->triangles.capacity) {
//         scene->triangles.capacity += 16;
//         scene->triangles.tp = realloc(scene->triangles.tp, scene->triangles.capacity * sizeof(Triangle));
//     }

//     n = normalize(cross(v_sub(b, a), v_sub(c, a)));
//     if (dot(n, scene->dir_light.direction) > 0.0) n = neg(n);

//     min = vec(
//         fmin(a.x, fmin(b.x, c.x)),
//         fmin(a.y, fmin(b.y, c.y)),
//         fmin(a.z, fmin(b.z, c.z))
//     );

//     max = vec(
//         fmax(a.x, fmax(b.x, c.x)),
//         fmax(a.y, fmax(b.y, c.y)),
//         fmax(a.z, fmax(b.z, c.z))
//     );

//     *((ptr = scene->triangles.tp + scene->triangles.count++)) = (Triangle){
//         .a = a,
//         .b = b,
//         .c = c,
//         .n = n,
//         .aabb = (AABB) {
//             .min = min,
//             .max = max
//         },
//         .m = m,
//     };

//     return ptr;
// }

// Box *add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *m) {
//     Box *box;
//     Vec r, x, y, z, z_guess;

//     if (scene->boxes.count == scene->boxes.capacity) {
//         scene->boxes.capacity += 16;
//         scene->boxes.bp = realloc(scene->boxes.bp, scene->boxes.capacity * sizeof(Box));
//     }
    
//     box = scene->boxes.bp + scene->boxes.count++;

//     ptr->type.box.center = position;
//     ptr->type.box.half_size = scale(size, 0.5);

//     x = normalize(rotate(RIGHT, rotation));
//     z_guess = normalize(rotate(UP, rotation));

//     y = normalize(cross(z_guess, x));
//     z = cross(x, y);

//     ptr->type.box.axes[0] = x;
//     ptr->type.box.axes[1] = y;
//     ptr->type.box.axes[2] = z;

//     r.x = fabs(ptr->type.box.axes[0].x) * ptr->type.box.half_size.x +
//         fabs(ptr->type.box.axes[1].x) * ptr->type.box.half_size.y +
//         fabs(ptr->type.box.axes[2].x) * ptr->type.box.half_size.z;

//     r.y = fabs(ptr->type.box.axes[0].y) * ptr->type.box.half_size.x +
//         fabs(ptr->type.box.axes[1].y) * ptr->type.box.half_size.y +
//         fabs(ptr->type.box.axes[2].y) * ptr->type.box.half_size.z;

//     r.z = fabs(ptr->type.box.axes[0].z) * ptr->type.box.half_size.x +
//         fabs(ptr->type.box.axes[1].z) * ptr->type.box.half_size.y +
//         fabs(ptr->type.box.axes[2].z) * ptr->type.box.half_size.z;

//     ptr->type.box.aabb.min = v_sub(ptr->type.box.center, r);
//     ptr->type.box.aabb.max = v_add(ptr->type.box.center, r);

//     ptr->type.box.m = m;

//     return box;
// }


// Rectangle add_rectangle(Scene *scene, Vec position, Vec rotation, Vec size, Material *m) {
//     Rectangle rect;
//     Vec a, b, c, d, y, x, ab, ad;
//     x = scale(vec(1.0, 0.0, 0.0), size.x);
//     y = scale(vec(0.0, 1.0, 0.0), size.y);
    
//     a = position;
//     b = v_add(position, x);
//     d = v_add(position, y);

//     ab = v_sub(b, a);
//     ad = v_sub(d, a);

//     ab = rotate(ab, rotation);
//     ad = rotate(ad, rotation);

//     b = v_add(a, ab);
//     d = v_add(a, ad);

//     c = v_add(a, v_add(ab, ad));

//     rect.abd = add_triangle(scene, a, b, d, m);
//     rect.bcd = add_triangle(scene, b, c, d, m);

//     return rect;
// }

// Rectangle add_rectangle_by_points(Scene *scene, Vec a, Vec b, Vec c, Vec d, Material *m) {
//     Rectangle rect;
    
//     rect.abd = add_triangle(scene, a, b, d, m);
//     rect.bcd = add_triangle(scene, b, c, d, m);

//     return rect;
// }

// Rectangle copy_rectangle(Scene *scene, Rectangle *rect) {
//     return (Rectangle) {
//         .abd = copy_triangle(scene, rect->abd),
//         .bcd = copy_triangle(scene, rect->bcd),
//     };
// }

// Box add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *m) {
//     Box box;
//     Rectangle base;
//     Vec a, b, c, d, e, f, g, h, up;

//     base = add_rectangle(scene, position, rotation, size, m);

//     up = scale(normalize(cross(v_sub(base.abd->b, base.abd->a), v_sub(base.abd->c, base.abd->a))), size.z);

//     a = base.abd->a;
//     b = base.abd->b;
//     c = base.bcd->b;
//     d = base.abd->c;

//     e = v_add(a, up);
//     f = v_add(b, up);
//     g = v_add(c, up);
//     h = v_add(d, up);

//     box.x_neg = add_rectangle_by_points(scene, e, a, d, h, m);
//     box.x_pos = add_rectangle_by_points(scene, f, b, c, g, m);
//     box.y_neg = add_rectangle_by_points(scene, e, a, b, f, m);
//     box.y_pos = add_rectangle_by_points(scene, h, d, c, g, m);
//     box.z_neg = base;
//     box.z_pos = add_rectangle_by_points(scene, e, f, g, h, m);

//     return box;
// }