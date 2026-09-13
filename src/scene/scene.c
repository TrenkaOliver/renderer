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
    //scene.objects = create_dyn_array(sizeof(Object), 1024);
    scene.meshes = create_dyn_array(sizeof(Mesh), 16);
    scene.materials = create_dyn_array(sizeof(Material), 16);
    scene.textures = create_dyn_array(1, 1024);

    scene.vertex_count = 0;
    scene.vertex_capacity = 128;
    scene.triangle_count = 0;
    scene.triangle_capacity = 128;

    scene.vertices = (Vertex){
        .x = malloc(scene.vertex_capacity * sizeof(float)),
        .y = malloc(scene.vertex_capacity * sizeof(float)),
        .z = malloc(scene.vertex_capacity * sizeof(float)),
        .nx = malloc(scene.vertex_capacity * sizeof(float)),
        .ny = malloc(scene.vertex_capacity * sizeof(float)),
        .nz = malloc(scene.vertex_capacity * sizeof(float)),
        .u = malloc(scene.vertex_capacity * sizeof(float)),
        .v = malloc(scene.vertex_capacity * sizeof(float))
    };

    scene.triangles = (BuildingTriangle){
        .ai = malloc(scene.triangle_capacity * sizeof(uint32_t)),
        .bi = malloc(scene.triangle_capacity * sizeof(uint32_t)),
        .ci = malloc(scene.triangle_capacity * sizeof(uint32_t)),

        .nx = malloc(scene.triangle_capacity * sizeof(float)),
        .ny = malloc(scene.triangle_capacity * sizeof(float)),
        .nz = malloc(scene.triangle_capacity * sizeof(float)),

        .aabb = malloc(scene.triangle_capacity * sizeof(AABB)),
        .centroid = malloc(scene.triangle_capacity * 3 * sizeof(float)),
        .material = malloc(scene.triangle_capacity * sizeof(Material *))
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

size_t add_plane(Scene *scene, Vec o, Vec n, Material *m) {
    size_t i = grow_dyn_array(&scene->planes);

    *(Plane *)get_element(i, &scene->planes) = (Plane){
        .o = o,
        .n = n,
        .m = m
    };

    return i;
}


// size_t add_sphere(Scene *scene, Vec o, double r, Material *m) {
//     size_t i = grow_dyn_array(&scene->objects);
//     Object *ptr = (Object *)get_element(i, &scene->objects);

//     *ptr = (Object){
//         .type.sphere = {
//             .o = o,
//             .r = r
//         },
//         .aabb = {
//             .min = {o.x - r, o.y - r, o.z - r},
//             .max = {o.x + r, o.y + r, o.z + r}
//         },
//         .material = m,
//         .get_ray_intersection = sphere_ray_intersection,
//         .get_hit_result = get_sphere_result
//     };

//     Vec centroid = calc_centroid(ptr->aabb);
//     ptr->centroid[0] = centroid.x;
//     ptr->centroid[1] = centroid.y;
//     ptr->centroid[2] = centroid.z;


//     return i;
// }

size_t add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *m) {
    // Object *ptr;
    // size_t i;

    // i = grow_dyn_array(&scene->objects);
    // ptr = get_element(i, &scene->objects);

    // *ptr = (Object) {
    //     .type.triangle = {
    //         .const_normal = 1,
    //         .has_texture = 0,
    //         .a = a,
    //         .b = b,
    //         .c = c,
    //         .ng = normalize(cross(v_sub(b, a), v_sub(c, a)))
    //     },
    //     .aabb = {
    //         .min = {fminf(fminf(a.x, b.x), c.x), fminf(fminf(a.y, b.y), c.y), fminf(fminf(a.z, b.z), c.z)},
    //         .max = {fmaxf(fmaxf(a.x, b.x), c.x), fmaxf(fmaxf(a.y, b.y), c.y), fmaxf(fmaxf(a.z, b.z), c.z)}
    //     },
    //     .material = m,
    //     .get_ray_intersection = triangle_ray_intersection,
    //     .get_hit_result = get_triangle_result
    // };

    //Vec centroid = calc_centroid(ptr->aabb);
    // ptr->centroid[0] = centroid.x;
    // ptr->centroid[1] = centroid.y;
    // ptr->centroid[2] = centroid.z;

    // return i;

    scene->vertex_count += 3;
    scene->triangle_count += 1;

    if (scene->vertex_count > scene->vertex_capacity) {
        scene->vertex_capacity *= 2;
        scene->vertices.x = realloc(scene->vertices.x, scene->vertex_capacity * sizeof(float));
        scene->vertices.y = realloc(scene->vertices.y, scene->vertex_capacity * sizeof(float));
        scene->vertices.z = realloc(scene->vertices.z, scene->vertex_capacity * sizeof(float));
        scene->vertices.nx = realloc(scene->vertices.nx, scene->vertex_capacity * sizeof(float));
        scene->vertices.ny = realloc(scene->vertices.ny, scene->vertex_capacity * sizeof(float));
        scene->vertices.nz = realloc(scene->vertices.nz, scene->vertex_capacity * sizeof(float));
        scene->vertices.u = realloc(scene->vertices.u, scene->vertex_capacity * sizeof(float));
        scene->vertices.v = realloc(scene->vertices.v, scene->vertex_capacity * sizeof(float));
    }

    if (scene->triangle_count > scene->triangle_capacity) {
        scene->triangle_capacity *= 2;

        scene->triangles.ai = realloc(scene->triangles.ai, scene->triangle_capacity * sizeof(uint32_t));
        scene->triangles.bi = realloc(scene->triangles.bi, scene->triangle_capacity * sizeof(uint32_t));
        scene->triangles.ci = realloc(scene->triangles.ci, scene->triangle_capacity * sizeof(uint32_t));

        scene->triangles.nx = realloc(scene->triangles.nx, scene->triangle_capacity * sizeof(float));
        scene->triangles.ny = realloc(scene->triangles.ny, scene->triangle_capacity * sizeof(float));
        scene->triangles.nz = realloc(scene->triangles.nz, scene->triangle_capacity * sizeof(float));

        scene->triangles.aabb = realloc(scene->triangles.aabb, scene->triangle_capacity * sizeof(AABB));
        scene->triangles.centroid = realloc(scene->triangles.centroid, scene->triangle_capacity * 3 * sizeof(float));
        scene->triangles.material = realloc(scene->triangles.material, scene->triangle_capacity * sizeof(Material *));
    }

    scene->vertices.x[scene->vertex_count - 3] = a.x;
    scene->vertices.y[scene->vertex_count - 3] = a.y;
    scene->vertices.z[scene->vertex_count - 3] = a.z;

    scene->vertices.x[scene->vertex_count - 2] = b.x;
    scene->vertices.y[scene->vertex_count - 2] = b.y;
    scene->vertices.z[scene->vertex_count - 2] = b.z;

    scene->vertices.x[scene->vertex_count - 1] = c.x;
    scene->vertices.y[scene->vertex_count - 1] = c.y;
    scene->vertices.z[scene->vertex_count - 1] = c.z;

    scene->triangles.ai[scene->triangle_count - 1] = scene->vertex_count - 3;
    scene->triangles.bi[scene->triangle_count - 1] = scene->vertex_count - 2;
    scene->triangles.ci[scene->triangle_count - 1] = scene->vertex_count - 1;

    scene->triangles.nx[scene->triangle_count - 1] = normalize(cross(v_sub(b, a), v_sub(c, a))).x;
    scene->triangles.ny[scene->triangle_count - 1] = normalize(cross(v_sub(b, a), v_sub(c, a))).y;
    scene->triangles.nz[scene->triangle_count - 1] = normalize(cross(v_sub(b, a), v_sub(c, a))).z;

    scene->triangles.aabb[scene->triangle_count - 1] = (AABB){
        .min = {fminf(fminf(a.x, b.x), c.x), fminf(fminf(a.y, b.y), c.y), fminf(fminf(a.z, b.z), c.z)},
        .max = {fmaxf(fmaxf(a.x, b.x), c.x), fmaxf(fmaxf(a.y, b.y), c.y), fmaxf(fmaxf(a.z, b.z), c.z)}
    };

    scene->triangles.centroid[(scene->triangle_count - 1) * 3 + 0] = (float)(scene->triangles.aabb[scene->triangle_count - 1].min[0] + scene->triangles.aabb[scene->triangle_count - 1].max[0]) / 2.0f;
    scene->triangles.centroid[(scene->triangle_count - 1) * 3 + 1] = (float)(scene->triangles.aabb[scene->triangle_count - 1].min[1] + scene->triangles.aabb[scene->triangle_count - 1].max[1]) / 2.0f;
    scene->triangles.centroid[(scene->triangle_count - 1) * 3 + 2] = (float)(scene->triangles.aabb[scene->triangle_count - 1].min[2] + scene->triangles.aabb[scene->triangle_count - 1].max[2]) / 2.0f;

    scene->triangles.material[scene->triangle_count - 1] = m;

    return scene->triangle_count - 1;
}

