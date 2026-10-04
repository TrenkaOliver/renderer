#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#include "scene/scene.h"
#include "stb_image.h"
#include "yyjson.h"

void create_path(char *buff, char *file_name, char *child_name);
size_t import_texture(char *line, char *mtl_path, Scene *scene);

void create_path(char *buff, char *file_name, char *child_name) {
    // char *last_slash, *last_backslash;
    // int cc1, cc2;

    // last_slash = strrchr(file_name, '/');
    // last_backslash = strrchr(file_name, '\\');
    
    // if (!last_slash || (last_backslash && last_backslash > last_slash)) {
    //     last_slash = last_backslash;
    // }

    // if (!last_slash) {

    // }

    // strcpy(buff, file_name);
    
    
    // cc1 = last_slash - file_name;
    // cc2 = 0;
    // while ((buff[++cc1] = child_name[cc2++]));

    char *last_slash = strrchr(file_name, '/');
    char *last_backslash = strrchr(file_name, '\\');

    if (!last_slash || (last_backslash && last_backslash > last_slash))
        last_slash = last_backslash;

    if (!last_slash) {
        strcpy(buff, child_name);
        return;
    }

    size_t dir_len = last_slash - file_name + 1;

    memcpy(buff, file_name, dir_len);
    strcpy(buff + dir_len, child_name);
}

size_t import_texture(char *line, char *mtl_path, Scene *scene) {
    size_t i;
    int cc1, cc2, x, y, *start;
    unsigned char *pixel_ptr;
    char texture_name[128], texture_path[128];

    cc1 = 0;
    cc2 = 0;
    while((texture_name[cc1++] = line[cc2++]) && texture_name[cc1 - 1] != '\n');
    if (texture_name[cc1 - 1] == '\n') texture_name[cc1 - 1] = '\0';
    create_path(texture_path, mtl_path, texture_name);
    pixel_ptr = stbi_load(texture_path, &x, &y, NULL, 3);
    i = grow_n_dyn_array(&scene->textures, 2 * sizeof(int) + x * y * 3);
    start = get_element(i, &scene->textures);
    start[0] = x;
    start[1] = y;
    memcpy(start + 2, pixel_ptr, x * y * 3);
    stbi_image_free(pixel_ptr);
    return i;
}

