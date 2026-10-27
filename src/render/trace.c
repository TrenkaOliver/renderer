#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "render/trace.h"
#include "render/render.h"
#include "scene/sample.h"

static inline __m256 packed_srgb_to_linear(const __m256 x) {
    
    const __m256 inv_12_92 = _mm256_set1_ps(1.0f / 12.92f);
    const __m256 low = _mm256_mul_ps(x, inv_12_92);

    const __m256 scale = _mm256_set1_ps(1.0f / 1.055f);
    const __m256 offset = _mm256_set1_ps(0.055f);
    const __m256 base = _mm256_mul_ps(
        _mm256_add_ps(x, offset),
        scale
    );

    float lanes[8];
    _mm256_storeu_ps(lanes, base);

    for (int i = 0; i < 8; i++) lanes[i] = powf(lanes[i], 2.4f);

    const __m256 high = _mm256_loadu_ps(lanes);

    const __m256 threshold = _mm256_set1_ps(0.04045f);
    const __m256 mask = _mm256_cmp_ps(x, threshold, _CMP_LE_OQ);

    return _mm256_blendv_ps(high, low, mask);
}

static inline ps_Vec read_map(Texture * const *map, __m256 u, __m256 v, const int stride)
{
    int width_arr[8];
    int max_x_arr[8], max_y_arr[8];
    float fw_arr[8], fh_arr[8];

    for (int i = 0; i < 8; i++) {
        if (map[i]) {
            width_arr[i] = map[i]->w;

            max_x_arr[i] = map[i]->w - 1;
            max_y_arr[i] = map[i]->h - 1;

            fw_arr[i] = (float)(map[i]->w - 1);
            fh_arr[i] = (float)(map[i]->h - 1);
        } else {
            width_arr[i] = 0;

            max_x_arr[i] = 0;
            max_y_arr[i] = 0;

            fw_arr[i] = 0.0f;
            fh_arr[i] = 0.0f;
        }
    }    
    
    const __m256 zero = _mm256_setzero_ps();
    const __m256 one = _mm256_set1_ps(1.0f);
    u = _mm256_max_ps(zero, _mm256_min_ps(u, one));
    v = _mm256_max_ps(zero, _mm256_min_ps(v, one));
    
    const __m256 fw = _mm256_loadu_ps(fw_arr);
    const __m256i max_x = _mm256_loadu_si256((const __m256i_u *)max_x_arr);
    const __m256i x = _mm256_max_epi32(
        _mm256_min_epi32(
            _mm256_cvttps_epi32(_mm256_mul_ps(u, fw)),
            max_x
        ),
        _mm256_setzero_si256()
    );
    
    const __m256 fh = _mm256_loadu_ps(fh_arr);
    const __m256i max_y = _mm256_loadu_si256((const __m256i_u *)max_y_arr);
    const __m256i y = _mm256_max_epi32(
        _mm256_min_epi32(
            _mm256_cvttps_epi32(_mm256_mul_ps(v, fh)),
            max_y
        ),
        _mm256_setzero_si256()
    );
    
    // Pixel index, not byte offset.
    const __m256i width = _mm256_loadu_si256((const __m256i_u *)width_arr);
    const __m256i idx = _mm256_add_epi32(
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

TraceResult packed_trace_ray(SoARay * restrict ray, const Scene * restrict scene, const Camera * restrict cam, const BVH8Tree * restrict bvh, const __m256i depth) {
    const PackedHitResult hit = packed_get_first_object(ray, bvh, &scene->materials, &scene->textures);

    const ps_Vec res = packed_sample(hit.ray.v, &scene->environment);

    int valid_hit_mask = _mm256_movemask_ps(hit.valid_hit);

    if (!valid_hit_mask)  {
        return (TraceResult) {
            .c = res,
            .ref_mul = {_mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps()},
            .valid_ref = _mm256_setzero_ps()
        };
    }    

    //calc uv coordinates
    const __m256 u = _mm256_sub_ps(hit.d_u, _mm256_floor_ps(hit.d_u));
    const __m256 v = _mm256_sub_ps(hit.d_v, _mm256_floor_ps(hit.d_v));

    //load base color
    ps_Vec c_base = read_map(hit.material.base_color_map, u, v, 3);

    c_base.x = packed_srgb_to_linear(c_base.x);
    c_base.y = packed_srgb_to_linear(c_base.y);
    c_base.z = packed_srgb_to_linear(c_base.z);

    c_base = ps_hadamard(c_base, hit.material.base_color_factor);

    //sample metallic and roughness
    const ps_Vec metallic_roughness_vec = read_map(hit.material.metallic_roughness_map, u, v, 3);

    const __m256 metallic = _mm256_mul_ps(metallic_roughness_vec.z, hit.material.metallic);
    const __m256 roughness = _mm256_mul_ps(metallic_roughness_vec.y, hit.material.roughness);

    //sample occulsion
    const ps_Vec occulsion_vec = read_map(hit.material.occlusion_map, u, v, 3);

    const __m256 _f1 = _mm256_set1_ps(1.0f);
    const __m256 ao = _mm256_fmadd_ps(hit.material.occlusion_strenght, _mm256_sub_ps(occulsion_vec.x, _f1), _f1);

    //sample emissive
    ps_Vec c_emissive = read_map(hit.material.emissive_map, u, v, 3);
    
    c_emissive.x = packed_srgb_to_linear(c_emissive.x);
    c_emissive.y = packed_srgb_to_linear(c_emissive.y);
    c_emissive.z = packed_srgb_to_linear(c_emissive.z);

    c_emissive = ps_hadamard(c_emissive, hit.material.emissive_factor);

    //sample normal
    ps_Vec nmap = read_map(hit.material.normal_map, u, v, 3);

    const __m256 _f2 = _mm256_set1_ps(2.0f);

    nmap.x = _mm256_fmsub_ps(nmap.x, _f2, _f1);
    nmap.y = _mm256_fmsub_ps(nmap.y, _f2, _f1);
    nmap.z = _mm256_fmsub_ps(nmap.z, _f2, _f1);

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

    const ps_Vec L = ps_from_vec(scene->dir_light.dir);

    const ps_Vec V = ps_normalize(ps_v_sub(ps_from_vec(cam->position), hit.point));

    ps_Vec N = ns;
    const __m256 _f0 = _mm256_setzero_ps();
    const ps_Vec neg_N = ps_scale_ss(N, -1.0f);
    const __m256 ns_mask = _mm256_cmp_ps(ps_dot(N, V), _f0, _CMP_LT_OQ);
    N.x = _mm256_blendv_ps(N.x, neg_N.x, ns_mask);
    N.y = _mm256_blendv_ps(N.y, neg_N.y, ns_mask);
    N.z = _mm256_blendv_ps(N.z, neg_N.z, ns_mask);
    
    __m256 NdotL = _mm256_max_ps(_f0, ps_dot(N, L));
    const __m256 NdotV = _mm256_max_ps(_f0, ps_dot(N, V));

    SoARay shadow_ray;

    const ps_Vec shadow_origin = ps_v_add(hit.point, ps_scale_ps(hit.ng, _mm256_set1_ps(EPSILON)));

    _mm256_store_ps(shadow_ray.ox, shadow_origin.x);
    _mm256_store_ps(shadow_ray.oy, shadow_origin.y);
    _mm256_store_ps(shadow_ray.oz, shadow_origin.z);

    _mm256_store_ps(shadow_ray.vx, L.x);
    _mm256_store_ps(shadow_ray.vy, L.y);
    _mm256_store_ps(shadow_ray.vz, L.z);

    const __m256 valid_shadow = _mm256_and_ps(
        hit.valid_hit,
        _mm256_cmp_ps(NdotL, _f0, _CMP_GT_OQ)
    );

    const __m256 visibility = packed_is_shaded(&shadow_ray, bvh, valid_shadow);

    NdotL = _mm256_and_ps(NdotL, visibility);

    const __m256 _f004 = _mm256_set1_ps(0.04f);
    const __m256 _c = _mm256_mul_ps(_mm256_sub_ps(_f1, metallic), _f004);

    const ps_Vec f0 = (ps_Vec) {
        .x = _mm256_fmadd_ps(metallic, c_base.x, _c),
        .y = _mm256_fmadd_ps(metallic, c_base.y, _c),
        .z = _mm256_fmadd_ps(metallic, c_base.z, _c)
    };

    const ps_Vec one_minus_f0 = (ps_Vec) {
        .x = _mm256_sub_ps(_f1, f0.x),
        .y = _mm256_sub_ps(_f1, f0.y),
        .z = _mm256_sub_ps(_f1, f0.z),
    };

    const ps_Vec H = ps_normalize(ps_v_add(L, V));

    const __m256 NdotH = _mm256_max_ps(_f0, ps_dot(N, H));
    const __m256 VdotH = _mm256_max_ps(_f0, ps_dot(V, H));

    __m256 _x = _mm256_sub_ps(_f1, VdotH);
    __m256 _x2 = _mm256_mul_ps(_x, _x);
    __m256 _x4 = _mm256_mul_ps(_x2, _x2);
    const __m256 factor_direct = _mm256_mul_ps(_x4, _x);

    const ps_Vec f_direct = (ps_Vec) {
        .x = _mm256_fmadd_ps(factor_direct, one_minus_f0.x, f0.x),
        .y = _mm256_fmadd_ps(factor_direct, one_minus_f0.y, f0.y),
        .z = _mm256_fmadd_ps(factor_direct, one_minus_f0.z, f0.z)
    };

    const ps_Vec one_minus_f_direct = (ps_Vec) {
        .x = _mm256_sub_ps(_f1, f_direct.x),
        .y = _mm256_sub_ps(_f1, f_direct.y),
        .z = _mm256_sub_ps(_f1, f_direct.z),
    };

    _x = _mm256_sub_ps(_f1, NdotV);
    _x2 = _mm256_mul_ps(_x, _x);
    _x4 = _mm256_mul_ps(_x2, _x2);
    const __m256 factor_reflection = _mm256_mul_ps(_x4, _x);

    const ps_Vec f_reflection = (ps_Vec) {
        .x = _mm256_fmadd_ps(factor_reflection, one_minus_f0.x, f0.x),
        .y = _mm256_fmadd_ps(factor_reflection, one_minus_f0.y, f0.y),
        .z = _mm256_fmadd_ps(factor_reflection, one_minus_f0.z, f0.z)
    };

    //calculate diffuse color

    const __m256 one_minus_metallic = _mm256_sub_ps(_f1, metallic);

    const ps_Vec diffuse_color = ps_scale_ps(
        ps_hadamard(c_base, one_minus_f_direct),
        one_minus_metallic
    );

    const ps_Vec scaled_color = ps_from_vec(scene->dir_light.scaled_color);

    const ps_Vec c_diffuse = ps_scale_ps(
        ps_hadamard(scaled_color, diffuse_color),
        NdotL
    );

    //calculate specular color

    const __m256 alpha = _mm256_mul_ps(roughness, roughness);
    const __m256 alpha2 = _mm256_mul_ps(alpha, alpha);

    const __m256 d = _mm256_fmadd_ps(_mm256_mul_ps(NdotH, NdotH), _mm256_sub_ps(alpha2, _f1), _f1);

    const __m256 _fPI = _mm256_set1_ps(M_PI); 
    const __m256 D = _mm256_mul_ps(alpha2, _mm256_rcp_ps(_mm256_mul_ps(_fPI, _mm256_mul_ps(d, d))));

    const __m256 _roughness_plus_1 = _mm256_add_ps(roughness, _f1);
    const __m256 _f8 = _mm256_set1_ps(8.0f);
    const __m256 k = _mm256_mul_ps(_mm256_mul_ps(_roughness_plus_1, _roughness_plus_1), _mm256_rcp_ps(_f8));
    
    const __m256 one_minus_k = _mm256_sub_ps(_f1, k);

    const __m256 Gv = _mm256_mul_ps(NdotV, _mm256_rcp_ps(_mm256_fmadd_ps(NdotV, one_minus_k, k)));

    const __m256 Gl = _mm256_mul_ps(NdotL, _mm256_rcp_ps(_mm256_fmadd_ps(NdotL, one_minus_k, k)));

    const __m256 G = _mm256_mul_ps(Gv, Gl);

    const __m256 _f4 = _mm256_set1_ps(4.0f);

    const ps_Vec specular_brdf = ps_scale_ps(
        f_direct,
        _mm256_mul_ps(_mm256_mul_ps(D, G), _mm256_rcp_ps(_mm256_mul_ps(_f4, _mm256_mul_ps(NdotV, NdotL))))
    );

    ps_Vec c_specular = ps_scale_ps(
        ps_hadamard(scaled_color, specular_brdf),
        NdotL
    );

    const __m256 spec_mask = _mm256_or_ps(
        _mm256_cmp_ps(NdotL, _f0, _CMP_LE_OQ),
        _mm256_cmp_ps(NdotV, _f0, _CMP_LE_OQ)
    );

    c_specular.x = _mm256_blendv_ps(c_specular.x, _f0, spec_mask);
    c_specular.y = _mm256_blendv_ps(c_specular.y, _f0, spec_mask);
    c_specular.z = _mm256_blendv_ps(c_specular.z, _f0, spec_mask);

    //calculate ibl diffuse

    const ps_Vec irradiance = packed_sample(N, &scene->irradiance);

    ps_Vec c_diffuse_ibl = ps_scale_ps(
        ps_hadamard(irradiance, c_base),
        _mm256_mul_ps(one_minus_metallic, _mm256_rcp_ps(_fPI))
    );

    c_diffuse_ibl = ps_scale_ps(c_diffuse_ibl, ao);

    //calculate ibl specular

    const __m256 valid_roughness_mask = _mm256_cmp_ps(roughness, _mm256_set1_ps(0.2f), _CMP_LE_OQ);
    const __m256 valid_metallic_mask = _mm256_cmp_ps(metallic, _mm256_set1_ps(0.5f), _CMP_GE_OQ);
    const __m256i valid_depth = _mm256_cmpgt_epi32(_mm256_set1_epi32(2), depth);

    const __m256 valid_reflection = _mm256_and_ps(_mm256_and_ps(hit.valid_hit, _mm256_castsi256_ps(valid_depth)), _mm256_and_ps(valid_roughness_mask, valid_metallic_mask));

    const int valid_reflection_mask = _mm256_movemask_ps(valid_reflection);

    const __m256 one_minus_roughness = _mm256_sub_ps(_f1, roughness);

    const __m256 o_m_r_valid = _mm256_blendv_ps(_f0, one_minus_roughness, valid_reflection);

    const __m256 reflection_strenght = _mm256_mul_ps(o_m_r_valid, o_m_r_valid);
    const __m256 one_minus_ref_str = _mm256_sub_ps(_f1, reflection_strenght);

    const ps_Vec R = ps_normalize(ps_v_sub(
        ps_scale_ps(N, _mm256_mul_ps(_f2, NdotV)), 
        V
    ));

    const ps_Vec c_prefiltered = packed_sample_prefiltered(R, roughness, scene->prefiltered);
    const ps_Vec brdf = packed_sample_brdf_lut(NdotV, roughness, &scene->brdf_lut);

    const ps_Vec specular_factor = {
        .x = _mm256_fmadd_ps(f0.x, brdf.x, brdf.y),
        .y = _mm256_fmadd_ps(f0.y, brdf.x, brdf.y),
        .z = _mm256_fmadd_ps(f0.z, brdf.x, brdf.y),
    };

    ps_Vec c_specular_ibl = ps_hadamard(c_prefiltered, specular_factor);

    c_specular_ibl = ps_scale_ps(c_specular_ibl, one_minus_ref_str);
    c_specular_ibl = ps_hadamard(f_reflection, c_specular_ibl);

    if(valid_reflection_mask) {
        ps_Vec Hr = packed_importance_sample_GGX(_f0, _f0, alpha2, N);

        const ps_Vec reflection_dir = ps_normalize(
            ps_v_sub(
                ps_scale_ps(Hr, _mm256_mul_ps(_f2, VdotH)),
                V
            )
        );

        _mm256_store_ps(ray->vx, reflection_dir.x);
        _mm256_store_ps(ray->vy, reflection_dir.y);
        _mm256_store_ps(ray->vz, reflection_dir.z);

        const ps_Vec O = ps_v_add(hit.point, ps_scale_ps(hit.ng, _mm256_set1_ps(EPSILON)));

        _mm256_store_ps(ray->ox, O.x);
        _mm256_store_ps(ray->oy, O.y);
        _mm256_store_ps(ray->oz, O.z);
    }

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

    const ps_Vec out =  {
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