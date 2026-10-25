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

__m256 packed_srgb_to_linear(__m256 x) {
    const __m256 threshold = _mm256_set1_ps(0.04045f);
    const __m256 inv_12_92 = _mm256_set1_ps(1.0f / 12.92f);
    const __m256 scale = _mm256_set1_ps(1.0f / 1.055f);
    const __m256 offset = _mm256_set1_ps(0.055f);
    const __m256 exponent = _mm256_set1_ps(2.4f);

    __m256 low = _mm256_mul_ps(x, inv_12_92);

    __m256 base = _mm256_mul_ps(
        _mm256_add_ps(x, offset),
        scale
    );

    float lanes[8];
    _mm256_storeu_ps(lanes, base);

    for (int i = 0; i < 8; i++)
        lanes[i] = powf(lanes[i], 2.4f);

    __m256 high = _mm256_loadu_ps(lanes);

    __m256 mask = _mm256_cmp_ps(x, threshold, _CMP_LE_OQ);

    return _mm256_blendv_ps(high, low, mask);
}

// ps_Vec read_map(Texture *map[8], __m256 u, __m256 v, int stride) {
//     int iw_arr[8], ih_arr[8];
//     float fw_arr[8], fh_arr[8];

//     for (int i = 0; i < 8; i++) {
//         if (map[i]) {
//             iw_arr[i] = stride * map[i]->w - stride;
//             ih_arr[i] = stride * map[i]->h - stride;
        
//             fw_arr[i] = stride * map[i]->w - (float)stride;
//             fh_arr[i] = stride * map[i]->h - (float)stride;
//         } else {
//             iw_arr[i] = 0;
//             ih_arr[i] = 0;
//             fw_arr[i] = 0.0f;
//             fh_arr[i] = 0.0f;
//         }
//     }

//     __m256i iw = _mm256_loadu_si256((__m256i_u *)iw_arr);
//     __m256i ih = _mm256_loadu_si256((__m256i_u *)ih_arr);

//     __m256 fw = _mm256_load_ps(fw_arr);
//     __m256 fh = _mm256_load_ps(fh_arr);

//     __m256i _i0 = _mm256_setzero_si256();

//     __m256i x = _mm256_max_epi32(
//         _mm256_min_epi32(
//             _mm256_cvttps_epi32(_mm256_mul_ps(u, fw)),
//             iw
//         ),
//         _i0
//     );

//     __m256i y = _mm256_max_epi32(
//         _mm256_min_epi32(
//             _mm256_cvttps_epi32(_mm256_mul_ps(v, fh)),
//             ih
//         ),
//         _i0
//     );

//     __m256i idx = _mm256_add_epi32(_mm256_mullo_epi32(y, iw), x);
    
//     float r[8], g[8], b[8];
//     int offset[8];

//     _mm256_storeu_si256((__m256i_u *)offset, idx);

//     float D = 1.0f / 255.0f;

//     for (int i = 0; i < 8; i++) {
//         if (map[i]) {
//             r[i] = map[i]->ptr[offset[i] + 0] * D;
//             g[i] = map[i]->ptr[offset[i] + 1] * D;
//             b[i] = map[i]->ptr[offset[i] + 2] * D;
//         } else {
//             r[i] = 0.0f;
//             g[i] = 0.0f;
//             b[i] = 0.0f;
//         }
//     }

//     return (ps_Vec) {
//         .x = _mm256_loadu_ps(r),
//         .y = _mm256_loadu_ps(g),
//         .z = _mm256_loadu_ps(b)
//     };
// }