size_t import_obj_mesh(Scene *scene, char *file_name) {
    // FILE *f = fopen(file_name, "r");

    // Material *active_material = NULL;
    // uint32_t active_material_id = 0;
    // DynArray m_idx = create_dyn_array(sizeof(MaterialEntry), 16);

    // uint32_t out = grow_dyn_array(&scene->meshes);
    // Mesh *mesh = get_element(out, &scene->meshes);

    // mesh->first_triangle = scene->triangle_count;
    // mesh->triangle_count = 0;

    // mesh->first_vertex_pos = scene->vertex_pos_count;
    // mesh->vertex_pos_count = 0;

    // mesh->first_vertex_normal = scene->vertex_normal_count;
    // mesh->vertex_normal_count = 0;

    // mesh->first_vertex_texcoord = scene->vertex_texcoord_count;
    // mesh->vertex_texcoord_count = 0;

    // AABB aabb = (AABB) {
    //     .min = {FLT_MAX, FLT_MAX, FLT_MAX}, 
    //     .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
    // };

    // char line[512], mtl_name[128];

    // while (fgets(line, 512, f)) {
    //     double x, y, z;
    //     if (sscanf(line, "v %lf %lf %lf", &x, &y, &z) == 3) {
    //         add_vertex_pos(x, -z, y, scene);
    //         mesh->vertex_pos_count++;
    //     } else if (sscanf(line, "vn %lf %lf %lf", &x, &y, &z) == 3) {
    //         add_vertex_normal(x, -z, y, scene);
    //         mesh->vertex_normal_count++;
    //     } else if (sscanf(line, "vt %lf %lf", &x, &y) == 2) {
    //         add_vertex_texcoord(x, y, scene);
    //         mesh->vertex_texcoord_count++;
    //     } else if (strncmp(line, "mtllib ", 7) == 0) {
    //         int cc1 = 0;
    //         int cc2 = 7;

    //         char mtl_name[128], mtl_path[128];

    //         while((mtl_name[cc1++] = line[cc2++]) && mtl_name[cc1 - 1] != '\n');
    //         if (mtl_name[cc1 - 1] == '\n') mtl_name[cc1 - 1] = '\0';

    //         create_path(mtl_path, file_name, mtl_name);
    //         FILE *m = fopen(mtl_path, "r");
    //         if (!m) continue;

    //         printf("Loading material file: %s\n", mtl_path);

    //         int illium;

    //         //must handle material change

    //         // while (fgets(line, 512, m)) {
    //         //     if (sscanf(line, "newmtl %s", mtl_name) == 1) {
    //         //         active_material = get_element(add_material(mtl_name, &m_idx, scene), &scene->materials);
    //         //         active_material->reflectivity = 0.0;
    //         //         active_material->diffuse_map = (size_t)-1;
    //         //         active_material->splecular_map = (size_t)-1;
    //         //         active_material->normal_map = (size_t)-1;
    //         //     } else if (sscanf(line, "Kd %lf %lf %lf", &x, &y, &z) == 3) {
    //         //         active_material->diffuse = vec(x, y, z);
    //         //     } else if (sscanf(line, "Ks %lf %lf %lf", &x, &y, &z) == 3) {
    //         //         active_material->specular = vec(x, y, z);
    //         //     } else if (sscanf(line, "Ns %lf", &x) == 1) {
    //         //         active_material->shininess = x;
    //         //     } else if (sscanf(line, "illum %d", &illium) == 1) {
    //         //         switch (illium) {
    //         //         case 0:
    //         //             active_material->diffuse = vec(0.0, 0.0, 0.0);
    //         //             active_material->specular = vec(0.0, 0.0, 0.0);
    //         //             break;
    //         //         case 1:
    //         //             active_material->specular = vec(0.0, 0.0, 0.0);
    //         //         default:
    //         //             break;
    //         //         }

    //         //     } else if (strncmp(line, "map_Kd ", 7) == 0) {
    //         //         active_material->diffuse_map = import_texture(line + 7, mtl_path, scene);
    //         //     }
    //         // }

    //         fclose(m);
    //     } else if (sscanf(line, "usemtl %s", mtl_name) == 1) {
    //         active_material_id = get_material_id(mtl_name, &m_idx);
    //     } else if (line[0] == 'f' && line[1] == ' ') {
    //         char *p = line + 2;
    //         uint32_t count = 0;
    //         Face idx[64];

    //         while(*p) {
    //             while (isspace((unsigned char)*p)) p++;
    //             if (!*p) break;

    //             long long v, vt, vn;

    //             v = strtoll(p, &p, 10);
    //             v = v < 0 ? (uint32_t)(mesh->vertex_pos_count + v) : (uint32_t)(v - 1);
    //             if (*p == '/') {
    //                 p++;

    //                 if (*p != '/') {
    //                     vt = strtoll(p, &p, 10);
    //                     vt = vt < 0 ? (uint32_t)(mesh->vertex_texcoord_count + vt) : (uint32_t)(vt - 1);
    //                 } else {
    //                     vt = (uint32_t)-1;
    //                 }
                    
    //                 if (*p == '/') {
    //                     p++;
    //                     vn = strtoll(p, &p, 10);
    //                     vn = vn < 0 ? (uint32_t)(mesh->vertex_normal_count + vn) : (uint32_t)(vn - 1);
    //                 } else {
    //                     vn = (uint32_t)-1;
    //                 }
    //             } else {
    //                 vt = (uint32_t)-1;
    //                 vn = (uint32_t)-1;
    //             }

    //             idx[count++] = (Face){.v = v, .vt = vt, .vn = vn};

    //             while(*p && !isspace((unsigned char)*p)) p++;
                
    //             if (count >= 64) break;
    //         }

    //         for (int i = 1; i < count - 1; i++) {
    //             add_triangle_from_indices(
    //                 mesh->first_vertex_pos + idx[0].v,
    //                 mesh->first_vertex_pos + idx[i].v,
    //                 mesh->first_vertex_pos + idx[i + 1].v,

    //                 idx[0].vn == (uint32_t)-1 ? (uint32_t)-1 : mesh->first_vertex_normal + idx[0].vn,
    //                 idx[i].vn == (uint32_t)-1 ? (uint32_t)-1 : mesh->first_vertex_normal + idx[i].vn,
    //                 idx[i + 1].vn == (uint32_t)-1 ? (uint32_t)-1 : mesh->first_vertex_normal + idx[i + 1].vn,

    //                 idx[0].vt == (uint32_t)-1 ? (uint32_t)-1 : mesh->first_vertex_texcoord + idx[0].vt,
    //                 idx[i].vt == (uint32_t)-1 ? (uint32_t)-1 : mesh->first_vertex_texcoord + idx[i].vt,
    //                 idx[i + 1].vt == (uint32_t)-1 ? (uint32_t)-1 : mesh->first_vertex_texcoord + idx[i + 1].vt,

    //                 active_material_id,
    //                 scene
    //             );

    //             aabb = aabb_merge(aabb, scene->triangles.aabb[scene->triangle_count - 1]);
    //             mesh->triangle_count++;
    //         }
    //     }
    // }

    // mesh->position = vec(aabb.min[0], aabb.min[1], aabb.min[2]);
    // mesh->rotation = vec(0.0, 0.0, 0.0);
    // mesh->size = vec(aabb.max[0] - aabb.min[0], aabb.max[1] - aabb.min[1], aabb.max[2] - aabb.min[2]);
    // mesh->aabb = aabb;

    // return out;
}

