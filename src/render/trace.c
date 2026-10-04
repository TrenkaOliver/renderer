#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "render/trace.h"
#include "render/render.h"
#include "scene/scene.h"

static inline int clampi(int x, int min, int max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

HitResult get_first_hit(Ray *ray, Scene *scene, BVH8Tree *bvh) {
    HitResult plane_result, object_result;

    plane_result = get_first_plane(ray, &scene->planes);
    object_result = get_first_object(ray, bvh, &scene->materials);

    if(plane_result.t < 0.0) return object_result;
    if(object_result.t < 0.0) return plane_result;

    return object_result.t < plane_result.t ? object_result : plane_result;
}

int is_shaded(Ray *ray, Scene *scene, BVH8Tree *bvh) {
    return  is_shaded_by_plane(ray, &scene->planes) ||
            is_shaded_by_object(ray, bvh);
}

float srgb_to_linear(float x)
{
    if (x <= 0.04045)
        return x / 12.92;

    return powf((x + 0.055) / 1.055, 2.4);
}

double random_01(void) {
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, BVH8Tree *bvh, int depth) {
    HitResult hit = get_first_hit(ray, scene, bvh);

    if (hit.t < 0.0) {
        double t = 0.5 * (ray->v.z + 1.0);

        Vec horizon  = vec(0.35, 0.35, 0.35);
        Vec zenith = vec(0.90, 0.90, 0.90);

        return v_add(scale(horizon, 1.0 - t), scale(zenith, t));
    }

    double u = hit.d_u - floor(hit.d_u);
    double v = hit.d_v - floor(hit.d_v);

    int x, y, w, h, idx;
    unsigned char *ptr;

    Texture *base_color_texture = (Texture *)get_element(hit.material->base_color_map, &scene->textures);

    w = base_color_texture->w;
    h = base_color_texture->h;
    ptr = base_color_texture->ptr;

    x = clampi((int)(u * (w - 1)), 0, w - 1);
    y = clampi((int)(v * (h - 1)), 0, h - 1);
    idx = (y * w + x) * 3;

    Vec c_base = vec(
        ptr[idx + 0] / 255.0,
        ptr[idx + 1] / 255.0,
        ptr[idx + 2] / 255.0
    );

    c_base = vec(
        srgb_to_linear(c_base.x),
        srgb_to_linear(c_base.y),
        srgb_to_linear(c_base.z)
    );

    c_base = hadamard(
        c_base,
        hit.material->base_color_factor
    );

    Texture *metallic_roughness_texture = (Texture *)get_element(hit.material->metallic_roughness_map, &scene->textures);

    w = metallic_roughness_texture->w;
    h = metallic_roughness_texture->h;
    ptr = metallic_roughness_texture->ptr;

    x = clampi((int)(u * (w - 1)), 0, w - 1);
    y = clampi((int)(v * (h - 1)), 0, h - 1);
    idx = (y * w + x) * 3;

    double metallic = ptr[idx + 2] / 255.0 *hit.material->metallic;
    double roughness = ptr[idx + 1] / 255.0 * hit.material->roughness;

    Texture *occlusion_texture = (Texture *)get_element(hit.material->occlusion_map, &scene->textures);
    w = occlusion_texture->w;
    h = occlusion_texture->h;
    ptr = occlusion_texture->ptr;

    x = clampi((int)(u * (w - 1)), 0, w - 1);
    y = clampi((int)(v * (h - 1)), 0, h - 1);
    idx = (y * w + x) * 3;

    double ao = ptr[idx + 0] / 255.0;
    ao = 1.0 + hit.material->occlusion_strength * (ao - 1.0);

    Texture *emissive_texture = (Texture *)get_element(hit.material->emissive_map, &scene->textures);
    w = emissive_texture->w;
    h = emissive_texture->h;
    ptr = emissive_texture->ptr;

    x = clampi((int)(u * (w - 1)), 0, w - 1);
    y = clampi((int)(v * (h - 1)), 0, h - 1);
    idx = (y * w + x) * 3;

    Vec c_emissive = vec(
        ptr[idx + 0] / 255.0,
        ptr[idx + 1] / 255.0,
        ptr[idx + 2] / 255.0
    );

    c_emissive = vec(
        srgb_to_linear(c_emissive.x),
        srgb_to_linear(c_emissive.y),
        srgb_to_linear(c_emissive.z)
    );

    c_emissive = hadamard(
        c_emissive,
        hit.material->emissive_factor
    );

    Texture *normal_map = (Texture *)get_element(hit.material->normal_map, &scene->textures);

    w = normal_map->w;
    h = normal_map->h;
    ptr = normal_map->ptr;

    x = clampi((int)(u * (w - 1)), 0, w - 1);
    y = clampi((int)(v * (h - 1)), 0, h - 1);
    idx = (y * w + x) * 3;

    Vec nmap = vec(
        ptr[idx + 0] / 255.0 * 2.0 - 1.0,
        ptr[idx + 1] / 255.0 * 2.0 - 1.0,
        ptr[idx + 2] / 255.0 * 2.0 - 1.0
    );

    nmap.x *= hit.material->normal_scale;
    nmap.y *= hit.material->normal_scale;

    Vec ns;

    if (hit.valid_tangent) {
        ns = normalize(
            v_add(
                v_add(
                    scale(hit.tangent, nmap.x),
                    scale(hit.bitangent, nmap.y)
                ),
                scale(hit.ns, nmap.z)
            )
        );
    } else {
        ns = hit.ns;
    }


    Vec L = scene->dir_light.dir;

    Vec V = normalize(
        v_sub(cam->position, hit.point)
    );

    Vec N = ns;
    if (dot(N, V) < 0.0) N = scale(N, -1.0);

    double NdotL = fmax(0.0, dot(N, L));
    double NdotV = fmax(0.0, dot(N, V));


    Vec f0 = vec(
        (1.0 - metallic) * 0.04 + metallic * c_base.x,
        (1.0 - metallic) * 0.04 + metallic * c_base.y,
        (1.0 - metallic) * 0.04 + metallic * c_base.z
    );

    Vec H = normalize(
        v_add(L, V)
    );

    double NdotH = fmax(0.0, dot(N, H));

    double VdotH = fmax(0.0, dot(V, H));

    double factor_direct = pow(
        1.0 - VdotH,
        5.0
    );

    Vec f_direct = vec(
        f0.x + (1.0 - f0.x) * factor_direct,
        f0.y + (1.0 - f0.y) * factor_direct,
        f0.z + (1.0 - f0.z) * factor_direct
    );

    double factor_reflection = pow(
        1.0 - NdotV,
        5.0
    );

    Vec f_reflection = vec(
        f0.x + (1.0 - f0.x) * factor_reflection,
        f0.y + (1.0 - f0.y) * factor_reflection,
        f0.z + (1.0 - f0.z) * factor_reflection
    );

    // printf("metallic: %f\n", metallic);
    // printf("f0: %f, %f, %f\n", f0.x, f0.y, f0.z);
    // printf("NdotV: %f\n", NdotV);
    // printf("factor_reflection: %f\n", factor_reflection);
    // printf("f_reflection: %f, %f, %f\n", f_reflection.x, f_reflection.y, f_reflection.z);

    Vec one_minus_f = vec(
        1.0 - f_direct.x,
        1.0 - f_direct.y,
        1.0 - f_direct.z
    );

    Vec diffuse_color = scale(
        hadamard(c_base, one_minus_f),
        1.0 - metallic
    );

    Vec c_diffuse = scale(
        hadamard(
            scene->dir_light.scaled_color,
            diffuse_color
        ),
        NdotL
    );

    Vec c_specular = vec(
        0.0,
        0.0,
        0.0
    );

    if (NdotL > 0.0 && NdotV > 0.0) {
        double alpha = roughness * roughness;
        double alpha2 = alpha * alpha;

        double d =
            NdotH * NdotH * (alpha2 - 1.0) + 1.0;

        double D =
            alpha2 /
            (M_PI * d * d);

        double k =
            (roughness + 1.0) *
            (roughness + 1.0) /
            8.0;

        double Gv =
            NdotV /
            (NdotV * (1.0 - k) + k);

        double Gl =
            NdotL /
            (NdotL * (1.0 - k) + k);

        double G = Gv * Gl;

        Vec specular_brdf = scale(
            f_direct,
            D * G /
            (4.0 * NdotV * NdotL)
        );

        c_specular = scale(
            hadamard(
                scene->dir_light.scaled_color,
                specular_brdf
            ),
            NdotL
        );
    }

    Vec c_ambient = scale(
        hadamard(
            scene->global_ambient,
            c_base
        ),
        1.0 - metallic
    );

    c_ambient = scale(c_ambient, ao);

    Vec c_reflected = vec(
        0.0,
        0.0,
        0.0
    );

    if (depth > 0) {
        Vec reflection_dir = normalize(
                v_sub(
                    ray->v,
                    scale(
                        N,
                        2.0 * dot(N, ray->v)
                    )
                )
        );

        Ray reflection_ray = create_ray(
            v_add(
                hit.point,
                scale(hit.ng, EPSILON)
            ),
            reflection_dir
        );

        c_reflected = trace_ray(
            &reflection_ray,
            scene,
            cam,
            bvh,
            depth - 1
        );
    }

    // printf("f_direct: %f, %f, %f\n", f_direct.x, f_direct.y, f_direct.z);
    // printf("f_reflection: %f, %f, %f\n", f_reflection.x, f_reflection.y, f_reflection.z);
    // printf("c_reflected: %f, %f, %f\n", c_reflected.x, c_reflected.y, c_reflected.z);
    // printf("\n");

    Vec c_reflection = hadamard(
        f_reflection,
        c_reflected
    );


    Vec c = v_add(
        v_add(
            c_ambient,
            c_diffuse
        ),
        v_add(
            c_specular,
            c_reflection
        )
    );

    c = v_add(c, c_emissive);

    return c;
}
