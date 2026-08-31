#include <math.h>
#include <float.h>
#include "geometry/object.h"

#define EPSILON 1e-8

double box_ray_intersection(Object *object, Ray *ray, Info *info) {
    int i;
    double t_min, t_max, t1, t2, e, f, h, temp;
    Vec p;

    p = v_sub(object->type.box.center, ray->o);

    t_min = -DBL_MAX;
    t_max = DBL_MAX;

    for (i = 0; i < 3; i++) {
        e = dot(object->type.box.axes[i], p);
        f = dot(object->type.box.axes[i], ray->v);
        h = i == 0 ? object->type.box.half_size.x : i == 1 ? object->type.box.half_size.y : object->type.box.half_size.z;

        if (fabs(f) < EPSILON) {
            if (fabs(e) > h)
                return -1.0;
            else
                continue;
        }

        t1 = (e - h) / f;
        t2 = (e + h) / f;

        if (t1 > t2) {
            temp = t1;
            t1 = t2;
            t2 = temp;
        }

        t_min = fmax(t1, t_min);
        t_max = fmin(t2, t_max);

        if (t_min > t_max) return -1.0;
    }

    if (t_max < 0.0) return -1.0;
    
    return t_min >= 0.0 ? t_min : t_max;
}

__m256 packed_box_ray_intersection(Object *object, PackedRay *ray, PackedInfo *info) {
    ps_Vec p = ps_v_sub(ps_from_vec(object->type.box.center), ray->o);

    __m256 neg_zero = _mm256_set1_ps(-0.0f);
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    
    __m256 t_min = _mm256_set1_ps(-FLT_MAX);
    __m256 t_max = _mm256_set1_ps(FLT_MAX);

    __m256 valid = _mm256_cmp_ps(zero, zero, _CMP_EQ_OQ);

    for (int i = 0; i < 3; i++) {
        ps_Vec axis = ps_from_vec(object->type.box.axes[i]);
        __m256 e = ps_dot(axis, p);
        __m256 f = ps_dot(axis, ray->v);
        __m256 inv_f = _mm256_div_ps(one, f);
        __m256 h = _mm256_set1_ps(i == 0 ? object->type.box.half_size.x : i == 1 ? object->type.box.half_size.y : object->type.box.half_size.z);

        __m256 f_good = _mm256_cmp_ps(_mm256_andnot_ps(neg_zero, f), _mm256_set1_ps(EPSILON), _CMP_GE_OQ);
        __m256 e_good = _mm256_cmp_ps(_mm256_andnot_ps(neg_zero, e), h, _CMP_LE_OQ);

        valid = _mm256_and_ps(valid, _mm256_or_ps(f_good, e_good));

        __m256 t1 = _mm256_mul_ps(_mm256_sub_ps(e, h), inv_f);
        __m256 t2 = _mm256_mul_ps(_mm256_add_ps(e, h), inv_f);

        t_min = _mm256_blendv_ps(t_min, _mm256_max_ps(_mm256_min_ps(t1, t2), t_min), f_good);
        t_max = _mm256_blendv_ps(t_max, _mm256_min_ps(_mm256_max_ps(t1, t2), t_max), f_good);

        valid = _mm256_and_ps(valid, _mm256_cmp_ps(t_min, t_max, _CMP_LE_OQ));
    }

    valid = _mm256_and_ps(valid, _mm256_cmp_ps(t_max, zero, _CMP_GE_OQ));

    __m256 change = _mm256_cmp_ps(t_min, zero, _CMP_LT_OQ);
    __m256 t = _mm256_blendv_ps(t_min, t_max, change);

    return _mm256_blendv_ps(_mm256_set1_ps(-1.0f), t, valid);
}

HitResult get_box_result(Ray *ray, Object *object, Info *info, double t) {
    Vec p, dist, n;
    double x, y, z, ax, ay, az;

    p = v_add(ray->o, scale(ray->v, t));
    dist = v_sub(p, object->type.box.center);

    x = dot(object->type.box.axes[0], dist);
    y = dot(object->type.box.axes[1], dist);
    z = dot(object->type.box.axes[2], dist);

    ax = fabs(x) / object->type.box.half_size.x;
    ay = fabs(y) / object->type.box.half_size.y;
    az = fabs(z) / object->type.box.half_size.z;

    if (ax >= ay && ax >= az)
        n = (x > 0.0) ? object->type.box.axes[0] : neg(object->type.box.axes[0]);
    else if (ay >= az)
        n = (y > 0.0) ? object->type.box.axes[1] : neg(object->type.box.axes[1]);
    else
        n = (z > 0.0) ? object->type.box.axes[2] : neg(object->type.box.axes[2]);

    return (HitResult){.point = p, .ng = n, .ns = n, .t = t, .material = object->material, .d_u = NAN, .d_v = NAN};
}