#include <math.h>
#include <stdio.h>
#include <immintrin.h>

#include "geometry/triangle.h"

#define EPSILON 1e-4

__m256 packed_triangle_ray_intersection(const uint32_t idx, const ps_Vec ray_o, const ps_Vec ray_v, const RuntimeTriangle * restrict array, PackedInfo * restrict info) {
    const float * restrict arr = array->arr;
    const uint32_t off = array->offset;

    const ps_Vec a = (ps_Vec){
        .x = _mm256_loadu_ps(arr + idx),
        .y = _mm256_loadu_ps(arr + off + idx),
        .z = _mm256_loadu_ps(arr + off * 2 + idx)
    };

    const ps_Vec ab = (ps_Vec){
        .x = _mm256_loadu_ps(arr + off * 3 + idx),
        .y = _mm256_loadu_ps(arr + off * 4 + idx),
        .z = _mm256_loadu_ps(arr + off * 5 + idx)
    };

    const ps_Vec ac = (ps_Vec){
        .x = _mm256_loadu_ps(arr + off * 6 + idx),
        .y = _mm256_loadu_ps(arr + off * 7 + idx),
        .z = _mm256_loadu_ps(arr + off * 8 + idx)
    };

    const ps_Vec ao = ps_v_sub(ray_o, a);

    const ps_Vec cross_ray_v_ac = ps_cross(ray_v, ac);
    const ps_Vec cross_ao_ab = ps_cross(ao, ab);

    const __m256 denom = ps_dot(ab, cross_ray_v_ac);
    const __m256 inv_denom = _mm256_rcp_ps(denom);

    const __m256 u_num = ps_dot(ao, cross_ray_v_ac);
    const __m256 v_num = ps_dot(ray_v, cross_ao_ab);
    const __m256 t_num = ps_dot(ac, cross_ao_ab);

    const __m256 u = _mm256_mul_ps(u_num, inv_denom);
    const __m256 v = _mm256_mul_ps(v_num, inv_denom);
    const __m256 t = _mm256_mul_ps(t_num, inv_denom);
    
    info->u = u;
    info->v = v;

    const __m256 zero = _mm256_setzero_ps();
    const __m256 signbit = _mm256_set1_ps(-0.0f);

    const __m256 abs_denom = _mm256_andnot_ps(signbit, denom);

    const __m256 denom_sign = _mm256_and_ps(denom, signbit);
    const __m256 u_test = _mm256_xor_ps(u_num, denom_sign);
    const __m256 v_test = _mm256_xor_ps(v_num, denom_sign);

    const __m256 uv_sum = _mm256_add_ps(u_test, v_test);

    const __m256 valid_denom = _mm256_cmp_ps(denom, zero, _CMP_NEQ_OQ);
    const __m256 valid_u = _mm256_cmp_ps(u_test, zero, _CMP_GE_OQ);
    const __m256 valid_v = _mm256_cmp_ps(v_test, zero, _CMP_GE_OQ);
    const __m256 valid_uv = _mm256_cmp_ps(uv_sum, abs_denom, _CMP_LE_OQ);

    const __m256 valid = _mm256_and_ps(
        _mm256_and_ps(valid_denom, valid_u),
        _mm256_and_ps(valid_v, valid_uv)
    );

    return _mm256_blendv_ps(_mm256_set1_ps(-1.0f), t, valid);
}

