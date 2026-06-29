#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "scene/scene.h"

#define STB_IMAGE_IMPLEMENTATION
#include "image/stb_image.h"

Material no_material = {
    .diffuse = {.x = 1.0, .y = 1.0, .z = 1.0},
    .specular = {.x = 0.0, .y = 0.0, .z = 0.0},
    .shininess = 1,
    .reflectivity = 0
};

void create_path(char *buff, char *file_name, char *child_name) {
    char *last_slash, *last_backslash;
    int cc1, cc2;

    last_slash = strrchr(file_name, '/');
    last_backslash = strrchr(file_name, '\\');
    
    if (!last_slash || (last_backslash && last_backslash > last_slash)) {
        last_slash = last_backslash;
    }

    strcpy(buff, file_name);
    
    
    cc1 = last_slash - file_name;
    cc2 = 0;
    while ((buff[++cc1] = child_name[cc2++]));
}

size_t import_mesh(Scene *scene, char *file_name) {
    FILE *f, *m, *t;
    DynArray v_arr, vt_arr, vn_arr, m_idx;
    Mesh *mesh;
    Material *active_material;
    AABB aabb;
    Vec _v, *vp, *vtp, *vnp;
    Face idx[64];
    size_t count, i, t_idx, out;
    char line[128], mtl_name[128], mtl_path[128], texture_name[128], texture_path[128], *p, *last_slash, *last_backslash;
    unsigned char *pixel_ptr;
    long long v, vn, vt;
    int cc1, cc2, illum, x, y;
    
    f = fopen(file_name, "r");
    v_arr = create_dyn_array(sizeof(Vec), 64);
    vt_arr = create_dyn_array(sizeof(Vec), 64);
    vn_arr = create_dyn_array(sizeof(Vec), 64);
    
    out = grow_dyn_array(&scene->meshes);
    mesh = get_element(out, &scene->meshes);

    mesh->rotation = vec(0.0, 0.0, 0.0);
    mesh->first_triangle = scene->objects.count;
    mesh->triangle_count = 0;
    active_material = &no_material;

    aabb = (AABB){.min = vec(INFINITY, INFINITY, INFINITY), .max = vec(-INFINITY, -INFINITY, -INFINITY)};

    while (fgets(line, 128, f)) {
        if (sscanf(line, "v %lf %lf %lf", &_v.x, &_v.y, &_v.z) == 3) {
            i = grow_dyn_array(&v_arr);
            *(Vec *)get_element(i, &v_arr) = _v;
        } else if (sscanf(line, "vn %lf %lf %lf", &_v.x, &_v.y, &_v.z) == 3) {
            i = grow_dyn_array(&vn_arr);
            *(Vec *)get_element(i, &vn_arr) = _v;
        } else if (sscanf(line, "vt %lf %lf", &_v.x, &_v.y) == 2) {
            i = grow_dyn_array(&vt_arr);
            *(Vec *)get_element(i, &vt_arr) = _v;
        } else if (strncmp(line, "mtllib ", 7) == 0) {
            printf("alam\n");
            cc1 = 0;
            cc2 = 7;
            while((mtl_name[cc1++] = line[cc2++]) && mtl_name[cc1 - 1] != '\n');
            if (mtl_name[cc1 - 1] == '\n') mtl_name[cc1 - 1] = '\0';

            create_path(mtl_path, file_name, mtl_name);
            m = fopen(mtl_path, "r");
            if (!m) continue;

            m_idx = create_dyn_array(sizeof(MaterialEntry), 16);
            while (fgets(line, 128, m)) {
                if (sscanf(line, "newmtl %s", mtl_name) == 1) {
                    active_material = get_element(add_material(mtl_name, &m_idx, scene), &scene->materials);
                    active_material->reflectivity = 0.0;
                    active_material->diffuse_map = (size_t)-1;
                    active_material->normal_map = (size_t)-1;
                } else if (sscanf(line, "Kd %lf %lf %lf", &_v.x, &_v.y, &_v.z) == 3) {
                    active_material->diffuse = _v;
                } else if (sscanf(line, "Ks %lf %lf %lf", &_v.x, &_v.y, &_v.z) == 3) {
                    active_material->specular = _v;
                } else if (sscanf(line, "Ns %lf", &_v.x) == 1) {
                    active_material->shininess = _v.x;
                } else if (sscanf(line, "illum %d", &illum) == 1) {
                    switch (illum) {
                    case 0:
                        active_material->diffuse = vec(0.0, 0.0, 0.0);
                        active_material->specular = vec(0.0, 0.0, 0.0);
                        break;
                    case 1:
                        active_material->specular = vec(0.0, 0.0, 0.0);
                    default:
                        break;
                    }
                } else if (strncmp(line, "map_Kd ", 7) == 0) {
                    cc1 = 0;
                    cc2 = 7;
                    while((texture_name[cc1++] = line[cc2++]) && texture_name[cc1 - 1] != '\n');
                    if (texture_name[cc1 - 1] == '\n') texture_name[cc1 - 1] = '\0';
                    create_path(texture_path, mtl_path, texture_name);
                    pixel_ptr = stbi_load(texture_path, &x, &y, NULL, 3);
                    i = grow_n_dyn_array(&scene->textures, 2 * sizeof(int) + x * y * 3);
                    int *start = get_element(i, &scene->textures);
                    start[0] = x;
                    start[1] = y;
                    memcpy(start + 2, pixel_ptr, x * y * 3);
                    active_material->diffuse_map = i;
                    stbi_image_free(pixel_ptr);
                }
            }
            fclose(m);
        } else if (sscanf(line, "usemtl %s", mtl_name) == 1 && active_material != &no_material) {
            active_material = get_element(get_material_id(mtl_name, &m_idx), &scene->materials);
        }else if (line[0] == 'f' && line[1] == ' ') {
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

            vp = v_arr.ptr;
            vnp = vn_arr.ptr;
            vtp = vt_arr.ptr;

            for (i = 1; i < count - 1; i++) {
                t_idx = add_triangle_complete(
                    scene,
                    vp[idx[0].v],
                    vp[idx[i].v],
                    vp[idx[i + 1].v],
                    idx[0].vn != (size_t)-1 ? vnp[idx[0].vn] : vec(0.0, 0.0, 0.0),
                    idx[i].vn != (size_t)-1 ? vnp[idx[i].vn] : vec(0.0, 0.0, 0.0),
                    idx[i + 1].vn != (size_t)-1 ? vnp[idx[i + 1].vn] : vec(0.0, 0.0, 0.0),
                    idx[0].vt != (size_t)-1 ? vtp[idx[0].vt] : vec(0.0, 0.0, 0.0),
                    idx[i].vt != (size_t)-1 ? vtp[idx[i].vt] : vec(0.0, 0.0, 0.0),
                    idx[i + 1].vt != (size_t)-1 ? vtp[idx[i + 1].vt] : vec(0.0, 0.0, 0.0),
                    active_material
                );
                aabb = aabb_merge(aabb, ((Object *)get_element(t_idx, &scene->objects))->aabb);
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

    return out;
}

void move_mesh(Scene *scene, Mesh *mesh, Vec delta) {
    Object *ptr;
    size_t i, end;
    
    ptr = scene->objects.ptr;

    mesh->position = v_add(mesh->position, delta);
    end = mesh->first_triangle + mesh->triangle_count;

    for (i = mesh->first_triangle; i < end; i++) {
        ptr[i].aabb.min = v_add(ptr[i].aabb.min, delta);
        ptr[i].aabb.max = v_add(ptr[i].aabb.max, delta);
        ptr[i].type.triangle.a = v_add(ptr[i].type.triangle.a, delta);
        ptr[i].type.triangle.b = v_add(ptr[i].type.triangle.b, delta);
        ptr[i].type.triangle.c = v_add(ptr[i].type.triangle.c, delta);
    }

    mesh->aabb.min = v_add(mesh->aabb.min, delta);
    mesh->aabb.max = v_add(mesh->aabb.max, delta);
}

void scale_mesh(Scene *scene, Mesh *mesh, Vec scaling) {
    size_t i, end;
    Vec delta;
    Object *ptr;

    ptr = scene->objects.ptr;

    mesh->size = hadamard(mesh->size, scaling);
    end = mesh->first_triangle + mesh->triangle_count;

    mesh->aabb = (AABB){
        .min = vec(INFINITY, INFINITY, INFINITY), 
        .max = vec(-INFINITY, -INFINITY, -INFINITY)
    };

    for (i = mesh->first_triangle; i < end; i++) {
        delta = v_sub(ptr[i].type.triangle.a, mesh->position);
        ptr[i].type.triangle.a = v_add(mesh->position, hadamard(delta, scaling));

        delta = v_sub(ptr[i].type.triangle.b, mesh->position);
        ptr[i].type.triangle.b = v_add(mesh->position, hadamard(delta, scaling));

        delta = v_sub(ptr[i].type.triangle.c, mesh->position);
        ptr[i].type.triangle.c = v_add(mesh->position, hadamard(delta, scaling));

        ptr[i].type.triangle.ng = normalize(hadamard(ptr[i].type.triangle.ng, reciproc(scaling)));
        ptr[i].type.triangle.na = normalize(hadamard(ptr[i].type.triangle.na, reciproc(scaling)));
        ptr[i].type.triangle.nb = normalize(hadamard(ptr[i].type.triangle.nb, reciproc(scaling)));
        ptr[i].type.triangle.nc = normalize(hadamard(ptr[i].type.triangle.nc, reciproc(scaling)));

        ptr[i].aabb = (AABB) {
            .min = v_min(v_min(ptr[i].type.triangle.a, ptr[i].type.triangle.b), ptr[i].type.triangle.c),
            .max = v_max(v_max(ptr[i].type.triangle.a, ptr[i].type.triangle.b), ptr[i].type.triangle.c)
        };

        mesh->aabb = aabb_merge(mesh->aabb, ptr[i].aabb);
    }
}

void rotate_mesh(Scene *scene, Mesh *mesh, Vec rotation) {
    size_t i, end;
    Vec delta;
    Object *ptr;

    ptr = scene->objects.ptr;

    mesh->rotation = v_add(mesh->rotation, rotation);
    end = mesh->first_triangle + mesh->triangle_count;

    mesh->aabb = (AABB){
        .min = vec(INFINITY, INFINITY, INFINITY), 
        .max = vec(-INFINITY, -INFINITY, -INFINITY)
    };

    for (i = mesh->first_triangle; i < end; i++) {
        delta = v_sub(ptr[i].type.triangle.a, mesh->position);
        ptr[i].type.triangle.a = v_add(mesh->position, rotate(delta, rotation));

        delta = v_sub(ptr[i].type.triangle.b, mesh->position);
        ptr[i].type.triangle.b = v_add(mesh->position, rotate(delta, rotation));

        delta = v_sub(ptr[i].type.triangle.c, mesh->position);
        ptr[i].type.triangle.c = v_add(mesh->position, rotate(delta, rotation));

        ptr[i].type.triangle.ng = normalize(rotate(ptr[i].type.triangle.ng, rotation));
        ptr[i].type.triangle.na = normalize(rotate(ptr[i].type.triangle.na, rotation));
        ptr[i].type.triangle.nb = normalize(rotate(ptr[i].type.triangle.nb, rotation));
        ptr[i].type.triangle.nc = normalize(rotate(ptr[i].type.triangle.nc, rotation));

        ptr[i].aabb = (AABB) {
            .min = v_min(v_min(ptr[i].type.triangle.a, ptr[i].type.triangle.b), ptr[i].type.triangle.c),
            .max = v_max(v_max(ptr[i].type.triangle.a, ptr[i].type.triangle.b), ptr[i].type.triangle.c)
        };

        mesh->aabb = aabb_merge(mesh->aabb, ptr[i].aabb);
    }
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