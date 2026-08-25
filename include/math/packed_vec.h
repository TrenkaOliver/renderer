#ifndef PACKED_VECTOR_H
#define PACKED_VECTOR_H

#include <math.h>
#include <immintrin.h>

typedef struct ps_Vec {
    __m256 x;
    __m256 y;
    __m256 z;
} ps_Vec;

static inline __m256 ps_len_sqr(ps_Vec vec) {
    return _mm256_fmadd_ps(vec.x, vec.x, _mm256_fmadd_ps(vec.y, vec.y, _mm256_mul_ps(vec.z, vec.z)));
}

static inline ps_Vec ps_v_min(ps_Vec a, ps_Vec b) {
    return (ps_Vec) {
        .x = _mm256_min_ps(a.x, b.x),
        .y = _mm256_min_ps(a.y, b.y),
        .z = _mm256_min_ps(a.z, b.z),
    };
}

static inline ps_Vec ps_v_max(ps_Vec a, ps_Vec b) {
    return (ps_Vec) {
        .x = _mm256_max_ps(a.x, b.x),
        .y = _mm256_max_ps(a.y, b.y),
        .z = _mm256_max_ps(a.z, b.z),
    };
}

static inline ps_Vec ps_neg(ps_Vec vec) {
    __m256 sign = _mm256_set1_ps(-0.0f);

    return (ps_Vec) {
        .x = _mm256_xor_ps(vec.x, sign),
        .y = _mm256_xor_ps(vec.y, sign),
        .z = _mm256_xor_ps(vec.z, sign),
    };
}

static inline ps_Vec ps_reciprocal(ps_Vec vec) {
    __m256 dividend = _mm256_set1_ps(1.0f);
    return (ps_Vec) {
        .x = _mm256_div_ps(dividend, vec.x),
        .y = _mm256_div_ps(dividend, vec.y),
        .z = _mm256_div_ps(dividend, vec.z),
    };
}

static inline ps_Vec ps_v_add(ps_Vec a, ps_Vec b) {
    return (ps_Vec) {
        .x = _mm256_add_ps(a.x, b.x),
        .y = _mm256_add_ps(a.y, b.y),
        .z = _mm256_add_ps(a.z, b.z)
    };
}

static inline ps_Vec ps_v_sub(ps_Vec a, ps_Vec b) {
    return (ps_Vec) {
        .x = _mm256_sub_ps(a.x, b.x),
        .y = _mm256_sub_ps(a.y, b.y),
        .z = _mm256_sub_ps(a.z, b.z)
    };
}

static inline ps_Vec ps_scale_ss(ps_Vec vec, float value) {
    __m256 scale_vec = _mm256_set1_ps(value);
    return (ps_Vec) {
        .x = _mm256_mul_ps(vec.x, scale_vec),
        .y = _mm256_mul_ps(vec.y, scale_vec),
        .z = _mm256_mul_ps(vec.z, scale_vec)
    };
}

static inline ps_Vec ps_scale_ps(ps_Vec vec, __m256 value) {
    return (ps_Vec) {
        .x = _mm256_mul_ps(vec.x, value),
        .y = _mm256_mul_ps(vec.y, value),
        .z = _mm256_mul_ps(vec.z, value)
    };
}

static inline __m256 ps_dot(ps_Vec a, ps_Vec b) {
    return _mm256_fmadd_ps(a.x, b.x, _mm256_fmadd_ps(a.y, b.y, _mm256_mul_ps(a.z, b.z)));
}

static inline ps_Vec ps_hadamard(ps_Vec a, ps_Vec b) {
    return (ps_Vec){.x = _mm256_mul_ps(a.x, b.x), .y = _mm256_mul_ps(a.y, b.y), .z = _mm256_mul_ps(a.z, b.z)};
}

static inline ps_Vec ps_cross(ps_Vec a, ps_Vec b) {
    return (ps_Vec){
        .x = _mm256_fmsub_ps(a.y, b.z, _mm256_mul_ps(a.z, b.y)),
        .y = _mm256_fmsub_ps(a.z, b.x, _mm256_mul_ps(a.x, b.z)),
        .z = _mm256_fmsub_ps(a.x, b.y, _mm256_mul_ps(a.y, b.x))
    };
}

static inline __m256 ps_len(ps_Vec vec) {
    __m256 len_sqr_vec = ps_len_sqr(vec);
    return _mm256_sqrt_ps(len_sqr_vec);
}

static inline ps_Vec ps_normalize(ps_Vec vec) {
    __m256 inv_len = _mm256_div_ps(_mm256_set1_ps(1.0f), ps_len(vec));
    return ps_scale_ps(vec, inv_len);
}


#endif