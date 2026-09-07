#include <math.h>
#include <stdio.h>
#include <immintrin.h>

#include "geometry/object.h"

#define EPSILON 1e-8

__m256 packed_triangle_ray_intersection(uint32_t idx, uint8_t count, PackedRay *ray, SoATriangle *array) {
    ps_Vec a = (ps_Vec){
        .x = _mm256_loadu_ps(array->arr + idx),
        .y = _mm256_loadu_ps(array->arr + array->offset + idx),
        .z = _mm256_loadu_ps(array->arr + array->offset * 2 + idx)
    };
    ps_Vec ao = ps_v_sub(ray->o, a);

    ps_Vec b = (ps_Vec){
        .x = _mm256_loadu_ps(array->arr + array->offset * 3 + idx),
        .y = _mm256_loadu_ps(array->arr + array->offset * 4 + idx),
        .z = _mm256_loadu_ps(array->arr + array->offset * 5 + idx)
    };    
    ps_Vec ab = ps_v_sub(b, a);

    ps_Vec c = (ps_Vec){
        .x = _mm256_loadu_ps(array->arr + array->offset * 6 + idx),
        .y = _mm256_loadu_ps(array->arr + array->offset * 7 + idx),
        .z = _mm256_loadu_ps(array->arr + array->offset * 8 + idx)
    };
    ps_Vec ac = ps_v_sub(c, a);

    ps_Vec cross_ao_ab = ps_cross(ao, ab);
    ps_Vec cross_ray_v_ac = ps_cross(ray->v, ac);
    
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);

    __m256 denom = ps_dot(ab, cross_ray_v_ac);
    __m256 inv_denom = _mm256_div_ps(one, denom);
    __m256 valid = _mm256_cmp_ps(_mm256_andnot_ps(_mm256_set1_ps(-0.0f), denom), _mm256_set1_ps(EPSILON), _CMP_GE_OQ);

    __m256 u = _mm256_mul_ps(ps_dot(ao, cross_ray_v_ac), inv_denom);
    valid = _mm256_and_ps(valid, _mm256_cmp_ps(u, zero, _CMP_GE_OQ));

    __m256 v = _mm256_mul_ps(ps_dot(ray->v, cross_ao_ab), inv_denom);
    valid = _mm256_and_ps(valid, _mm256_cmp_ps(v, zero, _CMP_GE_OQ));

    valid = _mm256_and_ps(valid, _mm256_cmp_ps(_mm256_add_ps(u, v), one, _CMP_LE_OQ));
    valid = _mm256_and_ps(valid, 
        _mm256_castsi256_ps(
            _mm256_set_epi32(
                count > 7 ? -1 : 0,
                count > 6 ? -1 : 0,
                count > 5 ? -1 : 0,
                count > 4 ? -1 : 0,
                count > 3 ? -1 : 0,
                count > 2 ? -1 : 0,
                count > 1 ? -1 : 0,
                count > 0 ? -1 : 0
            )
        )
    );

    __m256 t = _mm256_mul_ps(ps_dot(ac, cross_ao_ab), inv_denom);

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
    Vec p, ns;
    double d_u, d_v;

    p = v_add(ray->o, scale(ray->v, t));
    
    ns = object->type.triangle.const_normal
    ? object->type.triangle.ng 
    : normalize(v_add(v_add(scale(object->type.triangle.na, info->w), scale(object->type.triangle.nb, info->u)), scale(object->type.triangle.nc, info->v)));

    if (object->material->diffuse_map != (size_t)-1) {
        d_u = object->type.triangle.ta.x * info->w + object->type.triangle.tb.x * info->u + object->type.triangle.tc.x * info->v;
        d_v = object->type.triangle.ta.y * info->w + object->type.triangle.tb.y * info->u + object->type.triangle.tc.y * info->v;
    } else {
        d_u = NAN;
        d_v = NAN;
    }
    
    return (HitResult){.point = p, .ng = object->type.triangle.ng, .ns = ns, .t = t, .material = object->material, .d_u = d_u, .d_v = d_v};
}

HitResult triangle_result(float t, uint32_t idx, Ray *ray, SoATriangle *triangles) {
    Vec p = v_add(ray->o, scale(ray->v, t));

    Vec ng = {
        .x = triangles->arr[triangles->offset * 18 + idx],
        .y = triangles->arr[triangles->offset * 19 + idx],
        .z = triangles->arr[triangles->offset * 20 + idx]
    };

    Material *m = triangles->mat_ptr_arr[idx];

    return (HitResult){.point = p, .ng = ng, .ns = ng, .t = t, .material = m, .d_u = NAN, .d_v = NAN};
}