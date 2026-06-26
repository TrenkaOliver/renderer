#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "scene/scene.h"

Material no_texture = {
    .diffuse = {.x = 1.0, .y = 1.0, .z = 1.0},
    .specular = {.x = 0.0, .y = 0.0, .z = 0.0},
    .shininess = 1,
    .reflectivity = 0
};

Mesh *inport_mesh(Scene *scene, char *file_name) {
    FILE *f;
    VecDynArray v_arr, vt_arr, vn_arr;
    Mesh *mesh;
    Object *t;
    AABB aabb;
    Vec _v;
    Face idx[64];
    size_t count, i;
    char line[128], *p;
    long long v, vn, vt;
    
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
                while (isspace((unsigned char)*p)) p++;
                if (!*p) break;

                v = strtoll(p, &p, 10);
                v = v < 0 ? (size_t)(v_arr.count + v) : (size_t)(v - 1);
                if (*p == '/') {
                    p++;

                    if (*p != '/') {
                        vt = strtoll(p, &p, 10);
                        vt = vt < 0 ? (size_t)(vt_arr.count + vt) : (size_t)(vt - 1);
                    } else {
                        vt = (size_t)-1;
                    }

                    if (*p == '/') {
                        p++;
                        vn = strtoll(p, &p, 10);
                        vn = vn < 0 ? (size_t)(vn_arr.count + vn) : (size_t)(vn - 1);
                    } else {
                        vn = (size_t)-1;
                    }
                } else {
                    vt = (size_t)-1;
                    vn = (size_t)-1;
                }

                idx[count++] = (Face){.v = v, .vt = vt, .vn = vn};

                while(*p && !isspace((unsigned char)*p)) p++;
                
                if (count >= 64) break;
            }

            if (count < 3) continue;

            for (i = 1; i < count - 1; i++) {
                t = add_triangle_complete(
                    scene,
                    v_arr.ptr[idx[0].v],
                    v_arr.ptr[idx[i].v],
                    v_arr.ptr[idx[i + 1].v],
                    idx[0].vn != (size_t)-1 ? vn_arr.ptr[idx[0].vn] : vec(0.0, 0.0, 0.0),
                    idx[i].vn != (size_t)-1 ? vn_arr.ptr[idx[i].vn] : vec(0.0, 0.0, 0.0),
                    idx[i + 1].vn != (size_t)-1 ? vn_arr.ptr[idx[i + 1].vn] : vec(0.0, 0.0, 0.0),
                    idx[0].vt != (size_t)-1 ? vt_arr.ptr[idx[0].vt] : vec(0.0, 0.0, 0.0),
                    idx[i].vt != (size_t)-1 ? vt_arr.ptr[idx[i].vt] : vec(0.0, 0.0, 0.0),
                    idx[i + 1].vt != (size_t)-1 ? vt_arr.ptr[idx[i + 1].vt] : vec(0.0, 0.0, 0.0),
                    &no_texture
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

    fclose(f);

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

void apply_mesh_transform(Mesh *mesh) {
    Vec min, max;

    min = mesh->aabb.min;
    max = mesh->aabb.max;

    mesh->aabb.min = v_min(min, max);
    mesh->aabb.max = v_max(min, max);
    
    mesh->position = mesh->aabb.min;
    mesh->size = v_sub(mesh->aabb.max, mesh->aabb.min);
    mesh->rotation = vec(0.0, 0.0, 0.0);
}