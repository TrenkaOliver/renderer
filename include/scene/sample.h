#ifndef SAMPLE_H
#define SAMPLE_H

#include <stddef.h>
#include "light/material.h"

#define ENV_MIP_COUNT       6

#define IRRADIANCE_SAMPLES  256
#define PREFILTER_SAMPLES   256
#define BRDF_SAMPLES        256

double radical_inverse(unsigned k);
void hammersley(uint32_t i, uint32_t n, double * restrict u, double * restrict v);
void make_coord_space(const Vec N, Vec * restrict T, Vec * restrict B);
Vec importance_sample_GGX(double u1, double u2, double roughness, Vec N);

Vec sample(Vec N, const Texture *map);
Vec sample_prefiltered(Vec R, double roughness, Texture maps[]);
Vec sample_brdf_lut(double NdotV, double roughness, Texture *map);

void generate_irradiance_map(const Texture * restrict environment, Texture * restrict irradiance);
void generate_prefiltered_layer(const Texture * restrict environment, Texture * restrict prefiltered, const double roughness);
void generate_brdf_lut(Texture *brdf_lut);

static inline double clampd(const double x, const double min, const double max) {
    return fmax(min, fmin(x, max));
}

static inline void packed_environment_uv(const ps_Vec dir, __m256 * restrict u, __m256 * restrict v) {    
    float x[8], y[8], z[8];
    float theta_arr[8], phi_arr[8];

    _mm256_storeu_ps(x, dir.x);
    _mm256_storeu_ps(y, dir.y);
    _mm256_storeu_ps(z, dir.z);

    for (int i = 0; i < 8; i++) {
        theta_arr[i] = atan2(y[i], x[i]);
        phi_arr[i] = asin(clampd(z[i], -1.0, 1.0));
    }

    const __m256 theta = _mm256_loadu_ps(theta_arr);
    const __m256 phi = _mm256_loadu_ps(phi_arr);

    const __m256 _f05 = _mm256_set1_ps(0.5f);
    *u = _mm256_add_ps(_mm256_mul_ps(theta, _mm256_rcp_ps(_mm256_set1_ps(2.0f * M_PI))), _f05);
    *v = _mm256_sub_ps(_f05, _mm256_mul_ps(phi, _mm256_rcp_ps(_mm256_set1_ps(M_PI))));
}

static inline ps_Vec packed_sample(const ps_Vec N, const Texture *map) {
    __m256 u, v;
    packed_environment_uv(N, &u, &v);

    const __m256i _i0 = _mm256_setzero_si256();
    const __m256i _i1 = _mm256_set1_epi32(1);
    const __m256i _i3 = _mm256_set1_epi32(3);

    const __m256 w = _mm256_set1_ps((float)map->w);
    const __m256 h = _mm256_set1_ps((float)map->h - 1.0f);

    const __m256 x = _mm256_mul_ps(u, w);
    const __m256 y = _mm256_mul_ps(v, h);

    const __m256 xf = _mm256_floor_ps(x);
    const __m256 yf = _mm256_floor_ps(y);

    const __m256 fx = _mm256_sub_ps(x, xf);
    const __m256 fy = _mm256_sub_ps(y, yf);

    const __m256i x0 = _mm256_max_epi32(_mm256_min_epi32(_mm256_cvttps_epi32(xf), _mm256_set1_epi32(map->w - 1)), _i0);
    const __m256i y0 = _mm256_max_epi32(_mm256_min_epi32(_mm256_cvttps_epi32(yf), _mm256_set1_epi32(map->h - 1)), _i0);
    const __m256i y1 = _mm256_min_epi32(_mm256_add_epi32(y0, _i1), _mm256_set1_epi32(map->h - 1));
    
    __m256i x1 = _mm256_add_epi32(x0, _i1);

    const __m256i wrap = _mm256_cmpeq_epi32(x1, _mm256_set1_epi32(map->w));

    x1 = _mm256_blendv_epi8(x1, _mm256_setzero_si256(), wrap);

    const __m256i row0 = _mm256_mullo_epi32(y0, _mm256_set1_epi32(3 * map->w));
    const __m256i row1 = _mm256_mullo_epi32(y1, _mm256_set1_epi32(3 * map->w));

    const __m256i idx[4] = {
        _mm256_add_epi32(row0, _mm256_mullo_epi32(x0, _i3)),
        _mm256_add_epi32(row0, _mm256_mullo_epi32(x1, _i3)),
        _mm256_add_epi32(row1, _mm256_mullo_epi32(x0, _i3)),
        _mm256_add_epi32(row1, _mm256_mullo_epi32(x1, _i3)),
    };

    __m256 channels[4][3];

    float *data = (float *)map->ptr;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            __m256i index = _mm256_add_epi32(idx[i], _mm256_set1_epi32(j));
            channels[i][j] = _mm256_i32gather_ps(data, index, 4);
        }
    }

    ps_Vec c[4];

    for (int i = 0; i < 4; i++) {
        c[i] = (ps_Vec) {
            .x = channels[i][0],
            .y = channels[i][1],
            .z = channels[i][2],
        };
    }

    const ps_Vec c0 = ps_lerp_fma(c[0], c[1], fx);
    const ps_Vec c1 = ps_lerp_fma(c[2], c[3], fx);

    return ps_lerp_fma(c0, c1, fy);
}