typedef struct {
    unsigned char *data;

    size_t count;
    size_t stride;

    int component_size;
    int component_count;

    size_t byte_length;
} __gltfAccessor;

int handle_component_size(size_t s) {
    switch (s) {
    case 5120: case 5121: return 1;
    case 5122: case 5123: return 2;
    case 5125: case 5126: return 4;
    default: return -1;
    }
}

int pharse_component_count(char *s) {
    if (strcmp(s, "SCALAR") == 0)
        return 1;
    else if (strcmp(s, "VEC2") == 0)
        return 2;
    else if (strcmp(s, "VEC3") == 0)
        return 3;

    else return -1;
}

uint32_t import_glTF(Scene *scene, char *file_name) {
    yyjson_doc *doc = yyjson_read_file(file_name, 0, NULL, NULL);

    yyjson_val *root = yyjson_doc_get_root(doc);

    DynArray buffer_array = create_dyn_array(sizeof(unsigned char *), 4);
    DynArray accessor_array = create_dyn_array(sizeof(__gltfAccessor), 4);
    DynArray image_array = create_dyn_array(sizeof(unsigned char *), 4);

    yyjson_val *_buffers = yyjson_obj_get(root, "buffers");
    size_t _buffers_array_size = yyjson_arr_size(_buffers);

    for (size_t i = 0; i < _buffers_array_size; i++) {
        yyjson_val *entry = yyjson_arr_get(_buffers, i);

        size_t byte_length = yyjson_get_uint(yyjson_obj_get(entry, "byteLength"));

        char path[512];
        
        create_path(
            path, 
            file_name,
            yyjson_get_str(yyjson_obj_get(entry, "uri"))
        );

        FILE *f = fopen(path, "rb");

        unsigned char *p = malloc(byte_length);
        fread(p, 1, byte_length, f);
        *((unsigned char **)get_element(grow_dyn_array(&buffer_array), &buffer_array)) = p;
        fclose(f);
    }

    yyjson_val *_accessors = yyjson_obj_get(root, "accessors");
    size_t _accessors_array_count = yyjson_arr_size(_accessors);

    for (size_t i = 0; i < _accessors_array_count; i++) {
        __gltfAccessor gltfAccessor;

        yyjson_val *accessor = yyjson_arr_get(_accessors, i);

        gltfAccessor.count = yyjson_get_uint(yyjson_obj_get(accessor, "count"));
        gltfAccessor.component_size = handle_component_size(yyjson_get_uint(yyjson_obj_get(accessor, "componentType")));
        gltfAccessor.component_count = pharse_component_count(yyjson_get_str(yyjson_obj_get(accessor, "type")));
        gltfAccessor.stride = gltfAccessor.component_size * gltfAccessor.component_count;

        yyjson_val *buffer_view = yyjson_arr_get(yyjson_obj_get(root, "bufferViews"), yyjson_get_int(yyjson_obj_get(accessor, "bufferView")));
        gltfAccessor.byte_length = yyjson_get_uint(yyjson_obj_get(buffer_view, "byteLength"));
        gltfAccessor.data = *((unsigned char **)get_element(yyjson_get_uint(yyjson_obj_get(buffer_view, "buffer")), &buffer_array)) + yyjson_get_uint(yyjson_obj_get(buffer_view, "byteOffset"));
        
        *((__gltfAccessor *)get_element(grow_dyn_array(&accessor_array), &accessor_array)) = gltfAccessor;
    }


    yyjson_val *meshes = yyjson_obj_get(root, "meshes");
    size_t mesh_array_count = yyjson_arr_size(meshes);

    for (size_t i = 0; i < mesh_array_count; i++) {
        yyjson_val *item = yyjson_arr_get(meshes, i);

        Mesh *mesh = get_element(grow_dyn_array(&scene->meshes), &scene->meshes);
        mesh->aabb = (AABB) {
            .min = {FLT_MAX, FLT_MAX, FLT_MAX},
            .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
        };        

        yyjson_val *primitives = yyjson_obj_get(item, "primitives");
        ssize_t primitve_array_count = yyjson_arr_size(primitives);

        uint32_t first_vertex = scene->vertex_info[0];
        uint32_t first_triangle = scene->triangle_count;

        for (size_t j = 0; j < primitve_array_count; j++) {
            yyjson_val *primitive = yyjson_arr_get(primitives, j);
            yyjson_val *attributes = yyjson_obj_get(primitive, "attributes");

            char *titles[3] = {"POSITION", "NORMAL", "TEXCOORD_0"};

            uint32_t vertex_offset = scene->vertex_info[0];

            for (int k = 0; k < 3; k++) {
                __gltfAccessor *accessor = (__gltfAccessor *)get_element(yyjson_get_uint(yyjson_obj_get(attributes, titles[k])), &accessor_array);
                if (scene->vertex_info[2 * k + 1] < scene->vertex_info[2 * k] + accessor->count) {
                    scene->vertex_data[k] = realloc(scene->vertex_data[k], (scene->vertex_info[2 * k] + accessor->count) * accessor->component_count * sizeof(float));
                    scene->vertex_info[2 * k + 1] = scene->vertex_info[2 * k] + accessor->count;
                }
                for (size_t l = 0; l < accessor->count * accessor->component_count; l++) {
                    scene->vertex_data[k][scene->vertex_info[2 * k] * accessor->component_count + l] = ((float *)(accessor->data))[l];
                }

                // if (k < 2) {
                //     for (size_t l = 0; l < accessor->count * accessor->component_count; l += 3) {
                //         float tmp = scene->vertex_data[k][scene->vertex_info[2 * k] * accessor->component_count + l + 1];
                //         scene->vertex_data[k][scene->vertex_info[2 * k] * accessor->component_count + l + 1] = -scene->vertex_data[k][scene->vertex_info[2 * k] * accessor->component_count + l + 2];
                //         scene->vertex_data[k][scene->vertex_info[2 * k] * accessor->component_count + l + 2] = tmp;
                //     }
                // }
                scene->vertex_info[2 * k] += accessor->count;
            }

            size_t material_index = yyjson_get_uint(yyjson_obj_get(primitive, "material"));
            size_t indices = yyjson_get_uint(yyjson_obj_get(primitive, "indices"));

            __gltfAccessor *index_accessor = (__gltfAccessor *)get_element(indices, &accessor_array);

            if (scene->triangle_capacity < scene->triangle_count + index_accessor->count / 3) {
                scene->triangles.ai = realloc(scene->triangles.ai, (scene->triangle_count + index_accessor->count / 3) * sizeof(uint32_t));
                scene->triangles.bi = realloc(scene->triangles.bi, (scene->triangle_count + index_accessor->count / 3) * sizeof(uint32_t));
                scene->triangles.ci = realloc(scene->triangles.ci, (scene->triangle_count + index_accessor->count / 3) * sizeof(uint32_t));
                scene->triangles.nx = realloc(scene->triangles.nx, (scene->triangle_count + index_accessor->count / 3) * sizeof(float));
                scene->triangles.ny = realloc(scene->triangles.ny, (scene->triangle_count + index_accessor->count / 3) * sizeof(float));
                scene->triangles.nz = realloc(scene->triangles.nz, (scene->triangle_count + index_accessor->count / 3) * sizeof(float));
                scene->triangles.aabb = realloc(scene->triangles.aabb, (scene->triangle_count + index_accessor->count / 3) * sizeof(AABB));
                scene->triangles.centroid = realloc(scene->triangles.centroid, (scene->triangle_count + index_accessor->count / 3) * sizeof(float) * 3);
                scene->triangles.material = realloc(scene->triangles.material, (scene->triangle_count + index_accessor->count / 3) * sizeof(uint32_t));
                scene->triangle_capacity = scene->triangle_count + index_accessor->count / 3;
            }
            for (size_t k = 0; k < index_accessor->count / 3; k++) {
                scene->triangles.ai[scene->triangle_count + k] = ((uint16_t *)(index_accessor->data))[3 * k + 0] + vertex_offset;
                scene->triangles.bi[scene->triangle_count + k] = ((uint16_t *)(index_accessor->data))[3 * k + 1] + vertex_offset;
                scene->triangles.ci[scene->triangle_count + k] = ((uint16_t *)(index_accessor->data))[3 * k + 2] + vertex_offset;

                float *_a = scene->vertex_data[0] + 3 * scene->triangles.ai[scene->triangle_count + k];
                Vec a = vec(
                    _a[0],
                    _a[1],
                    _a[2]
                );

                float *_b = scene->vertex_data[0] + 3 * scene->triangles.bi[scene->triangle_count + k];
                Vec b = vec(
                    _b[0],
                    _b[1],
                    _b[2]
                );

                float *_c = scene->vertex_data[0] + 3 * scene->triangles.ci[scene->triangle_count + k];
                Vec c = vec(
                    _c[0],
                    _c[1],
                    _c[2]
                );

                Vec n = normalize(cross(v_sub(b, a), v_sub(c, a)));
                
                scene->triangles.nx[scene->triangle_count + k] = n.x;
                scene->triangles.ny[scene->triangle_count + k] = n.y;
                scene->triangles.nz[scene->triangle_count + k] = n.z;

                float _tmp[3];
                float _min[3];
                float _max[3];

                vecf_min3(_a, _b, _tmp);
                vecf_min3(_c, _tmp, _min);

                vecf_max3(_a, _b, _tmp);
                vecf_max3(_c, _tmp, _max);

                AABB aabb = {
                    .min = {_min[0], _min[1], _min[2]},
                    .max = {_max[0], _max[1], _max[2]}
                };

                mesh->aabb = aabb_merge(mesh->aabb, aabb);

                scene->triangles.aabb[scene->triangle_count + k] = aabb;
                
                scene->triangles.centroid[3 * (scene->triangle_count + k) + 0] = (aabb.min[0] + aabb.max[0]) * 0.5f;
                scene->triangles.centroid[3 * (scene->triangle_count + k) + 1] = (aabb.min[1] + aabb.max[1]) * 0.5f;
                scene->triangles.centroid[3 * (scene->triangle_count + k) + 2] = (aabb.min[2] + aabb.max[2]) * 0.5f;
                
                scene->triangles.material[scene->triangle_count + k] = yyjson_get_uint(yyjson_obj_get(primitive, "material"));
            }

            scene->triangle_count += index_accessor->count / 3;
        }
        mesh->first_triangle = first_triangle;
        mesh->first_vertex_pos = first_vertex;
        mesh->first_vertex_normal = first_vertex;
        mesh->first_vertex_texcoord = first_vertex;

        mesh->triangle_count = scene->triangle_count - mesh->first_triangle;
        mesh->vertex_pos_count = scene->vertex_info[0] - first_vertex;
        mesh->vertex_normal_count = scene->vertex_info[0] - first_vertex;
        mesh->vertex_texcoord_count = scene->vertex_info[0] - first_vertex;
    
        mesh->position = vec(
            (mesh->aabb.min[0] + mesh->aabb.max[0]) * 0.5f,
            (mesh->aabb.min[1] + mesh->aabb.max[1]) * 0.5f,
            (mesh->aabb.min[2] + mesh->aabb.max[2]) * 0.5f
        );
    }

    yyjson_val *images = yyjson_obj_get(root, "images");
    size_t images_array_count = yyjson_arr_size(images);

    for (size_t i = 0; i < images_array_count; i++) {
        yyjson_val *image = yyjson_arr_get(images, i);

        char path[512];
        
        create_path(
            path, 
            file_name,
            yyjson_get_str(yyjson_obj_get(image, "uri"))
        );

        uint32_t w, h;

        unsigned char *p = stbi_load(path, &w, &h, NULL, 3);

        *((Texture *)get_element(grow_dyn_array(&scene->textures), &scene->textures)) = (Texture) {
            .w = w,
            .h = h,
            .ptr = p
        };
    }

    yyjson_val *materials = yyjson_obj_get(root, "materials");
    yyjson_val *textures = yyjson_obj_get(root, "textures");
    size_t materials_array_count = yyjson_arr_size(materials);
    
    for (size_t i = 0; i < materials_array_count; i++) {
        Material material;

        yyjson_val *mat = yyjson_arr_get(materials, i);

        yyjson_val *pbr = yyjson_obj_get(mat, "pbrMetallicRoughness");

        yyjson_val *base_color_factor = yyjson_obj_get(pbr, "baseColorFactor");
        if (base_color_factor) {
            material.base_color_factor.x = yyjson_get_real(yyjson_arr_get(base_color_factor, 0));
            material.base_color_factor.y = yyjson_get_real(yyjson_arr_get(base_color_factor, 1));
            material.base_color_factor.z = yyjson_get_real(yyjson_arr_get(base_color_factor, 2));
        } else {
            material.base_color_factor = vec(1.0, 1.0, 1.0);
        }

        yyjson_val *base_color_map = yyjson_obj_get(pbr, "baseColorTexture");
        if (base_color_map) {
            uint32_t index = yyjson_get_uint(yyjson_obj_get(base_color_map, "index"));
            material.base_color_map = yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(textures, index), "source"));
        } else {
            material.base_color_map = (uint32_t)-1;
        }

        yyjson_val *metallic = yyjson_obj_get(pbr, "metallicFactor");
        yyjson_val *roughness = yyjson_obj_get(pbr, "roughnessFactor");

        material.metallic = metallic ? yyjson_get_real(metallic) : 1.0;
        material.roughness = roughness ? yyjson_get_real(roughness) : 1.0;

        yyjson_val *metallic_roughness_map = yyjson_obj_get(pbr, "metallicRoughnessTexture");
        if (metallic_roughness_map) {
            uint32_t index = yyjson_get_uint(yyjson_obj_get(metallic_roughness_map, "index"));
            material.metallic_roughness_map = yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(textures, index), "source"));
        } else {
            material.metallic_roughness_map = (uint32_t)-1;
        }

        yyjson_val *occlusion_map = yyjson_obj_get(mat, "occlusionTexture");

        material.occlusion_strength = 1.0;

        if (occlusion_map) {
            uint32_t index = yyjson_get_uint(yyjson_obj_get(occlusion_map, "index"));
            material.occlusion_map = yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(textures, index), "source"));

            yyjson_val *strength = yyjson_obj_get(occlusion_map, "strength");
            if (strength) material.occlusion_strength = yyjson_get_real(strength);
        } else {
            material.occlusion_map = (uint32_t)-1;
        }

        yyjson_val *emissive_map = yyjson_obj_get(mat, "emissiveTexture");
        if (emissive_map) {
            uint32_t index = yyjson_get_uint(yyjson_obj_get(emissive_map, "index"));
            material.emissive_map = yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(textures, index), "source"));
        } else {
            material.emissive_map = (uint32_t)-1;
        }

        yyjson_val *emissive_factor = yyjson_obj_get(pbr, "emissiveFactor");
        if (emissive_factor) {
            material.emissive_factor.x = yyjson_get_real(yyjson_arr_get(emissive_factor, 0));
            material.emissive_factor.y = yyjson_get_real(yyjson_arr_get(emissive_factor, 1));
            material.emissive_factor.z = yyjson_get_real(yyjson_arr_get(emissive_factor, 2));
        } else {
            material.emissive_factor = vec(1.0, 1.0, 1.0);
        }        
        
        material.occlusion_strength = 1.0;

        if (occlusion_map) {
            uint32_t index = yyjson_get_uint(yyjson_obj_get(occlusion_map, "index"));
            material.occlusion_map = yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(textures, index), "source"));

            yyjson_val *strength = yyjson_obj_get(occlusion_map, "strength");
            if (strength) material.occlusion_strength = yyjson_get_real(strength);
        } else {
            material.occlusion_map = (uint32_t)-1;
        }

        yyjson_val *normal_texture = yyjson_obj_get(mat, "normalTexture");

        material.normal_scale = 1.0f;

        if (normal_texture) {
            uint32_t index = yyjson_get_uint(yyjson_obj_get(normal_texture, "index"));
            material.normal_map = yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(textures, index), "source"));

            yyjson_val *scale = yyjson_obj_get(normal_texture, "scale");
            if (scale) material.normal_scale = yyjson_get_real(scale);
        } else {
            material.normal_map = (uint32_t)-1;
        }

        *((Material *)get_element(grow_dyn_array(&scene->materials), &scene->materials)) = material;
    }

    delete_array(&buffer_array, 1);
    delete_array(&accessor_array, 0);
    yyjson_doc_free(doc);
}