static inline PackedMaterial create_packed_material(Material * restrict materials[8], const DynArray * restrict textures) {
    float bcfx[8], bcfy[8], bcfz[8];
    float metallic[8], roughness[8];
    float efx[8], efy[8], efz[8];
    float normal_scale[8], occlusion_strength[8];

    Texture *base_color_map[8];
    Texture *metallic_roughness_map[8];
    Texture *emissive_map[8];
    Texture *normal_map[8];
    Texture *occlusion_map[8];

    for (int i = 0; i < 8; i++) {
        if (materials[i]) {
            bcfx[i] = materials[i]->base_color_factor.x;
            bcfy[i] = materials[i]->base_color_factor.y;
            bcfz[i] = materials[i]->base_color_factor.z;

            metallic[i] = materials[i]->metallic;
            roughness[i] = materials[i]->roughness;

            efx[i] = materials[i]->emissive_factor.x;
            efy[i] = materials[i]->emissive_factor.y;
            efz[i] = materials[i]->emissive_factor.z;

            normal_scale[i] = materials[i]->normal_scale;
            occlusion_strength[i] = materials[i]->occlusion_strength;

            base_color_map[i] = (Texture *)get_element(materials[i]->base_color_map, textures);
            metallic_roughness_map[i] = (Texture *)get_element(materials[i]->metallic_roughness_map, textures);
            emissive_map[i] = (Texture *)get_element(materials[i]->emissive_map, textures);
            normal_map[i] = (Texture *)get_element(materials[i]->normal_map, textures);
            occlusion_map[i] = (Texture *)get_element(materials[i]->occlusion_map, textures);
        } else {
            bcfx[i] = 0.0f;
            bcfy[i] = 0.0f;
            bcfz[i] = 0.0f;

            metallic[i] = 0.0f;
            roughness[i] = 0.0f;

            efx[i] = 0.0f;
            efy[i] = 0.0f;
            efz[i] = 0.0f;

            normal_scale[i] = 0.0f;
            occlusion_strength[i] = 0.0f;

            base_color_map[i] = NULL;
            metallic_roughness_map[i] = NULL;
            emissive_map[i] = NULL;
            normal_map[i] = NULL;
            occlusion_map[i] = NULL;
        }
    }

    return (PackedMaterial){
        .base_color_factor = (ps_Vec){
            .x = _mm256_loadu_ps(bcfx),
            .y = _mm256_loadu_ps(bcfy),
            .z = _mm256_loadu_ps(bcfz)
        },
        .metallic = _mm256_loadu_ps(metallic),
        .roughness = _mm256_loadu_ps(roughness),
        .emissive_factor = (ps_Vec){
            .x = _mm256_loadu_ps(efx),
            .y = _mm256_loadu_ps(efy),
            .z = _mm256_loadu_ps(efz)
        },
        .normal_scale = _mm256_loadu_ps(normal_scale),
        .occlusion_strenght = _mm256_loadu_ps(occlusion_strength),
        .base_color_map = {
            base_color_map[0],
            base_color_map[1],
            base_color_map[2],
            base_color_map[3],
            base_color_map[4],
            base_color_map[5],
            base_color_map[6],
            base_color_map[7]
        },
        .metallic_roughness_map = {
            metallic_roughness_map[0],
            metallic_roughness_map[1],
            metallic_roughness_map[2],
            metallic_roughness_map[3],
            metallic_roughness_map[4],
            metallic_roughness_map[5],
            metallic_roughness_map[6],
            metallic_roughness_map[7]
        },
        .emissive_map = {
            emissive_map[0],
            emissive_map[1],
            emissive_map[2],
            emissive_map[3],
            emissive_map[4],
            emissive_map[5],
            emissive_map[6],
            emissive_map[7]
        },
        .normal_map = {
            normal_map[0],
            normal_map[1],
            normal_map[2],
            normal_map[3],
            normal_map[4],
            normal_map[5],
            normal_map[6],
            normal_map[7]
        },
        .occlusion_map = {
            occlusion_map[0],
            occlusion_map[1],
            occlusion_map[2],
            occlusion_map[3],
            occlusion_map[4],
            occlusion_map[5],
            occlusion_map[6],
            occlusion_map[7]
        }
    };
}