// size_t add_triangle_ns(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Material *m) {
//     Object *ptr;
//     size_t i;

//     i = add_triangle(scene, a, b, c, m);
//     //ptr = get_element(i, &scene->objects);

//     ptr->type.triangle.const_normal = 0;

//     ptr->type.triangle.na = na;
//     ptr->type.triangle.nb = nb;
//     ptr->type.triangle.nc = nc;

//     return i;
// }

// size_t add_triangle_t(Scene *scene, Vec a, Vec b, Vec c, Vec ta, Vec tb, Vec tc, Material *m) {
//     Object *ptr;
//     size_t i;

//     i = add_triangle(scene, a, b, c, m);
//     //ptr = get_element(i, &scene->objects);

//     ptr->type.triangle.has_texture = 1;

//     ptr->type.triangle.ta = ta;
//     ptr->type.triangle.tb = tb;
//     ptr->type.triangle.tc = tc;

//     return i;
// }

// size_t add_triangle_complete(Scene *scene, Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Vec ta, Vec tb, Vec tc, Material *m) {
//     Object *ptr;
//     size_t i;

//     i = add_triangle(scene, a, b, c, m);
//     ptr = get_element(i, &scene->objects);


//     ptr->type.triangle.const_normal = 0;
//     ptr->type.triangle.has_texture = 1;