void move_mesh(Scene *scene, Mesh *mesh, Vec delta) {
    size_t i, end;
    float delta_f[3] = {delta.x, delta.y, delta.z};

    mesh->position = v_add(mesh->position, delta);

    end = mesh->first_vertex_pos + mesh->vertex_pos_count;

    for (i = mesh->first_vertex_pos; i < end; i++) {
        scene->vertex_data[0][3 * i + 0] += delta_f[0];
        scene->vertex_data[0][3 * i + 1] += delta_f[1];
        scene->vertex_data[0][3 * i + 2] += delta_f[2];
    }

    end = mesh->first_triangle + mesh->triangle_count;

    for (i = mesh->first_triangle; i < end; i++) {
        vecf_add(delta_f, scene->triangles.aabb[i].min);
        vecf_add(delta_f, scene->triangles.aabb[i].max);
    }

    vecf_add(delta_f, mesh->aabb.min);
    vecf_add(delta_f, mesh->aabb.max);
}

void scale_mesh(Scene *scene, Mesh *mesh, Vec scaling) {
    Vec reciprocal_scaling = reciproc(scaling);

    mesh->size = hadamard(mesh->size, scaling);
    
    mesh->aabb = (AABB){
        .min = {FLT_MAX, FLT_MAX, FLT_MAX}, 
        .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
    };

    uint32_t end;
    
    end = mesh->first_vertex_pos + mesh->vertex_pos_count;
    for (uint32_t i = mesh->first_vertex_pos; i < end; i++) {
        float dx = scene->vertex_data[0][3 * i + 0] - mesh->position.x;
        float dy = scene->vertex_data[0][3 * i + 1] - mesh->position.y;
        float dz = scene->vertex_data[0][3 * i + 2] - mesh->position.z;

        scene->vertex_data[0][3 * i + 0] = mesh->position.x + dx * scaling.x;
        scene->vertex_data[0][3 * i + 1] = mesh->position.y + dy * scaling.y;
        scene->vertex_data[0][3 * i + 2] = mesh->position.z + dz * scaling.z;
    }

    end = mesh->first_vertex_normal + mesh->vertex_normal_count;
    for (uint32_t i = mesh->first_vertex_normal; i < end; i++) {
        Vec n = vec(scene->vertex_data[1][3 * i + 0], scene->vertex_data[1][3 * i + 1], scene->vertex_data[1][3 * i + 2]);
        n = normalize(hadamard(n, reciprocal_scaling));
        scene->vertex_data[1][3 * i + 0] = n.x;
        scene->vertex_data[1][3 * i + 1] = n.y;
        scene->vertex_data[1][3 * i + 2] = n.z;
    }

    end = mesh->first_triangle + mesh->triangle_count;
    for (uint32_t i = mesh->first_triangle; i < end; i++) {
        Vec n = vec(scene->triangles.nx[i], scene->triangles.ny[i], scene->triangles.nz[i]);
        n = normalize(hadamard(n, reciprocal_scaling));
        scene->triangles.nx[i] = n.x;
        scene->triangles.ny[i] = n.y;
        scene->triangles.nz[i] = n.z;

        float v_a[3] = {scene->vertex_data[0][3 * scene->triangles.ai[i] + 0], scene->vertex_data[0][3 * scene->triangles.ai[i] + 1], scene->vertex_data[0][3 * scene->triangles.ai[i] + 2]};
        float v_b[3] = {scene->vertex_data[0][3 * scene->triangles.bi[i] + 0], scene->vertex_data[0][3 * scene->triangles.bi[i] + 1], scene->vertex_data[0][3 * scene->triangles.bi[i] + 2]};
        float v_c[3] = {scene->vertex_data[0][3 * scene->triangles.ci[i] + 0], scene->vertex_data[0][3 * scene->triangles.ci[i] + 1], scene->vertex_data[0][3 * scene->triangles.ci[i] + 2]};
        float min[3], max[3];
        vecf_min3(v_a, v_b, min);
        vecf_min3(min, v_c, min);
        vecf_max3(v_a, v_b, max);
        vecf_max3(max, v_c, max);

        scene->triangles.aabb[i] = (AABB) {
            .min = {min[0], min[1], min[2]},
            .max = {max[0], max[1], max[2]}
        };

        mesh->aabb = aabb_merge(mesh->aabb, scene->triangles.aabb[i]);
    }
}

