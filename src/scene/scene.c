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

Material no_texture = {
    .diffuse = {.x = 1.0, .y = 1.0, .z = 1.0},
    .specular = {.x = 0.0, .y = 0.0, .z = 0.0},
    .shininess = 1,
    .reflectivity = 0
};

Material skin = {
    .diffuse = {.x = 0.76, .y = 0.58, .z = 0.50},
    .specular = {.x = 0.0, .y = 0.0, .z = 0.0},
    .shininess = 1,
    .reflectivity = 0,
};

Scene create_scene() {
    Scene scene;

    scene.planes = (Planes){
        .ptr = calloc(16, sizeof(Sphere)),
        .count = 0,
        .capacity = 16,
    };

    scene.objects = (Objects){
        .ptr = calloc(16, sizeof(Object)),
        .count = 0,
        .capacity = 16,
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

    //if (dot(n, scene->dir_light.dir) > 0.0) n = neg(n);

    *((ptr = scene->planes.ptr + scene->planes.count++)) = (Plane){
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

    ptr->type.triangle.a = a;
    ptr->type.triangle.b = b;
    ptr->type.triangle.c = c;

    ptr->type.triangle.na = vec(0.0, 0.0, 0.0);
    ptr->type.triangle.nb = vec(0.0, 0.0, 0.0);
    ptr->type.triangle.nc = vec(0.0, 0.0, 0.0);

    n = normalize(cross(v_sub(b, a), v_sub(c, a)));
    // if (dot(n, scene->dir_light.dir) > 0.0) n = neg(n);
    // ptr->type.triangle.ng = n;

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


Mesh *inport_mesh(Scene *scene, char *file_name) {
    FILE *f;
    VecDynArray v_arr, vt_arr, vn_arr;
    Mesh *mesh;
    Object *t;
    AABB aabb;
    Vec _v;
    Face idx[64];
    size_t count;
    char line[128], *p;
    long v, vn, vt, i;
    
    f = fopen(file_name, "r");
    v_arr = (VecDynArray){
        .ptr = calloc(64, sizeof(Vec)),
        .capacity = 64,
        .count = 0,
    };

    vt_arr = (VecDynArray){
        .ptr = calloc(64, sizeof(Vec)),
        .capacity = 64,
        .count = 0,
    };

    vn_arr = (VecDynArray){
        .ptr = calloc(64, sizeof(Vec)),
        .capacity = 64,
        .count = 0
    };
    
    mesh = add_mesh(scene);
    mesh->rotation = vec(0.0, 0.0, 0.0);
    mesh->first_triangle = scene->objects.count;
    mesh->triangle_count = 0;

    aabb = (AABB){.min = vec(INFINITY, INFINITY, INFINITY), .max = vec(-INFINITY, -INFINITY, -INFINITY)};

    while (fgets(line, 128, f)){
        if (sscanf(line, "v %lf %lf %lf", &_v.x, &_v.y, &_v.z) == 3) {
            add_element(&v_arr, _v);
        } else if (sscanf(line, "vn %lf %lf %lf", &_v.x, &_v.y, &_v.z) == 3) {
            add_element(&vn_arr, normalize(_v));
        } else if (line[0] == 'f' && line[1] == ' ') {
            p = line + 2;
            count = 0;

            while(*p) {
                while (isspace(*p)) p++;
                if (!*p) break;

                if (sscanf(p, "%ld/%ld/%ld", &v, &vt, &vn) == 3) {
                    idx[count++] = (Face){
                        .v = (v < 0) ? (size_t)(v_arr.count + v) : (size_t)(v - 1),
                        .vn = (vn < 0) ? (size_t)(vn_arr.count + vn) : (size_t)(vn - 1)
                    };
                } else if (sscanf(p, "%ld//%ld", &v, &vn) == 2) {
                    idx[count++] = (Face){
                        .v = (v < 0) ? (size_t)(v_arr.count + v) : (size_t)(v - 1),
                        .vn = (vn < 0) ? (size_t)(vn_arr.count + vn) : (size_t)(vn - 1)
                    };
                } else if (sscanf(p, "%ld/%ld", &v, &vt) == 2) {
                    idx[count++] = (Face){
                        .v = (v < 0) ? (size_t)(v_arr.count + v) : (size_t)(v - 1),
                        .vn = (size_t)-1
                    };
                } else if (sscanf(p, "%ld", &v) == 1) {
                    idx[count++] = (Face){
                        .v = (v < 0) ? (size_t)(v_arr.count + v) : (size_t)(v - 1),
                        .vn = (size_t)-1
                    };
                }

                while(*p && !isspace(*p)) p++;
                
                if (count >= 64) break;
            }

            if (count < 3) continue;

            for (i = 1; i < count - 1; i++) {
                //printf("%f, %f, %f\n", vn_arr.ptr[idx[i].vn].x, vn_arr.ptr[idx[i].vn].y, vn_arr.ptr[idx[i].vn].z);
                t = add_triangle_ns(
                    scene,
                    v_arr.ptr[idx[0].v],
                    v_arr.ptr[idx[i].v],
                    v_arr.ptr[idx[i + 1].v],
                    idx[0].vn != (size_t)-1 ? vn_arr.ptr[idx[0].vn] : vec(0.0, 0.0, 0.0),
                    idx[i].vn != (size_t)-1 ? vn_arr.ptr[idx[i].vn] : vec(0.0, 0.0, 0.0),
                    idx[i + 1].vn != (size_t)-1 ? vn_arr.ptr[idx[i + 1].vn] : vec(0.0, 0.0, 0.0),
                    &skin
                );
                aabb = aabb_merge(aabb, t->aabb);
                mesh->triangle_count++;
            }
        }
    }

    mesh->position = aabb.min;
    mesh->size = v_sub(aabb.max, aabb.min);
    mesh->aabb = aabb;
    
    free(v_arr.ptr);
    free(vt_arr.ptr);
    free(vn_arr.ptr);
    return mesh;
}

void move_mesh(Scene *scene, Mesh *mesh, Vec delta) {
    size_t i, end;
    
    mesh->position = v_add(mesh->position, delta);
    end = mesh->first_triangle + mesh->triangle_count;

    for (i = mesh->first_triangle; i < end; i++) {
        scene->objects.ptr[i].aabb.min = v_add(scene->objects.ptr[i].aabb.min, delta);
        scene->objects.ptr[i].aabb.max = v_add(scene->objects.ptr[i].aabb.max, delta);
        scene->objects.ptr[i].type.triangle.a = v_add(scene->objects.ptr[i].type.triangle.a, delta);
        scene->objects.ptr[i].type.triangle.b = v_add(scene->objects.ptr[i].type.triangle.b, delta);
        scene->objects.ptr[i].type.triangle.c = v_add(scene->objects.ptr[i].type.triangle.c, delta);
    }

    mesh->aabb.min = v_add(mesh->aabb.min, delta);
    mesh->aabb.max = v_add(mesh->aabb.max, delta);
}

void scale_mesh(Scene *scene, Mesh *mesh, Vec scaling) {
    size_t i, end;
    Vec delta;

    mesh->size = hadamard(mesh->size, scaling);
    end = mesh->first_triangle + mesh->triangle_count;

    for (i = mesh->first_triangle; i < end; i++) {
        delta = v_sub(scene->objects.ptr[i].type.triangle.a, mesh->position);
        scene->objects.ptr[i].type.triangle.a = v_add(mesh->position, hadamard(delta, scaling));

        delta = v_sub(scene->objects.ptr[i].type.triangle.b, mesh->position);
        scene->objects.ptr[i].type.triangle.b = v_add(mesh->position, hadamard(delta, scaling));

        delta = v_sub(scene->objects.ptr[i].type.triangle.c, mesh->position);
        scene->objects.ptr[i].type.triangle.c = v_add(mesh->position, hadamard(delta, scaling));

        scene->objects.ptr[i].type.triangle.ng = normalize(hadamard(scene->objects.ptr[i].type.triangle.ng, reciproc(scaling)));
        scene->objects.ptr[i].type.triangle.na = normalize(hadamard(scene->objects.ptr[i].type.triangle.na, reciproc(scaling)));
        scene->objects.ptr[i].type.triangle.nb = normalize(hadamard(scene->objects.ptr[i].type.triangle.nb, reciproc(scaling)));
        scene->objects.ptr[i].type.triangle.nc = normalize(hadamard(scene->objects.ptr[i].type.triangle.nc, reciproc(scaling)));

        scene->objects.ptr[i].aabb = (AABB) {
            .min = v_min(v_min(scene->objects.ptr[i].type.triangle.a, scene->objects.ptr[i].type.triangle.b), scene->objects.ptr[i].type.triangle.c),
            .max = v_max(v_max(scene->objects.ptr[i].type.triangle.a, scene->objects.ptr[i].type.triangle.b), scene->objects.ptr[i].type.triangle.c)
        };
    }

    mesh->aabb.max = v_add(mesh->aabb.min, hadamard(v_sub(mesh->aabb.max, mesh->aabb.min), scaling));
}

void rotate_mesh(Scene *scene, Mesh *mesh, Vec rotation) {
    size_t i, end;
    Vec delta, min, max;

    mesh->rotation = v_add(mesh->rotation, rotation);

    end = mesh->first_triangle + mesh->triangle_count;

    for (i = mesh->first_triangle; i < end; i++) {
        delta = v_sub(scene->objects.ptr[i].type.triangle.a, mesh->position);
        scene->objects.ptr[i].type.triangle.a = v_add(mesh->position, rotate(delta, rotation));

        delta = v_sub(scene->objects.ptr[i].type.triangle.b, mesh->position);
        scene->objects.ptr[i].type.triangle.b = v_add(mesh->position, rotate(delta, rotation));

        delta = v_sub(scene->objects.ptr[i].type.triangle.c, mesh->position);
        scene->objects.ptr[i].type.triangle.c = v_add(mesh->position, rotate(delta, rotation));

        scene->objects.ptr[i].type.triangle.ng = normalize(rotate(scene->objects.ptr[i].type.triangle.ng, rotation));
        scene->objects.ptr[i].type.triangle.na = normalize(rotate(scene->objects.ptr[i].type.triangle.na, rotation));
        scene->objects.ptr[i].type.triangle.nb = normalize(rotate(scene->objects.ptr[i].type.triangle.nb, rotation));
        scene->objects.ptr[i].type.triangle.nc = normalize(rotate(scene->objects.ptr[i].type.triangle.nc, rotation));

        scene->objects.ptr[i].aabb = (AABB) {
            .min = v_min(v_min(scene->objects.ptr[i].type.triangle.a, scene->objects.ptr[i].type.triangle.b), scene->objects.ptr[i].type.triangle.c),
            .max = v_max(v_max(scene->objects.ptr[i].type.triangle.a, scene->objects.ptr[i].type.triangle.b), scene->objects.ptr[i].type.triangle.c)
        };
    }

    min = mesh->aabb.min;
    max = v_add(mesh->aabb.min, rotate(v_sub(mesh->aabb.max, mesh->aabb.min), rotation));

    mesh->aabb.min = v_min(min, max);
    mesh->aabb.max = v_max(min, max);

}

void set_mesh_position(Scene *scene, Mesh *mesh, Vec position) {
    Vec delta;

    delta = v_sub(position, mesh->position);    
    move_mesh(scene, mesh, delta);
}

void set_mesh_size(Scene *scene, Mesh *mesh, Vec size) {
    Vec delta;

    delta = hadamard(size, reciproc(mesh->size));
    scale_mesh(scene, mesh, delta);
}

void set_mesh_rotation(Scene *scene, Mesh *mesh, Vec rotation) {
    Vec delta;

    delta = v_sub(rotation, mesh->rotation);
    rotate_mesh(scene, mesh, delta);
}

void apply_mesh_transform(Scene *scene, Mesh *mesh) {
    Vec min, max;

    min = mesh->aabb.min;
    max = mesh->aabb.max;

    mesh->aabb.min = v_min(min, max);
    mesh->aabb.max = v_max(min, max);
    
    mesh->position = mesh->aabb.min;
    mesh->size = v_sub(mesh->aabb.max, mesh->aabb.min);
    mesh->rotation = vec(0.0, 0.0, 0.0);
}