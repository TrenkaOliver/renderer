#include <math.h>
#include <float.h>
#include "geometry/object.h"

double sphere_ray_intersection(Object *object, Ray *ray, Info *info) {
    double a, b, c, d, t1, t2, t;
    Vec m;

    m = v_sub(ray->o, object->type.sphere.o);

    a = 1.0;
    b = 2.0 * dot(m, ray->v);
    c = dot(m, m) - object->type.sphere.r * object->type.sphere.r;

    d = b * b - 4 * a * c;

    if (d < 0.0) return -1.0;

    t1 = (-b + sqrt(d)) / (2.0 * a);
    t2 = (-b - sqrt(d)) / (2.0 * a);

    t = DBL_MAX;

    if (t1 > 0.0 && t1 < t) t = t1;
    if (t2 > 0.0 && t2 < t) t = t2;

    if (t == DBL_MAX) return -1.0;

    return t;
}

__m256 packed_sphere_ray_intersection(Object *object, PackedRay *ray, Info *info) {
    ps_Vec m = ps_v_sub(ray->o, ps_from_vec(object->type.sphere.o));

    __m256 zero = _mm256_setzero_ps();
    __m256 flt_max = _mm256_set1_ps(FLT_MAX);

    __m256 b = ps_dot(m, ray->v);
    __m256 neg_b = _mm256_sub_ps(zero, b);

    __m256 r = _mm256_set1_ps(object->type.sphere.r);
    __m256 c = _mm256_sub_ps(ps_dot(m, m), _mm256_mul_ps(r, r));

    __m256 d = _mm256_sub_ps(_mm256_mul_ps(b, b), c);
    __m256 valid = _mm256_cmp_ps(d, zero, _CMP_GE_OQ);

    __m256 sqrt_d = _mm256_sqrt_ps(_mm256_max_ps(d, zero));
    
    __m256 t1 = _mm256_sub_ps(neg_b, sqrt_d);
    __m256 t2 = _mm256_add_ps(neg_b, sqrt_d);

    __m256 t = flt_max;

    __m256 t1_pos = _mm256_and_ps(valid, _mm256_cmp_ps(t1, zero, _CMP_GT_OQ));
    t = _mm256_blendv_ps(t, t1, t1_pos);

    __m256 t2_better = _mm256_and_ps(
        valid,
        _mm256_and_ps(
            _mm256_cmp_ps(t2, zero, _CMP_GT_OQ),
            _mm256_cmp_ps(t2, t, _CMP_LT_OQ)
        )
    );

    t = _mm256_blendv_ps(t, t2, t2_better);

    valid = _mm256_cmp_ps(t, flt_max, _CMP_NEQ_OQ);

    return _mm256_blendv_ps(_mm256_set1_ps(-1.0f), t, valid);    
}


HitResult get_sphere_result(Ray *ray, Object *object, Info *info, double t) {
    Vec p, n;

    p = v_add(ray->o, scale(ray->v, t));
    n = normalize(v_sub(p, object->type.sphere.o));

    return (HitResult){.point = p, .ng = n, .ns = n, .t = t, .material = object->material, .d_u = NAN, .d_v = NAN};
}