//     ptr->type.triangle.na = na;
//     ptr->type.triangle.nb = nb;
//     ptr->type.triangle.nc = nc;

//     ptr->type.triangle.ta = ta;
//     ptr->type.triangle.tb = tb;
//     ptr->type.triangle.tc = tc;

//     return i;
// }

// size_t add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *m) {
//     size_t i;
//     Vec r, x, y, z, z_guess;
//     Object *ptr;

//     i = grow_dyn_array(&scene->objects);
//     ptr = get_element(i, &scene->objects);

//     ptr->type.box.center = position;
//     ptr->type.box.half_size = scale(size, 0.5);

//     x = normalize(rotate(RIGHT, rotation));
//     z_guess = normalize(rotate(UP, rotation));

//     y = normalize(cross(z_guess, x));
//     z = cross(x, y);
    
//     ptr->type.box.axes[0] = x;
//     ptr->type.box.axes[1] = y;
//     ptr->type.box.axes[2] = z;


//     r.x = fabs(ptr->type.box.axes[0].x) *  ptr->type.box.half_size.x +
//         fabs(ptr->type.box.axes[1].x) * ptr->type.box.half_size.y +
//         fabs(ptr->type.box.axes[2].x) * ptr->type.box.half_size.z;

//     r.y = fabs(ptr->type.box.axes[0].y) * ptr->type.box.half_size.x +
//         fabs(ptr->type.box.axes[1].y) * ptr->type.box.half_size.y +
//         fabs(ptr->type.box.axes[2].y) * ptr->type.box.half_size.z;

//     r.z = fabs(ptr->type.box.axes[0].z) * ptr->type.box.half_size.x +
//         fabs(ptr->type.box.axes[1].z) * ptr->type.box.half_size.y +
//         fabs(ptr->type.box.axes[2].z) * ptr->type.box.half_size.z;

//     ptr->aabb = (AABB){
//         .min = {position.x - r.x, position.y - r.y, position.z - r.z},
//         .max = {position.x + r.x, position.y + r.y, position.z + r.z}
//     };

//     Vec centroid = calc_centroid(ptr->aabb);
//     ptr->centroid[0] = centroid.x;
//     ptr->centroid[1] = centroid.y;
//     ptr->centroid[2] = centroid.z;

//     ptr->material = m;

//     ptr->get_ray_intersection = box_ray_intersection;
//     ptr->get_hit_result = get_box_result;

//     return i;
// }