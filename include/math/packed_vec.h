#ifndef PACKED_VECTOR_H
#define PACKED_VECTOR_H

#include <math.h>
#include <math/vec.h>
#include <immintrin.h>

typedef struct ps_Vec {
    __m256 x;
    __m256 y;
    __m256 z;
} ps_Vec;

static inline ps_Vec ps_from_vec(const Vec v) {
    return (ps_Vec) {
        .x = _mm256_set1_ps(v.x),
        .y = _mm256_set1_ps(v.y),
        .z = _mm256_set1_ps(v.z)
    };
}

static inline ps_Vec ps_zero_vec() {
    return (ps_Vec) {
        .x = _mm256_setzero_ps(),
        .y = _mm256_setzero_ps(),
        .z = _mm256_setzero_ps()
    };
}

static inline __m256 ps_len_sqr(const ps_Vec vec) {
    return _mm256_fmadd_ps(vec.x, vec.x, _mm256_fmadd_ps(vec.y, vec.y, _mm256_mul_ps(vec.z, vec.z)));
}

static inline ps_Vec ps_v_min(const ps_Vec a, const ps_Vec b) {
    return (ps_Vec) {
        .x = _mm256_min_ps(a.x, b.x),
        .y = _mm256_min_ps(a.y, b.y),
        .z = _mm256_min_ps(a.z, b.z),
    };
}

static inline ps_Vec ps_v_max(const ps_Vec a, const ps_Vec b) {
    return (ps_Vec) {
        .x = _mm256_max_ps(a.x, b.x),
        .y = _mm256_max_ps(a.y, b.y),
        .z = _mm256_max_ps(a.z, b.z),
    };
}

static inline ps_Vec ps_neg(const ps_Vec vec) {
    __m256 sign = _mm256_set1_ps(-0.0f);

    return (ps_Vec) {
        .x = _mm256_xor_ps(vec.x, sign),
        .y = _mm256_xor_ps(vec.y, sign),
        .z = _mm256_xor_ps(vec.z, sign),
    };
}

static inline ps_Vec ps_reciprocal(const ps_Vec vec) {
    // __m256 dividend = _mm256_set1_ps(1.0f);
    // return (ps_Vec) {
    //     .x = _mm256_div_ps(dividend, vec.x),
    //     .y = _mm256_div_ps(dividend, vec.y),
    //     .z = _mm256_div_ps(dividend, vec.z),
    // };
    return (ps_Vec) {
        .x = _mm256_rcp_ps(vec.x),
        .y = _mm256_rcp_ps(vec.y),
        .z = _mm256_rcp_ps(vec.z),
    };
}

static inline ps_Vec ps_v_add(const ps_Vec a, const ps_Vec b) {
    return (ps_Vec) {
        .x = _mm256_add_ps(a.x, b.x),
        .y = _mm256_add_ps(a.y, b.y),
        .z = _mm256_add_ps(a.z, b.z)
    };
}

static inline ps_Vec ps_v_sub(const ps_Vec a, const ps_Vec b) {
    return (ps_Vec) {
        .x = _mm256_sub_ps(a.x, b.x),
        .y = _mm256_sub_ps(a.y, b.y),
        .z = _mm256_sub_ps(a.z, b.z)
    };
}

static inline ps_Vec ps_scale_ss(const ps_Vec vec, const float value) {
    __m256 scale_vec = _mm256_set1_ps(value);
    return (ps_Vec) {
        .x = _mm256_mul_ps(vec.x, scale_vec),
        .y = _mm256_mul_ps(vec.y, scale_vec),
        .z = _mm256_mul_ps(vec.z, scale_vec)
    };
}

static inline ps_Vec ps_scale_ps(const ps_Vec vec, const __m256 value) {
    return (ps_Vec) {
        .x = _mm256_mul_ps(vec.x, value),
        .y = _mm256_mul_ps(vec.y, value),
        .z = _mm256_mul_ps(vec.z, value)
    };
}

static inline __m256 ps_dot(const ps_Vec a, const ps_Vec b) {
    const __m256 x = _mm256_mul_ps(a.x, b.x);
    const __m256 y = _mm256_mul_ps(a.y, b.y);
    const __m256 z = _mm256_mul_ps(a.z, b.z);

    return _mm256_add_ps(_mm256_add_ps(x, y), z);
}

static inline ps_Vec ps_hadamard(const ps_Vec a, const ps_Vec b) {
    return (ps_Vec){.x = _mm256_mul_ps(a.x, b.x), .y = _mm256_mul_ps(a.y, b.y), .z = _mm256_mul_ps(a.z, b.z)};
}

static inline ps_Vec ps_cross(const ps_Vec a, const ps_Vec b) {
    return (ps_Vec){
        .x = _mm256_sub_ps(_mm256_mul_ps(a.y, b.z),
                           _mm256_mul_ps(a.z, b.y)),
        .y = _mm256_sub_ps(_mm256_mul_ps(a.z, b.x),
                           _mm256_mul_ps(a.x, b.z)),
        .z = _mm256_sub_ps(_mm256_mul_ps(a.x, b.y),
                           _mm256_mul_ps(a.y, b.x))
    };
}

static inline __m256 ps_len(const ps_Vec vec) {
    const __m256 len_sqr_vec = ps_len_sqr(vec);
    return _mm256_sqrt_ps(len_sqr_vec);
}

static inline ps_Vec ps_normalize(const ps_Vec vec) {
    const __m256 inv_len = _mm256_rcp_ps(ps_len(vec));
    return ps_scale_ps(vec, inv_len);
}

static inline ps_Vec ps_lerp_fma(const ps_Vec a, const ps_Vec b, const __m256 t) {
    return (ps_Vec) {
        .x = _mm256_fmadd_ps(
            t,
            _mm256_sub_ps(b.x, a.x),
            a.x
        ),
        .y = _mm256_fmadd_ps(
            t,
            _mm256_sub_ps(b.y, a.y),
            a.y
        ),
        .z = _mm256_fmadd_ps(
            t,
            _mm256_sub_ps(b.z, a.z),
            a.z
        )
    };
}


#endif