ps_Vec read_map(Texture *map[8], __m256 u, __m256 v, int stride)
{
    int width_arr[8], height_arr[8];
    int max_x_arr[8], max_y_arr[8];
    float fw_arr[8], fh_arr[8];

    for (int i = 0; i < 8; i++) {
        if (map[i]) {
            width_arr[i] = map[i]->w;
            height_arr[i] = map[i]->h;

            max_x_arr[i] = map[i]->w - 1;
            max_y_arr[i] = map[i]->h - 1;

            fw_arr[i] = (float)(map[i]->w - 1);
            fh_arr[i] = (float)(map[i]->h - 1);
        } else {
            width_arr[i] = 0;
            height_arr[i] = 0;

            max_x_arr[i] = 0;
            max_y_arr[i] = 0;

            fw_arr[i] = 0.0f;
            fh_arr[i] = 0.0f;
        }
    }

    __m256i width = _mm256_loadu_si256((const __m256i_u *)width_arr);
    __m256i max_x = _mm256_loadu_si256((const __m256i_u *)max_x_arr);
    __m256i max_y = _mm256_loadu_si256((const __m256i_u *)max_y_arr);

    __m256 fw = _mm256_loadu_ps(fw_arr);
    __m256 fh = _mm256_loadu_ps(fh_arr);

    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);

    u = _mm256_max_ps(zero, _mm256_min_ps(u, one));
    v = _mm256_max_ps(zero, _mm256_min_ps(v, one));

    __m256i x = _mm256_max_epi32(
        _mm256_min_epi32(
            _mm256_cvttps_epi32(_mm256_mul_ps(u, fw)),
            max_x
        ),
        _mm256_setzero_si256()
    );

    __m256i y = _mm256_max_epi32(
        _mm256_min_epi32(
            _mm256_cvttps_epi32(_mm256_mul_ps(v, fh)),
            max_y
        ),
        _mm256_setzero_si256()
    );

    // Pixel index, not byte offset.
    __m256i idx = _mm256_add_epi32(
        _mm256_mullo_epi32(y, width),
        x
    );

    int offset[8];
    _mm256_storeu_si256((__m256i_u *)offset, idx);

    float r[8], g[8], b[8];
    const float inv_255 = 1.0f / 255.0f;

    for (int i = 0; i < 8; i++) {
        if (map[i]) {
            size_t byte_offset = (size_t)offset[i] * stride;

            r[i] = map[i]->ptr[byte_offset + 0] * inv_255;
            g[i] = map[i]->ptr[byte_offset + 1] * inv_255;
            b[i] = map[i]->ptr[byte_offset + 2] * inv_255;
        } else {
            r[i] = 0.0f;
            g[i] = 0.0f;
            b[i] = 0.0f;
        }
    }

    return (ps_Vec) {
        .x = _mm256_loadu_ps(r),
        .y = _mm256_loadu_ps(g),
        .z = _mm256_loadu_ps(b)
    };
}




