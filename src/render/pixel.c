#include <math.h>

#include "render/render.h"
#include "math/vec.h"

Pixel create_pixel(unsigned char r, unsigned char g, unsigned char b) {
    return (Pixel) {
        .r = r, 
        .g = g, 
        .b = b
    };
}

Pixel color_to_pixel(Vec c) {
    return (Pixel) {
        .r = fmin(c.x, 1.0) * 255,
        .g = fmin(c.y, 1.0) * 255,
        .b = fmin(c.z, 1.0) * 255,
    };
}

PackedPixel color_to_packed_pixel(ps_Vec c) {
    const __m256 one = _mm256_set1_ps(1.0f);
    const __m256 scale = _mm256_set1_ps(255.0f);

    __m256i r = _mm256_cvttps_epi32(_mm256_mul_ps(_mm256_min_ps(c.x, one), scale));
    __m256i g = _mm256_cvttps_epi32(_mm256_mul_ps(_mm256_min_ps(c.y, one), scale));
    __m256i b = _mm256_cvttps_epi32(_mm256_mul_ps(_mm256_min_ps(c.z, one), scale));

    uint32_t r_array[8], g_array[8], b_array[8];

    _mm256_storeu_si256((__m256i*)r_array, r);
    _mm256_storeu_si256((__m256i*)g_array, g);
    _mm256_storeu_si256((__m256i*)b_array, b);

    PackedPixel packed_pixel;

    for (int i = 0; i < 8; i++) {
        packed_pixel.r[i] = (uint8_t)r_array[i];
        packed_pixel.g[i] = (uint8_t)g_array[i];
        packed_pixel.b[i] = (uint8_t)b_array[i];
    }

    return packed_pixel;
}