static inline ps_Vec packed_sample_prefiltered(const ps_Vec R, __m256 roughness, const Texture maps[]) {
    const __m256 _f0 = _mm256_setzero_ps();
    const __m256i _0 = _mm256_setzero_si256();
    const __m256 _f1 = _mm256_set1_ps(1.0f);
    const __m256i _1 = _mm256_set1_epi32(1);
    const __m256i _emc = _mm256_set1_epi32(ENV_MIP_COUNT - 1);
    const __m256 _femc = _mm256_set1_ps((float)(ENV_MIP_COUNT - 1));

    roughness = _mm256_max_ps(_f0, _mm256_min_ps(roughness, _f1));
    
    const __m256 level = _mm256_mul_ps(roughness, _femc);
    const __m256 f0 = _mm256_floor_ps(level);
    const __m256 f = _mm256_sub_ps(level, f0);

    const __m256i i0 = _mm256_cvttps_epi32(f0);
    const __m256i i1 = _mm256_max_epi32(_0, _mm256_min_epi32(_mm256_add_epi32(i0, _1), _emc));

    uint32_t i0_arr[8], i1_arr[8];

    _mm256_storeu_si256((__m256i_u *)i0_arr, i0);
    _mm256_storeu_si256((__m256i_u *)i1_arr, i1);

    float c0x[8], c0y[8], c0z[8], c1x[8], c1y[8], c1z[8], x[8], y[8], z[8];

    _mm256_storeu_ps(x, R.x);
    _mm256_storeu_ps(y, R.y);
    _mm256_storeu_ps(z, R.z);

    for (int i = 0; i < 8; i++) {
        Vec r = vec(x[i], y[i], z[i]);
        Vec c0 = sample(r, maps + i0_arr[i]);
        Vec c1 = sample(r, maps + i1_arr[i]);

        c0x[i] = c0.x;
        c0y[i] = c0.y;
        c0z[i] = c0.z;
        c1x[i] = c1.x;
        c1y[i] = c1.y;
        c1z[i] = c1.z;
    }

    return ps_lerp_fma(
        (ps_Vec){.x = _mm256_loadu_ps(c0x), .y = _mm256_loadu_ps(c0y), .z = _mm256_loadu_ps(c0z)},
        (ps_Vec){.x = _mm256_loadu_ps(c1x), .y = _mm256_loadu_ps(c1y), .z = _mm256_loadu_ps(c1z)},
        f
    );
}

static inline ps_Vec packed_sample_brdf_lut(__m256 NdotV, __m256 roughness, const Texture *map) {
    const __m256 _f0 = _mm256_setzero_ps();
    const __m256 _f1 = _mm256_set1_ps(1.0f);

    const __m256i _i0 = _mm256_setzero_si256();
    const __m256i _i1 = _mm256_set1_epi32(1);
    const __m256i _i2 = _mm256_set1_epi32(2);

    NdotV = _mm256_max_ps(_f0, _mm256_min_ps(NdotV, _f1));
    roughness = _mm256_max_ps(_f0, _mm256_min_ps(roughness, _f1));

    const __m256 w = _mm256_set1_ps((float)(map->w - 1));   
    const __m256 h = _mm256_set1_ps((float)(map->h - 1));

    const __m256 x = _mm256_mul_ps(NdotV, w);
    const __m256 y = _mm256_mul_ps(roughness, h);

    const __m256 xf = _mm256_floor_ps(x);
    const __m256 yf = _mm256_floor_ps(y);

    const __m256 fx = _mm256_sub_ps(x, xf);
    const __m256 fy = _mm256_sub_ps(y, yf);

    const __m256i x0 = _mm256_max_epi32(_mm256_min_epi32(_mm256_cvttps_epi32(xf), _mm256_set1_epi32(map->w - 1)), _i0);
    const __m256i y0 = _mm256_max_epi32(_mm256_min_epi32(_mm256_cvttps_epi32(yf), _mm256_set1_epi32(map->h - 1)), _i0);
    const __m256i y1 = _mm256_min_epi32(_mm256_add_epi32(y0, _i1), _mm256_set1_epi32(map->h - 1));
    
    const __m256i x1 = _mm256_min_epi32(
        _mm256_add_epi32(x0, _i1),
        _mm256_set1_epi32(map->w - 1)
    );

    const __m256i row0 = _mm256_mullo_epi32(y0, _mm256_set1_epi32(2 * map->w));
    const __m256i row1 = _mm256_mullo_epi32(y1, _mm256_set1_epi32(2 * map->w));

    __m256i idx[4] = {
        _mm256_add_epi32(row0, _mm256_mullo_epi32(x0, _i2)),
        _mm256_add_epi32(row0, _mm256_mullo_epi32(x1, _i2)),
        _mm256_add_epi32(row1, _mm256_mullo_epi32(x0, _i2)),
        _mm256_add_epi32(row1, _mm256_mullo_epi32(x1, _i2)),
    };

    __m256 channels[4][2];

    float *data = (float *)map->ptr;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            __m256i index = _mm256_add_epi32(idx[i], _mm256_set1_epi32(j));
            channels[i][j] = _mm256_i32gather_ps(data, index, 4);
        }
    }

    ps_Vec c[4];

    for (int i = 0; i < 4; i++) {
        c[i] = (ps_Vec) {
            .x = channels[i][0],
            .y = channels[i][1],
            .z = _f0
        };
    }

    const ps_Vec c0 = ps_lerp_fma(c[0], c[1], fx);
    const ps_Vec c1 = ps_lerp_fma(c[2], c[3], fx);

    return ps_lerp_fma(c0, c1, fy);
}