TraceResult packed_trace_ray(Ray ray[8], Scene *scene, Camera *cam, BVH8Tree *bvh, __m256i depth) {
    PackedHitResult hit = packed_get_first_object(ray, bvh, &scene->materials, &scene->textures);
    
    // return (ps_Vec) {
    //     hit.t,
    //     hit.valid_hit,
    //     hit.valid_tangent
    // };
    //sample environment for no hits
    ps_Vec res = packed_sample(hit.ray.v, &scene->environment);

    //calc uv coordinates
    __m256 u = _mm256_sub_ps(hit.d_u, _mm256_floor_ps(hit.d_u));
    __m256 v = _mm256_sub_ps(hit.d_v, _mm256_floor_ps(hit.d_v));

    //load base color
    ps_Vec c_base = read_map(hit.material.base_color_map, u, v, 3);

    c_base = (ps_Vec) {
        .x = packed_srgb_to_linear(c_base.x),
        .y = packed_srgb_to_linear(c_base.y),
        .z = packed_srgb_to_linear(c_base.z),
    };

    c_base = ps_hadamard(c_base, hit.material.base_color_factor);

    //sample metallic and roughness
    ps_Vec metallic_roughness_vec = read_map(hit.material.metallic_roughness_map, u, v, 3);

    __m256 metallic = _mm256_mul_ps(metallic_roughness_vec.z, hit.material.metallic);
    __m256 roughness = _mm256_mul_ps(metallic_roughness_vec.y, hit.material.roughness);

    //sample occulsion
    ps_Vec occulsion_vec = read_map(hit.material.occlusion_map, u, v, 3);

    __m256 _f1 = _mm256_set1_ps(1.0f);
    __m256 ao = _mm256_fmadd_ps(hit.material.occlusion_strenght, _mm256_sub_ps(occulsion_vec.x, _f1), _f1);

    //sample emissive
    ps_Vec c_emissive = read_map(hit.material.emissive_map, u, v, 3);
    
    c_emissive = (ps_Vec) {
        .x = packed_srgb_to_linear(c_emissive.x),
        .y = packed_srgb_to_linear(c_emissive.y),
        .z = packed_srgb_to_linear(c_emissive.z),
    };

    c_emissive = ps_hadamard(c_emissive, hit.material.emissive_factor);

    //sample normal
    ps_Vec nmap = read_map(hit.material.normal_map, u, v, 3);

    __m256 _f2 = _mm256_set1_ps(2.0f);

    nmap = (ps_Vec) {
        _mm256_fmsub_ps(nmap.x, _f2, _f1),
        _mm256_fmsub_ps(nmap.y, _f2, _f1),
        _mm256_fmsub_ps(nmap.z, _f2, _f1),
    };

    nmap.x = _mm256_mul_ps(nmap.x, hit.material.normal_scale);
    nmap.y = _mm256_mul_ps(nmap.y, hit.material.normal_scale);

    ps_Vec ns = ps_normalize(
        ps_v_add(
            ps_v_add(
                ps_scale_ps(hit.tangent, nmap.x),
                ps_scale_ps(hit.bitangent, nmap.y)
            ),
            ps_scale_ps(hit.ns, nmap.z)
        )
    );

    ns.x = _mm256_blendv_ps(ns.x, hit.ns.x, hit.valid_tangent);
    ns.y = _mm256_blendv_ps(ns.y, hit.ns.y, hit.valid_tangent);
    ns.z = _mm256_blendv_ps(ns.z, hit.ns.z, hit.valid_tangent);

    //pre calculations

    ps_Vec L = ps_from_vec(scene->dir_light.dir);

    ps_Vec V = ps_normalize(ps_v_sub(ps_from_vec(cam->position), hit.point));

    ps_Vec N = ns;
    __m256 _f0 = _mm256_setzero_ps();
    __m256 mask = _mm256_cmp_ps(ps_dot(N, V), _f0, _CMP_LT_OQ);
    ps_Vec neg_N = ps_scale_ss(N, -1.0f);
    N.x = _mm256_blendv_ps(N.x, neg_N.x, mask);
    N.y = _mm256_blendv_ps(N.y, neg_N.y, mask);
    N.z = _mm256_blendv_ps(N.z, neg_N.z, mask);

    __m256 NdotL = _mm256_max_ps(_f0, ps_dot(N, L));
    __m256 NdotV = _mm256_max_ps(_f0, ps_dot(N, V));

    __m256 _f004 = _mm256_set1_ps(0.04f);
    __m256 _c = _mm256_mul_ps(_mm256_sub_ps(_f1, metallic), _f004);

    ps_Vec f0 = (ps_Vec) {
        .x = _mm256_fmadd_ps(metallic, c_base.x, _c),
        .y = _mm256_fmadd_ps(metallic, c_base.y, _c),
        .z = _mm256_fmadd_ps(metallic, c_base.z, _c)
    };

    ps_Vec one_minus_f0 = (ps_Vec) {
        .x = _mm256_sub_ps(_f1, f0.x),
        .y = _mm256_sub_ps(_f1, f0.y),
        .z = _mm256_sub_ps(_f1, f0.z),
    };

    ps_Vec H = ps_normalize(ps_v_add(L, V));

    __m256 NdotH = _mm256_max_ps(_f0, ps_dot(N, H));
    __m256 VdotH = _mm256_max_ps(_f0, ps_dot(V, H));

    __m256 _x = _mm256_sub_ps(_f1, VdotH);
    __m256 _x2 = _mm256_mul_ps(_x, _x);
    __m256 _x4 = _mm256_mul_ps(_x2, _x2);
    __m256 factor_direct = _mm256_mul_ps(_x4, _x);

    ps_Vec f_direct = (ps_Vec) {
        .x = _mm256_fmadd_ps(factor_direct, one_minus_f0.x, f0.x),
        .y = _mm256_fmadd_ps(factor_direct, one_minus_f0.y, f0.y),
        .z = _mm256_fmadd_ps(factor_direct, one_minus_f0.z, f0.z)
    };

    ps_Vec one_minus_f_direct = (ps_Vec) {
        .x = _mm256_sub_ps(_f1, f_direct.x),
        .y = _mm256_sub_ps(_f1, f_direct.y),
        .z = _mm256_sub_ps(_f1, f_direct.z),
    };

    _x = _mm256_sub_ps(_f1, NdotV);
    _x2 = _mm256_mul_ps(_x, _x);
    _x4 = _mm256_mul_ps(_x2, _x2);
    __m256 factor_reflection = _mm256_mul_ps(_x4, _x);

    ps_Vec f_reflection = (ps_Vec) {
        .x = _mm256_fmadd_ps(factor_reflection, one_minus_f0.x, f0.x),
        .y = _mm256_fmadd_ps(factor_reflection, one_minus_f0.y, f0.y),
        .z = _mm256_fmadd_ps(factor_reflection, one_minus_f0.z, f0.z)
    };

    //calculate diffuse color

    __m256 one_minus_metallic = _mm256_sub_ps(_f1, metallic);

    ps_Vec diffuse_color = ps_scale_ps(
        ps_hadamard(c_base, one_minus_f_direct),
        one_minus_metallic
    );

    ps_Vec scaled_color = ps_from_vec(scene->dir_light.scaled_color);

    ps_Vec c_diffuse = ps_scale_ps(
        ps_hadamard(scaled_color, diffuse_color),
        NdotL
    );

    //calculate specular color

    __m256 alpha = _mm256_mul_ps(roughness, roughness);
    __m256 alpha2 = _mm256_mul_ps(alpha, alpha);

    __m256 d = _mm256_fmadd_ps(_mm256_mul_ps(NdotH, NdotH), _mm256_sub_ps(alpha2, _f1), _f1);

    __m256 _fPI = _mm256_set1_ps(M_PI); 
    // __m256 D = _mm256_div_ps(alpha2, _mm256_mul_ps(_fPI, _mm256_mul_ps(d, d)));
    __m256 D = _mm256_mul_ps(alpha2, _mm256_rcp_ps(_mm256_mul_ps(_fPI, _mm256_mul_ps(d, d))));

    __m256 _roughness_plus_1 = _mm256_add_ps(roughness, _f1);
    __m256 _f8 = _mm256_set1_ps(8.0f);
    // __m256 k = _mm256_div_ps(_mm256_mul_ps(_roughness_plus_1, _roughness_plus_1), _f8);
    __m256 k = _mm256_mul_ps(_mm256_mul_ps(_roughness_plus_1, _roughness_plus_1), _mm256_rcp_ps(_f8));
    
    __m256 one_minus_k = _mm256_sub_ps(_f1, k);

    // __m256 Gv = _mm256_div_ps(NdotV, _mm256_fmadd_ps(NdotV, one_minus_k, k));
    __m256 Gv = _mm256_mul_ps(NdotV, _mm256_rcp_ps(_mm256_fmadd_ps(NdotV, one_minus_k, k)));

    // __m256 Gl = _mm256_div_ps(NdotL, _mm256_fmadd_ps(NdotL, one_minus_k, k));
    __m256 Gl = _mm256_mul_ps(NdotL, _mm256_rcp_ps(_mm256_fmadd_ps(NdotL, one_minus_k, k)));

    __m256 G = _mm256_mul_ps(Gv, Gl);

    __m256 _f4 = _mm256_set1_ps(4.0f);

    ps_Vec specular_brdf = ps_scale_ps(
        f_direct,
        // _mm256_div_ps(_mm256_mul_ps(D, G), _mm256_mul_ps(_f4, _mm256_mul_ps(NdotV, NdotL)))
        _mm256_mul_ps(_mm256_mul_ps(D, G), _mm256_rcp_ps(_mm256_mul_ps(_f4, _mm256_mul_ps(NdotV, NdotL))))
    );

    ps_Vec c_specular = ps_scale_ps(
        ps_hadamard(scaled_color, specular_brdf),
        NdotL
    );

    mask = _mm256_or_ps(
        _mm256_cmp_ps(NdotL, _f0, _CMP_LE_OQ),
        _mm256_cmp_ps(NdotV, _f0, _CMP_LE_OQ)
    );

    c_specular = (ps_Vec) {
        .x = _mm256_blendv_ps(c_specular.x, _f0, mask),
        .y = _mm256_blendv_ps(c_specular.y, _f0, mask),
        .z = _mm256_blendv_ps(c_specular.z, _f0, mask)
    };

    //calculate ibl diffuse

    ps_Vec irradiance = packed_sample(N, &scene->irradiance);

    ps_Vec c_diffuse_ibl = ps_scale_ps(
        ps_hadamard(irradiance, c_base),
        // _mm256_div_ps(one_minus_metallic, _fPI)
        _mm256_mul_ps(one_minus_metallic, _mm256_rcp_ps(_fPI))
    );

    c_diffuse_ibl = ps_scale_ps(c_diffuse_ibl, ao);

    //calculate ibl specular

    ps_Vec R = ps_normalize(ps_v_sub(
        ps_scale_ps(N, _mm256_mul_ps(_f2, NdotV)), 
        V
    ));

    ps_Vec c_prefiltered = packed_sample_prefiltered(R, roughness, scene->prefiltered);
    ps_Vec brdf = packed_sample_brdf_lut(NdotV, roughness, &scene->brdf_lut);

    ps_Vec specular_factor = {
        .x = _mm256_fmadd_ps(f0.x, brdf.x, brdf.y),
        .y = _mm256_fmadd_ps(f0.y, brdf.x, brdf.y),
        .z = _mm256_fmadd_ps(f0.z, brdf.x, brdf.y),
    };

    ps_Vec c_specular_ibl = ps_hadamard(c_prefiltered, specular_factor);

    __m256 valid_roughness_mask = _mm256_cmp_ps(roughness, _mm256_set1_ps(0.2f), _CMP_LE_OQ);
    __m256 valid_metallic_mask = _mm256_cmp_ps(metallic, _mm256_set1_ps(0.5f), _CMP_GE_OQ);
    __m256i valid_depth = _mm256_cmpgt_epi32(_mm256_set1_epi32(2), depth);

    __m256 valid_reflection = _mm256_and_ps(_mm256_and_ps(hit.valid_hit, _mm256_castsi256_ps(valid_depth)), _mm256_and_ps(valid_roughness_mask, valid_metallic_mask));

    __m256 one_minus_roughness = _mm256_sub_ps(_f1, roughness);

    __m256 o_m_r_valid = _mm256_blendv_ps(_f0, one_minus_roughness, valid_reflection);

    __m256 reflection_strenght = _mm256_mul_ps(o_m_r_valid, o_m_r_valid);
    __m256 one_minus_ref_str = _mm256_sub_ps(_f1, reflection_strenght);

    c_specular_ibl = ps_scale_ps(c_specular_ibl, one_minus_ref_str);
    c_specular_ibl = ps_hadamard(f_reflection, c_specular_ibl);

    ps_Vec Hr = packed_importance_sample_GGX(_f0, _f0, alpha2, N);

    ps_Vec reflection_dir = ps_normalize(
        ps_v_sub(
            ps_scale_ps(Hr, _mm256_mul_ps(_f2, VdotH)),
            V
        )
    );

    ps_Vec O = ps_v_add(hit.point, ps_scale_ps(hit.ng, _mm256_set1_ps(EPSILON)));

    float dx[8], dy[8], dz[8], ox[8], oy[8], oz[8];
    _mm256_storeu_ps(dx, reflection_dir.x);
    _mm256_storeu_ps(dy, reflection_dir.y);
    _mm256_storeu_ps(dz, reflection_dir.z);
    _mm256_storeu_ps(ox, O.x);
    _mm256_storeu_ps(oy, O.y);
    _mm256_storeu_ps(oz, O.z);

    for (int i = 0; i < 8; i++) {
        ray[i] = create_ray(vec(ox[i], oy[i], oz[i]), vec(dx[i], dy[i], dz[i]));
    }

    //ignore recursive reflections for now
    // ps_Vec c_reflection_or_specular_ibl = c_specular_ibl;

    // __m256 valid_roughness_mask = _mm256_cmp_ps(roughness, _mm256_set1_ps(0.2f), _CMP_LE_OQ);
    // __m256 valid_metallic_mask = _mm256_cmp_ps(metallic, _mm256_set1_ps(0.5f), _CMP_GE_OQ);

    // __m256 valid_reflection = _mm256_and_ps(hit.valid_hit, _mm256_and_ps(valid_roughness_mask, valid_metallic_mask));

    // int valid_roughness = _mm256_movemask_ps(valid_roughness_mask);
    // int valid_metallic = _mm256_movemask_ps(valid_metallic_mask);
    // int valid_hit = _mm256_movemask_ps(hit.valid_hit);


    // if (depth > 0 && valid_roughness && valid_metallic && valid_hit) {
    //     double sd_u, sd_v;
    //     hammersley(0, 1, &sd_u, &sd_v);
    //     __m256 u = _mm256_set1_ps(sd_u);
    //     __m256 v = _mm256_set1_ps(sd_v);

    //     ps_Vec H = packed_importance_sample_GGX(u, v, alpha2, N);

    //     ps_Vec reflection_dir = ps_normalize(
    //         ps_v_sub(
    //             ps_scale_ps(H, _mm256_mul_ps(_f2, VdotH)),
    //             V
    //         )
    //     );

    //     ps_Vec O = ps_v_add(hit.point, ps_scale_ps(hit.ng, _mm256_set1_ps(EPSILON)));
        
    //     float dx[8], dy[8], dz[8], ox[8], oy[8], oz[8];
    //     _mm256_storeu_ps(dx, reflection_dir.x);
    //     _mm256_storeu_ps(dy, reflection_dir.y);
    //     _mm256_storeu_ps(dz, reflection_dir.z);
    //     _mm256_storeu_ps(ox, O.x);
    //     _mm256_storeu_ps(oy, O.y);
    //     _mm256_storeu_ps(oz, O.z);

    //     for (int i = 0; i < 8; i++) {
    //         ray[i] = create_ray(vec(ox[i], oy[i], oz[i]), vec(dx[i], dy[i], dz[i]));
    //     }

    //     ps_Vec c_reflected = packed_trace_ray(ray, scene, cam, bvh, depth - 1);

    //     __m256 one_minus_roughness = _mm256_sub_ps(_f1, roughness);
    //     __m256 reflection_strenght = _mm256_mul_ps(one_minus_roughness, one_minus_roughness);
    //     __m256 one_minus_ref_str = _mm256_sub_ps(_f1, reflection_strenght);

    //     ps_Vec reflected_scale = ps_scale_ps(c_reflected, reflection_strenght);
    //     ps_Vec specular_scale = ps_scale_ps(c_specular_ibl, one_minus_ref_str);

    //     c_reflection_or_specular_ibl = ps_v_add(reflected_scale, specular_scale);

    //     c_reflection_or_specular_ibl = ps_hadamard(f_reflection, c_reflection_or_specular_ibl);

    //     c_reflection_or_specular_ibl.x = _mm256_blendv_ps(c_specular_ibl.x, c_reflection_or_specular_ibl.x, valid_reflection);
    //     c_reflection_or_specular_ibl.y = _mm256_blendv_ps(c_specular_ibl.y, c_reflection_or_specular_ibl.y, valid_reflection);
    //     c_reflection_or_specular_ibl.z = _mm256_blendv_ps(c_specular_ibl.z, c_reflection_or_specular_ibl.z, valid_reflection);

    // }


    ps_Vec c = {
        .x = _f0,
        .y = _f0,
        .z = _f0
    };

    c = ps_v_add(c, c_diffuse);
    c = ps_v_add(c, c_diffuse_ibl);
    c = ps_v_add(c, c_specular);
    c = ps_v_add(c, c_specular_ibl);
    c = ps_v_add(c, c_emissive);

    ps_Vec out =  {
        .x = _mm256_blendv_ps(res.x, c.x, hit.valid_hit),
        .y = _mm256_blendv_ps(res.y, c.y, hit.valid_hit),
        .z = _mm256_blendv_ps(res.z, c.z, hit.valid_hit),
    };

    return (TraceResult) {
        .c = out,
        .ref_mul = ps_scale_ps(f_reflection, reflection_strenght),
        .valid_ref = valid_reflection
    };
}