void rotate_mesh(Scene *scene, Mesh *mesh, Vec rotation) {

    mesh->rotation = v_add(mesh->rotation, rotation);
    mesh->aabb = (AABB){
        .min = {FLT_MAX, FLT_MAX, FLT_MAX}, 
        .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
    };

    uint32_t end;

    end = mesh->first_vertex_pos + mesh->vertex_pos_count;
    for (uint32_t i = mesh->first_vertex_pos; i < end; i++) {
        Vec delta = vec(scene->vertex_data[0][3 * i + 0] - mesh->position.x, scene->vertex_data[0][3 * i + 1] - mesh->position.y, scene->vertex_data[0][3 * i + 2] - mesh->position.z);
        Vec rotated_delta = rotate(delta, rotation);
        scene->vertex_data[0][3 * i + 0] = mesh->position.x + rotated_delta.x;
        scene->vertex_data[0][3 * i + 1] = mesh->position.y + rotated_delta.y;
        scene->vertex_data[0][3 * i + 2] = mesh->position.z + rotated_delta.z;
    }

    end = mesh->first_vertex_normal + mesh->vertex_normal_count;
    for (uint32_t i = mesh->first_vertex_normal; i < end; i++) {
        Vec n = vec(scene->vertex_data[1][3 * i + 0], scene->vertex_data[1][3 * i + 1], scene->vertex_data[1][3 * i + 2]);
        n = normalize(rotate(n, rotation));
        scene->vertex_data[1][3 * i + 0] = n.x;
        scene->vertex_data[1][3 * i + 1] = n.y;
        scene->vertex_data[1][3 * i + 2] = n.z;
    }

    end = mesh->first_triangle + mesh->triangle_count;
    for (uint32_t i = mesh->first_triangle; i < end; i++) {
        Vec n = vec(scene->triangles.nx[i], scene->triangles.ny[i], scene->triangles.nz[i]);
        n = normalize(rotate(n, rotation));
        scene->triangles.nx[i] = n.x;
        scene->triangles.ny[i] = n.y;
        scene->triangles.nz[i] = n.z;

        float v_a[3] = {scene->vertex_data[0][3 * scene->triangles.ai[i] + 0], scene->vertex_data[0][3 * scene->triangles.ai[i] + 1], scene->vertex_data[0][3 * scene->triangles.ai[i] + 2]};
        float v_b[3] = {scene->vertex_data[0][3 * scene->triangles.bi[i] + 0], scene->vertex_data[0][3 * scene->triangles.bi[i] + 1], scene->vertex_data[0][3 * scene->triangles.bi[i] + 2]};
        float v_c[3] = {scene->vertex_data[0][3 * scene->triangles.ci[i] + 0], scene->vertex_data[0][3 * scene->triangles.ci[i] + 1], scene->vertex_data[0][3 * scene->triangles.ci[i] + 2]};
        float min[3], max[3];
        vecf_min3(v_a, v_b, min);
        vecf_min3(min, v_c, min);
        vecf_max3(v_a, v_b, max);
        vecf_max3(max, v_c, max);

        scene->triangles.aabb[i] = (AABB) {
            .min = {min[0], min[1], min[2]},
            .max = {max[0], max[1], max[2]}
        };

        mesh->aabb = aabb_merge(mesh->aabb, scene->triangles.aabb[i]);
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
    float min[3] = {mesh->aabb.min[0], mesh->aabb.min[1], mesh->aabb.min[2]};
    float max[3] = {mesh->aabb.max[0], mesh->aabb.max[1], mesh->aabb.max[2]};

    vecf_min3(min, max, mesh->aabb.min);
    vecf_max3(min, max, mesh->aabb.max);
    
    mesh->position = vec(mesh->aabb.min[0], mesh->aabb.min[1], mesh->aabb.min[2]);
    mesh->size = v_sub(vec(mesh->aabb.max[0], mesh->aabb.max[1], mesh->aabb.max[2]), mesh->position);
    mesh->rotation = vec(0.0, 0.0, 0.0);
}