static inline void packed_make_coord_space(const ps_Vec N, ps_Vec * restrict T, ps_Vec * restrict B) {
    const ps_Vec up0 = ps_from_vec(vec(0.0, 0.0, 1.0));
    const ps_Vec up1 = ps_from_vec(vec(0.0, 1.0, 0.0));

    const __m256 mask = _mm256_cmp_ps(_mm256_andnot_ps(_mm256_set1_ps(-0.0f), N.z), _mm256_set1_ps(0.999f), _CMP_GE_OQ);
    const ps_Vec up = {
        .x = _mm256_blendv_ps(up0.x, up1.x, mask),
        .y = _mm256_blendv_ps(up0.y, up1.y, mask),
        .z = _mm256_blendv_ps(up0.z, up1.z, mask),
    };

    *T = ps_cross(up, N);
    *B = ps_cross(N, *T);
}

static inline ps_Vec packed_importance_sample_GGX(const __m256 u1, const __m256 u2, const __m256 alpha2, const ps_Vec N) {
    const __m256 _f0 = _mm256_setzero_ps();
    const __m256 _f1 = _mm256_set1_ps(1.0f);
    const __m256 _f2PI = _mm256_set1_ps(2.0f * M_PI);

    const __m256 phi = _mm256_mul_ps(_f2PI, u1);

    const __m256 cos_theta2 = _mm256_mul_ps(_mm256_sub_ps(_f1, u2), _mm256_rcp_ps(_mm256_fmadd_ps(u2, _mm256_sub_ps(alpha2, _f1), _f1)));
    const __m256 cos_theta = _mm256_sqrt_ps(cos_theta2);
    const __m256 sin_theta = _mm256_sqrt_ps(_mm256_max_ps(_f0, _mm256_sub_ps(_f1, cos_theta2)));

    float phi_arr[8], sin_phi_arr[8], cos_phi_arr[8];
    _mm256_storeu_ps(phi_arr, phi);

    for (int i = 0; i < 8; i++) {
        sin_phi_arr[i] = sin(phi_arr[i]);
        cos_phi_arr[i] = cos(phi_arr[i]);
    }

    const __m256 sin_phi = _mm256_loadu_ps(sin_phi_arr);
    const __m256 cos_phi = _mm256_loadu_ps(cos_phi_arr);

    const ps_Vec H_local = {
        .x = _mm256_mul_ps(sin_theta, cos_phi),
        .y = _mm256_mul_ps(sin_theta, sin_phi),
        .z = cos_theta
    };

    ps_Vec T, B;
    packed_make_coord_space(N, &T, &B);

    const ps_Vec H = {
        .x = _mm256_fmadd_ps(T.x, H_local.x, _mm256_fmadd_ps(B.x, H_local.y, _mm256_mul_ps(N.x, H_local.z))),
        .y = _mm256_fmadd_ps(T.y, H_local.x, _mm256_fmadd_ps(B.y, H_local.y, _mm256_mul_ps(N.y, H_local.z))),
        .z = _mm256_fmadd_ps(T.z, H_local.x, _mm256_fmadd_ps(B.z, H_local.y, _mm256_mul_ps(N.z, H_local.z)))
    };

    return ps_normalize(H);
}

#endif