Vec trace_ray(Ray *ray, Scene *scene, Camera *cam, BVH8Tree *bvh, int depth) {
    // HitResult hit = get_first_hit(ray, scene, bvh);

    // if (hit.t < 0.0) {
    //     return sample(ray->v, &scene->environment);
    // }

    // double u = hit.d_u - floor(hit.d_u);
    // double v = hit.d_v - floor(hit.d_v);

    // int x, y, w, h, idx;
    // unsigned char *ptr;

    // Texture *base_color_texture = (Texture *)get_element(hit.material->base_color_map, &scene->textures);

    // w = base_color_texture->w;
    // h = base_color_texture->h;
    // ptr = base_color_texture->ptr;

    // x = clampi((int)(u * (w - 1)), 0, w - 1);
    // y = clampi((int)(v * (h - 1)), 0, h - 1);
    // idx = (y * w + x) * 3;

    // Vec c_base = vec(
    //     ptr[idx + 0] / 255.0,
    //     ptr[idx + 1] / 255.0,
    //     ptr[idx + 2] / 255.0
    // );

    // c_base = vec(
    //     srgb_to_linear(c_base.x),
    //     srgb_to_linear(c_base.y),
    //     srgb_to_linear(c_base.z)
    // );

    // c_base = hadamard(
    //     c_base,
    //     hit.material->base_color_factor
    // );

    // Texture *metallic_roughness_texture = (Texture *)get_element(hit.material->metallic_roughness_map, &scene->textures);

    // w = metallic_roughness_texture->w;
    // h = metallic_roughness_texture->h;
    // ptr = metallic_roughness_texture->ptr;

    // x = clampi((int)(u * (w - 1)), 0, w - 1);
    // y = clampi((int)(v * (h - 1)), 0, h - 1);
    // idx = (y * w + x) * 3;

    // double metallic = ptr[idx + 2] / 255.0 *hit.material->metallic;
    // double roughness = ptr[idx + 1] / 255.0 * hit.material->roughness;

    // Texture *occlusion_texture = (Texture *)get_element(hit.material->occlusion_map, &scene->textures);
    // w = occlusion_texture->w;
    // h = occlusion_texture->h;
    // ptr = occlusion_texture->ptr;

    // x = clampi((int)(u * (w - 1)), 0, w - 1);
    // y = clampi((int)(v * (h - 1)), 0, h - 1);
    // idx = (y * w + x) * 3;

    // double ao = ptr[idx + 0] / 255.0;
    // ao = 1.0 + hit.material->occlusion_strength * (ao - 1.0);

    // Texture *emissive_texture = (Texture *)get_element(hit.material->emissive_map, &scene->textures);
    // w = emissive_texture->w;
    // h = emissive_texture->h;
    // ptr = emissive_texture->ptr;

    // x = clampi((int)(u * (w - 1)), 0, w - 1);
    // y = clampi((int)(v * (h - 1)), 0, h - 1);
    // idx = (y * w + x) * 3;

    // Vec c_emissive = vec(
    //     ptr[idx + 0] / 255.0,
    //     ptr[idx + 1] / 255.0,
    //     ptr[idx + 2] / 255.0
    // );

    // c_emissive = vec(
    //     srgb_to_linear(c_emissive.x),
    //     srgb_to_linear(c_emissive.y),
    //     srgb_to_linear(c_emissive.z)
    // );

    // c_emissive = hadamard(
    //     c_emissive,
    //     hit.material->emissive_factor
    // );

    // Texture *normal_map = (Texture *)get_element(hit.material->normal_map, &scene->textures);

    // w = normal_map->w;
    // h = normal_map->h;
    // ptr = normal_map->ptr;

    // x = clampi((int)(u * (w - 1)), 0, w - 1);
    // y = clampi((int)(v * (h - 1)), 0, h - 1);
    // idx = (y * w + x) * 3;

    // Vec nmap = vec(
    //     ptr[idx + 0] / 255.0 * 2.0 - 1.0,
    //     ptr[idx + 1] / 255.0 * 2.0 - 1.0,
    //     ptr[idx + 2] / 255.0 * 2.0 - 1.0
    // );

    // nmap.x *= hit.material->normal_scale;
    // nmap.y *= hit.material->normal_scale;

    // Vec ns;

    // if (hit.valid_tangent) {
    //     ns = normalize(
    //         v_add(
    //             v_add(
    //                 scale(hit.tangent, nmap.x),
    //                 scale(hit.bitangent, nmap.y)
    //             ),
    //             scale(hit.ns, nmap.z)
    //         )
    //     );
    // } else {
    //     ns = hit.ns;
    // }


    // Vec L = scene->dir_light.dir;

    // Vec V = normalize(
    //     v_sub(cam->position, hit.point)
    // );

    // Vec N = ns;
    // if (dot(N, V) < 0.0) N = scale(N, -1.0);

    // double NdotL = fmax(0.0, dot(N, L));
    // double NdotV = fmax(0.0, dot(N, V));


    // Vec f0 = vec(
    //     (1.0 - metallic) * 0.04 + metallic * c_base.x,
    //     (1.0 - metallic) * 0.04 + metallic * c_base.y,
    //     (1.0 - metallic) * 0.04 + metallic * c_base.z
    // );

    // Vec H = normalize(
    //     v_add(L, V)
    // );

    // double NdotH = fmax(0.0, dot(N, H));

    // double VdotH = fmax(0.0, dot(V, H));

    // double factor_direct = pow(
    //     1.0 - VdotH,
    //     5.0
    // );

    // Vec f_direct = vec(
    //     f0.x + (1.0 - f0.x) * factor_direct,
    //     f0.y + (1.0 - f0.y) * factor_direct,
    //     f0.z + (1.0 - f0.z) * factor_direct
    // );

    // double factor_reflection = pow(
    //     1.0 - NdotV,
    //     5.0
    // );

    // Vec f_reflection = vec(
    //     f0.x + (1.0 - f0.x) * factor_reflection,
    //     f0.y + (1.0 - f0.y) * factor_reflection,
    //     f0.z + (1.0 - f0.z) * factor_reflection
    // );

    // Vec one_minus_f = vec(
    //     1.0 - f_direct.x,
    //     1.0 - f_direct.y,
    //     1.0 - f_direct.z
    // );

    // Vec diffuse_color = scale(
    //     hadamard(c_base, one_minus_f),
    //     1.0 - metallic
    // );

    // Vec c_diffuse = scale(
    //     hadamard(
    //         scene->dir_light.scaled_color,
    //         diffuse_color
    //     ),
    //     NdotL
    // );

    // Vec c_specular = vec(
    //     0.0,
    //     0.0,
    //     0.0
    // );

    // if (NdotL > 0.0 && NdotV > 0.0) {
    //     double alpha = roughness * roughness;
    //     double alpha2 = alpha * alpha;

    //     double d =
    //         NdotH * NdotH * (alpha2 - 1.0) + 1.0;

    //     double D =
    //         alpha2 /
    //         (M_PI * d * d);

    //     double k =
    //         (roughness + 1.0) *
    //         (roughness + 1.0) /
    //         8.0;

    //     double Gv =
    //         NdotV /
    //         (NdotV * (1.0 - k) + k);

    //     double Gl =
    //         NdotL /
    //         (NdotL * (1.0 - k) + k);

    //     double G = Gv * Gl;

    //     Vec specular_brdf = scale(
    //         f_direct,
    //         D * G /
    //         (4.0 * NdotV * NdotL)
    //     );

    //     c_specular = scale(
    //         hadamard(
    //             scene->dir_light.scaled_color,
    //             specular_brdf
    //         ),
    //         NdotL
    //     );
    // }

    // Vec irradiance = sample(N, &scene->irradiance);

    // Vec c_diffuse_ibl = scale(
    //     hadamard(irradiance, c_base),
    //     (1.0 - metallic) / M_PI
    // );

    // c_diffuse_ibl = scale(c_diffuse_ibl, ao);

    // Vec R = normalize(
    //     v_sub(
    //         scale(N, 2.0 * NdotV),
    //         V
    //     )
    // );

    // Vec c_prefiltered = sample_prefiltered(R, roughness, scene->prefiltered);

    // Vec brdf = sample_brdf_lut(NdotV, roughness, &scene->brdf_lut);

    // Vec specular_factor = vec(
    //     f0.x * brdf.x + brdf.y,
    //     f0.y * brdf.x + brdf.y,
    //     f0.z * brdf.x + brdf.y
    // );

    // Vec c_specular_ibl = hadamard(
    //     c_prefiltered,
    //     specular_factor
    // );

    // Vec c_reflection_or_specular_ibl;

    // if (depth > 0 && roughness < 0.2 && metallic > 0.5) {
    //     Vec c_reflected = vec(0.0, 0.0, 0.0);

    //     int sample_count = 1;

    //     for (int i = 0; i < sample_count; i++) {
    //         double u, v;
    //         hammersley(i, sample_count, &u, &v);
    
    //         Vec H = importance_sample_GGX(u, v, roughness, N);
    
    //         Vec reflection_dir = normalize(
    //             v_sub(
    //                 scale(H, 2.0 * dot(V, H)),
    //                 V
    //             )
    //         );
    
    //         Ray reflection_ray = create_ray(
    //             v_add(hit.point, scale(hit.ng, EPSILON)),
    //             reflection_dir
    //         );
    
    //         Vec sample = trace_ray(
    //             &reflection_ray,
    //             scene,
    //             cam,
    //             bvh,
    //             depth - 1
    //         );

    //         c_reflected = v_add(c_reflected, sample);
    //     }

    //     c_reflected = scale(c_reflected, 1.0 / sample_count);

    //     double reflection_strenght = (1.0 - roughness) * (1.0 - roughness);

    //     c_reflection_or_specular_ibl = v_add(
    //         scale(c_reflected, reflection_strenght),
    //         scale(c_specular_ibl, 1.0 - reflection_strenght)
    //     );

    //     c_reflection_or_specular_ibl = hadamard(
    //         f_reflection,
    //         c_reflection_or_specular_ibl
    //     );

    // } else {
    //     c_reflection_or_specular_ibl = c_specular_ibl;
    // }


    // Ray shadow_ray = create_ray(
    //     v_add(hit.point, scale(hit.ng, EPSILON)),
    //     L
    // );

    // Vec c = vec(0.0, 0.0, 0.0);

    // if (!is_shaded(&shadow_ray, scene, bvh)) {

    // }

    // c = v_add(c, c_diffuse);
    // c = v_add(c, c_diffuse_ibl);
    // c = v_add(c, c_reflection_or_specular_ibl);
    // c = v_add(c, c_specular);
    // c = v_add(c, c_emissive);

    // return c;
}
