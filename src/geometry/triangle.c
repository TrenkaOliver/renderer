#include <math.h>
#include <stdio.h>
#include <immintrin.h>

#include "geometry/object.h"

#define EPSILON 1e-4

__m256 packed_triangle_ray_intersection(uint32_t idx, PackedRay *ray, RuntimeTriangle *array, PackedInfo *info) {
    ps_Vec a = (ps_Vec){
        .x = _mm256_loadu_ps(array->arr + idx),
        .y = _mm256_loadu_ps(array->arr + array->offset + idx),
        .z = _mm256_loadu_ps(array->arr + array->offset * 2 + idx)
    };
    
    ps_Vec ab = (ps_Vec){
        .x = _mm256_loadu_ps(array->arr + array->offset * 3 + idx),
        .y = _mm256_loadu_ps(array->arr + array->offset * 4 + idx),
        .z = _mm256_loadu_ps(array->arr + array->offset * 5 + idx)
    };
    
    ps_Vec ac = (ps_Vec){
        .x = _mm256_loadu_ps(array->arr + array->offset * 6 + idx),
        .y = _mm256_loadu_ps(array->arr + array->offset * 7 + idx),
        .z = _mm256_loadu_ps(array->arr + array->offset * 8 + idx)
    };
    
    ps_Vec ao = ps_v_sub(ray->o, a);
    
    ps_Vec cross_ao_ab = ps_cross(ao, ab);
    ps_Vec cross_ray_v_ac = ps_cross(ray->v, ac);
    
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);

    __m256 denom = ps_dot(ab, cross_ray_v_ac);
    __m256 inv_denom = _mm256_rcp_ps(denom);
    
    __m256 _eps = _mm256_set1_ps(EPSILON);
    __m256 _negzero = _mm256_set1_ps(-0.0f);

    __m256 valid = _mm256_cmp_ps(_mm256_andnot_ps(_negzero, denom), _eps, _CMP_GE_OQ);
    
    __m256 u = _mm256_mul_ps(ps_dot(ao, cross_ray_v_ac), inv_denom);
    valid = _mm256_and_ps(valid, _mm256_cmp_ps(u, zero, _CMP_GE_OQ));

    __m256 v = _mm256_mul_ps(ps_dot(ray->v, cross_ao_ab), inv_denom);
    valid = _mm256_and_ps(valid, _mm256_cmp_ps(v, zero, _CMP_GE_OQ));

    valid = _mm256_and_ps(valid, _mm256_cmp_ps(_mm256_add_ps(u, v), one, _CMP_LE_OQ));

    __m256 t = _mm256_mul_ps(ps_dot(ac, cross_ao_ab), inv_denom);

    info->u = u;
    info->v = v;

    return _mm256_blendv_ps(_mm256_set1_ps(-1.0f), t, valid);
}

double triangle_ray_intersection(Object *object, Ray *ray, Info *info) {
    Vec ab, ac, ao;
    double denom, u, v, t;

    ab = v_sub(object->type.triangle.b, object->type.triangle.a);
    ac = v_sub(object->type.triangle.c, object->type.triangle.a);
    ao = v_sub(ray->o, object->type.triangle.a);

    denom = dot(ab, cross(ray->v, ac));
    if (fabs(denom) < EPSILON) return -1.0;

    u = dot(ao, cross(ray->v, ac)) / denom;
    if (u < 0.0) return -1.0;

    v = dot(ray->v, cross(ao, ab)) / denom;
    if (v < 0.0) return -1.0;

    if (u + v > 1.0) return -1.0;

    t = dot(ac, cross(ao, ab)) / denom;
    info->u = u;
    info->v = v;
    info->w = 1 - u - v;
    
    return t;
}



HitResult get_triangle_result(Ray *ray, Object *object, Info *info, double t) {
    // Vec p, ns;
    // double d_u, d_v;

    // p = v_add(ray->o, scale(ray->v, t));
    
    // ns = object->type.triangle.const_normal
    // ? object->type.triangle.ng 
    // : normalize(v_add(v_add(scale(object->type.triangle.na, info->w), scale(object->type.triangle.nb, info->u)), scale(object->type.triangle.nc, info->v)));

    // if (object->material->diffuse_map != (size_t)-1) {
    //     d_u = object->type.triangle.ta.x * info->w + object->type.triangle.tb.x * info->u + object->type.triangle.tc.x * info->v;
    //     d_v = object->type.triangle.ta.y * info->w + object->type.triangle.tb.y * info->u + object->type.triangle.tc.y * info->v;
    // } else {
    //     d_u = NAN;
    //     d_v = NAN;
    // }
    
    // return (HitResult){.point = p, .ng = object->type.triangle.ng, .ns = ns, .t = t, .material = object->material, .d_u = d_u, .d_v = d_v};
}