PackedHitResult packed_triangle_result(
const float t_arr[8], const __m256 u, const __m256 v, const uint32_t building_indices[8], const PackedRay ray, const RuntimeTriangle * restrict runtime_triangles,
const OriginalTriangle * restrict building_triangles, float **vertex_data, const DynArray * restrict materials, const DynArray * restrict textures
) {
    const __m256 t = _mm256_loadu_ps(t_arr);
    const ps_Vec p = ps_v_add(ray.o, ps_scale_ps(ray.v, t));

    uint32_t indices[8];

    float ngx[8];
    float ngy[8];
    float ngz[8];

    Material *m[8];

    float nax[8];
    float nay[8];
    float naz[8];

    float nbx[8];
    float nby[8];
    float nbz[8];

    float ncx[8];
    float ncy[8];
    float ncz[8];

    float d_u[8];
    float d_v[8];

    float ax[8];
    float ay[8];
    float az[8];

    float bx[8];
    float by[8];
    float bz[8];

    float cx[8];
    float cy[8];
    float cz[8];

    const __m256 w = _mm256_sub_ps(_mm256_set1_ps(1.0f), _mm256_add_ps(u, v));

    for (int i = 0; i < 8; i++) {
        if (t_arr[i] != -1.0f) {
            indices[i] = runtime_triangles->building_idx[building_indices[i]];

            ngx[i] = building_triangles->nx[indices[i]];
            ngy[i] = building_triangles->ny[indices[i]];
            ngz[i] = building_triangles->nz[indices[i]];

            m[i] = ((Material *)materials->ptr) + building_triangles->material[indices[i]];

            nax[i] = vertex_data[1][3 * building_triangles->ai[indices[i]] + 0];
            nay[i] = vertex_data[1][3 * building_triangles->ai[indices[i]] + 1];
            naz[i] = vertex_data[1][3 * building_triangles->ai[indices[i]] + 2];

            nbx[i] = vertex_data[1][3 * building_triangles->bi[indices[i]] + 0];
            nby[i] = vertex_data[1][3 * building_triangles->bi[indices[i]] + 1];
            nbz[i] = vertex_data[1][3 * building_triangles->bi[indices[i]] + 2];

            ncx[i] = vertex_data[1][3 * building_triangles->ci[indices[i]] + 0];
            ncy[i] = vertex_data[1][3 * building_triangles->ci[indices[i]] + 1];
            ncz[i] = vertex_data[1][3 * building_triangles->ci[indices[i]] + 2];

            d_u[i] = vertex_data[2][2 * building_triangles->ai[indices[i]] + 0] * w[i] + vertex_data[2][2 * building_triangles->bi[indices[i]] + 0] * u[i] + vertex_data[2][2 * building_triangles->ci[indices[i]] + 0] * v[i];
            d_v[i] = vertex_data[2][2 * building_triangles->ai[indices[i]] + 1] * w[i] + vertex_data[2][2 * building_triangles->bi[indices[i]] + 1] * u[i] + vertex_data[2][2 * building_triangles->ci[indices[i]] + 1] * v[i];

            ax[i] = vertex_data[0][3 * building_triangles->ai[indices[i]] + 0];
            ay[i] = vertex_data[0][3 * building_triangles->ai[indices[i]] + 1];
            az[i] = vertex_data[0][3 * building_triangles->ai[indices[i]] + 2];

            bx[i] = vertex_data[0][3 * building_triangles->bi[indices[i]] + 0];
            by[i] = vertex_data[0][3 * building_triangles->bi[indices[i]] + 1];
            bz[i] = vertex_data[0][3 * building_triangles->bi[indices[i]] + 2];

            cx[i] = vertex_data[0][3 * building_triangles->ci[indices[i]] + 0];
            cy[i] = vertex_data[0][3 * building_triangles->ci[indices[i]] + 1];
            cz[i] = vertex_data[0][3 * building_triangles->ci[indices[i]] + 2];
        } else {
            indices[i] = 0;

            ngx[i] = 0.0f;
            ngy[i] = 0.0f;
            ngz[i] = 0.0f;

            m[i] = NULL;

            nax[i] = 0.0f;
            nay[i] = 0.0f;
            naz[i] = 0.0f;

            nbx[i] = 0.0f;
            nby[i] = 0.0f;
            nbz[i] = 0.0f;

            ncx[i] = 0.0f;
            ncy[i] = 0.0f;
            ncz[i] = 0.0f;

            d_u[i] = NAN;
            d_v[i] = NAN;

            ax[i] = 0.0f;
            ay[i] = 0.0f;
            az[i] = 0.0f;

            bx[i] = 0.0f;
            by[i] = 0.0f;
            bz[i] = 0.0f;

            cx[i] = 0.0f;
            cy[i] = 0.0f;
            cz[i] = 0.0f;
        }
    }

    const ps_Vec ng = (ps_Vec){
        .x = _mm256_loadu_ps(ngx),
        .y = _mm256_loadu_ps(ngy),
        .z = _mm256_loadu_ps(ngz)
    };

    const ps_Vec na = (ps_Vec){
        .x = _mm256_loadu_ps(nax),
        .y = _mm256_loadu_ps(nay),
        .z = _mm256_loadu_ps(naz)
    };

    const ps_Vec nb = (ps_Vec){
        .x = _mm256_loadu_ps(nbx),
        .y = _mm256_loadu_ps(nby),
        .z = _mm256_loadu_ps(nbz)
    };

    const ps_Vec nc = (ps_Vec){
        .x = _mm256_loadu_ps(ncx),
        .y = _mm256_loadu_ps(ncy),
        .z = _mm256_loadu_ps(ncz)
    };

    const ps_Vec ns = ps_normalize(ps_v_add(ps_v_add(ps_scale_ps(na, w), ps_scale_ps(nb, u)), ps_scale_ps(nc, v)));

    const ps_Vec a = (ps_Vec){
        .x = _mm256_loadu_ps(ax),
        .y = _mm256_loadu_ps(ay),
        .z = _mm256_loadu_ps(az)
    };

    const ps_Vec b = (ps_Vec){
        .x = _mm256_loadu_ps(bx),
        .y = _mm256_loadu_ps(by),
        .z = _mm256_loadu_ps(bz)
    };

    const ps_Vec c = (ps_Vec){
        .x = _mm256_loadu_ps(cx),
        .y = _mm256_loadu_ps(cy),
        .z = _mm256_loadu_ps(cz)
    };

    const ps_Vec ab = ps_v_sub(b, a);
    const ps_Vec ac = ps_v_sub(c, a);

    float uv_ab_u[8], uv_ab_v[8];
    float uv_ac_u[8], uv_ac_v[8];


    for (int i = 0; i < 8; i++) {
        uv_ab_u[i] = vertex_data[2][2 * building_triangles->bi[indices[i]] + 0] - vertex_data[2][2 * building_triangles->ai[indices[i]] + 0];
        uv_ab_v[i] = vertex_data[2][2 * building_triangles->bi[indices[i]] + 1] - vertex_data[2][2 * building_triangles->ai[indices[i]] + 1];

        uv_ac_u[i] = vertex_data[2][2 * building_triangles->ci[indices[i]] + 0] - vertex_data[2][2 * building_triangles->ai[indices[i]] + 0];
        uv_ac_v[i] = vertex_data[2][2 * building_triangles->ci[indices[i]] + 1] - vertex_data[2][2 * building_triangles->ai[indices[i]] + 1];
    }

    const __m256 uv_ab[2] = {
        _mm256_loadu_ps(uv_ab_u),
        _mm256_loadu_ps(uv_ab_v)
    };

    __m256 uv_ac[2] = {
        _mm256_loadu_ps(uv_ac_u),
        _mm256_loadu_ps(uv_ac_v)
    };

    const __m256 D = _mm256_sub_ps(_mm256_mul_ps(uv_ab[0], uv_ac[1]), _mm256_mul_ps(uv_ab[1], uv_ac[0]));
    const __m256 rcp_D = _mm256_rcp_ps(D);

    const __m256 valid_tangent = _mm256_cmp_ps(_mm256_andnot_ps(_mm256_set1_ps(-0.0f), D), _mm256_set1_ps(EPSILON), _CMP_GT_OQ);

    const __m256 valid_hit = _mm256_cmp_ps(t, _mm256_set1_ps(-1.0f), _CMP_NEQ_OQ);

    const ps_Vec tangent = ps_scale_ps(
        ps_v_sub(
            ps_scale_ps(ab, uv_ac[1]),
            ps_scale_ps(ac, uv_ab[1])
        ),
        rcp_D
    );

    const ps_Vec bitangent = ps_scale_ps(
        ps_v_sub(
            ps_scale_ps(ac, uv_ab[0]),
            ps_scale_ps(ab, uv_ac[0])
        ),
        rcp_D
    );

    return (PackedHitResult){
        .ray = ray,
        .point = p, 
        .ng = ng, 
        .ns = ns, 
        .tangent = tangent, 
        .bitangent = bitangent, 
        .valid_tangent = valid_tangent, 
        .valid_hit = valid_hit,
        .t = t, 
        .material = create_packed_material(m, textures),
        .d_u = _mm256_loadu_ps(d_u), 
        .d_v = _mm256_loadu_ps(d_v)
    };
}