PackedMaterial create_packed_material(Material *materials[8], DynArray *textures) {
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

PackedHitResult packed_triangle_result(float t_arr[8], __m256 u, __m256 v, uint32_t building_indices[8], PackedRay ray, RuntimeTriangle *runtime_triangles, BuildingTriangle *building_triangles, float **vertex_data, DynArray *materials, DynArray *textures) {
    __m256 t = _mm256_loadu_ps(t_arr);
    ps_Vec p = ps_v_add(ray.o, ps_scale_ps(ray.v, t));

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

    __m256 w = _mm256_sub_ps(_mm256_set1_ps(1.0f), _mm256_add_ps(u, v));

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

    ps_Vec ng = (ps_Vec){
        .x = _mm256_loadu_ps(ngx),
        .y = _mm256_loadu_ps(ngy),
        .z = _mm256_loadu_ps(ngz)
    };

    ps_Vec na = (ps_Vec){
        .x = _mm256_loadu_ps(nax),
        .y = _mm256_loadu_ps(nay),
        .z = _mm256_loadu_ps(naz)
    };

    ps_Vec nb = (ps_Vec){
        .x = _mm256_loadu_ps(nbx),
        .y = _mm256_loadu_ps(nby),
        .z = _mm256_loadu_ps(nbz)
    };

    ps_Vec nc = (ps_Vec){
        .x = _mm256_loadu_ps(ncx),
        .y = _mm256_loadu_ps(ncy),
        .z = _mm256_loadu_ps(ncz)
    };

    ps_Vec ns = ps_normalize(ps_v_add(ps_v_add(ps_scale_ps(na, w), ps_scale_ps(nb, u)), ps_scale_ps(nc, v)));

    ps_Vec a = (ps_Vec){
        .x = _mm256_loadu_ps(ax),
        .y = _mm256_loadu_ps(ay),
        .z = _mm256_loadu_ps(az)
    };

    ps_Vec b = (ps_Vec){
        .x = _mm256_loadu_ps(bx),
        .y = _mm256_loadu_ps(by),
        .z = _mm256_loadu_ps(bz)
    };

    ps_Vec c = (ps_Vec){
        .x = _mm256_loadu_ps(cx),
        .y = _mm256_loadu_ps(cy),
        .z = _mm256_loadu_ps(cz)
    };

    ps_Vec ab = ps_v_sub(b, a);
    ps_Vec ac = ps_v_sub(c, a);

    float uv_ab_u[8], uv_ab_v[8];
    float uv_ac_u[8], uv_ac_v[8];


    for (int i = 0; i < 8; i++) {
        uv_ab_u[i] = vertex_data[2][2 * building_triangles->bi[indices[i]] + 0] - vertex_data[2][2 * building_triangles->ai[indices[i]] + 0];
        uv_ab_v[i] = vertex_data[2][2 * building_triangles->bi[indices[i]] + 1] - vertex_data[2][2 * building_triangles->ai[indices[i]] + 1];

        uv_ac_u[i] = vertex_data[2][2 * building_triangles->ci[indices[i]] + 0] - vertex_data[2][2 * building_triangles->ai[indices[i]] + 0];
        uv_ac_v[i] = vertex_data[2][2 * building_triangles->ci[indices[i]] + 1] - vertex_data[2][2 * building_triangles->ai[indices[i]] + 1];
    }

    __m256 uv_ab[2] = {
        _mm256_loadu_ps(uv_ab_u),
        _mm256_loadu_ps(uv_ab_v)
    };

    __m256 uv_ac[2] = {
        _mm256_loadu_ps(uv_ac_u),
        _mm256_loadu_ps(uv_ac_v)
    };

    __m256 D = _mm256_sub_ps(_mm256_mul_ps(uv_ab[0], uv_ac[1]), _mm256_mul_ps(uv_ab[1], uv_ac[0]));
    __m256 rcp_D = _mm256_rcp_ps(D);

    __m256 valid_tangent = _mm256_cmp_ps(_mm256_andnot_ps(_mm256_set1_ps(-0.0f), D), _mm256_set1_ps(EPSILON), _CMP_GT_OQ);

    __m256 valid_hit = _mm256_cmp_ps(t, _mm256_set1_ps(-1.0f), _CMP_NEQ_OQ);

    ps_Vec tangent = ps_scale_ps(
        ps_v_sub(
            ps_scale_ps(ab, uv_ac[1]),
            ps_scale_ps(ac, uv_ab[1])
        ),
        // _mm256_div_ps(_mm256_set1_ps(1.0f), D)
        rcp_D
    );

    ps_Vec bitangent = ps_scale_ps(
        ps_v_sub(
            ps_scale_ps(ac, uv_ab[0]),
            ps_scale_ps(ab, uv_ac[0])
        ),
        // _mm256_div_ps(_mm256_set1_ps(1.0f), D)
        rcp_D
    );

    //maybe zero out tangent and bitangent if valid_tangent is false

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

HitResult triangle_result(float t, float u, float v, uint32_t idx, Ray *ray, RuntimeTriangle *runtime_triangles, BuildingTriangle *building_triangles, float **vertex_data, DynArray *materials) {
    Vec p = v_add(ray->o, scale(ray->v, t));

    uint32_t i = runtime_triangles->building_idx[idx];
    Vec ng = {
        .x = building_triangles->nx[i],
        .y = building_triangles->ny[i],
        .z = building_triangles->nz[i]
    };

    Material *m = ((Material *)materials->ptr) + building_triangles->material[i];

    float w = 1 - u - v;

    Vec ns;

    Vec na = vec(
        vertex_data[1][3 * building_triangles->ai[i] + 0],
        vertex_data[1][3 * building_triangles->ai[i] + 1],
        vertex_data[1][3 * building_triangles->ai[i] + 2]
    );

    Vec nb = vec(
        vertex_data[1][3 * building_triangles->bi[i] + 0],
        vertex_data[1][3 * building_triangles->bi[i] + 1],
        vertex_data[1][3 * building_triangles->bi[i] + 2]
    );

    Vec nc = vec(
        vertex_data[1][3 * building_triangles->ci[i] + 0],
        vertex_data[1][3 * building_triangles->ci[i] + 1],
        vertex_data[1][3 * building_triangles->ci[i] + 2]
    );

    ns = ns = normalize(v_add(v_add(scale(na, w), scale(nb, u)), scale(nc, v)));
    double d_u, d_v;

    d_u = vertex_data[2][2 * building_triangles->ai[i] + 0] * w + vertex_data[2][2 * building_triangles->bi[i] + 0] * u + vertex_data[2][2 * building_triangles->ci[i] + 0] * v;
    d_v = vertex_data[2][2 * building_triangles->ai[i] + 1] * w + vertex_data[2][2 * building_triangles->bi[i] + 1] * u + vertex_data[2][2 * building_triangles->ci[i] + 1] * v;

    Vec a = vec(
        vertex_data[0][3 * building_triangles->ai[i] + 0],
        vertex_data[0][3 * building_triangles->ai[i] + 1],
        vertex_data[0][3 * building_triangles->ai[i] + 2]
    );

    Vec b = vec(
        vertex_data[0][3 * building_triangles->bi[i] + 0],
        vertex_data[0][3 * building_triangles->bi[i] + 1],
        vertex_data[0][3 * building_triangles->bi[i] + 2]
    );

    Vec c = vec(
        vertex_data[0][3 * building_triangles->ci[i] + 0],
        vertex_data[0][3 * building_triangles->ci[i] + 1],
        vertex_data[0][3 * building_triangles->ci[i] + 2]
    );

    Vec ab = v_sub(b, a);
    Vec ac = v_sub(c, a);
    
    float uv_ab[2] = {
        vertex_data[2][2 * building_triangles->bi[i] + 0] - vertex_data[2][2 * building_triangles->ai[i] + 0],
        vertex_data[2][2 * building_triangles->bi[i] + 1] - vertex_data[2][2 * building_triangles->ai[i] + 1],
    };

    float uv_ac[2] = {
        vertex_data[2][2 * building_triangles->ci[i] + 0] - vertex_data[2][2 * building_triangles->ai[i] + 0],
        vertex_data[2][2 * building_triangles->ci[i] + 1] - vertex_data[2][2 * building_triangles->ai[i] + 1],
    };

    float D = uv_ab[0] * uv_ac[1] - uv_ab[1] * uv_ac[0];

    Vec tangent, bitangent;

    int valid_tangent = fabsf(D) > EPSILON;

    if (valid_tangent) {
        tangent = scale(
            v_sub(
                scale(ab, uv_ac[1]),
                scale(ac, uv_ab[1])
            ),
            1.0f / D
        );

        bitangent = scale(
            v_sub(
                scale(ac, uv_ab[0]),
                scale(ab, uv_ac[0])
            ),
            1.0f / D
        );
    } else {
        tangent = vec(0.0, 0.0, 0.0);
        bitangent = vec(0.0, 0.0, 0.0);
    }

    return (HitResult){.point = p, .ng = ng, .ns = ns, .tangent = tangent, .bitangent = bitangent, .valid_tangent = valid_tangent, .t = t, .material = m, .d_u = d_u, .d_v = d_v};
}