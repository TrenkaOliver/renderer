#include <math.h>
#include "geometry/aabb.h"

void vecf_min(float *src, float *dst) {
    dst[0] = fminf(dst[0], src[0]);
    dst[1] = fminf(dst[1], src[1]);
    dst[2] = fminf(dst[2], src[2]);
}

void vecf_max(float *src, float *dst) {
    dst[0] = fmaxf(dst[0], src[0]);
    dst[1] = fmaxf(dst[1], src[1]);
    dst[2] = fmaxf(dst[2], src[2]);
}

void vecf_min3(float *src1, float *src2, float *dst) {
    dst[0] = fminf(src1[0], src2[0]);
    dst[1] = fminf(src1[1], src2[1]);
    dst[2] = fminf(src1[2], src2[2]);
}

void vecf_max3(float *src1, float *src2, float *dst) {
    dst[0] = fmaxf(src1[0], src2[0]);
    dst[1] = fmaxf(src1[1], src2[1]);
    dst[2] = fmaxf(src1[2], src2[2]);
}

void vecf_add(float *src, float *dst) {
    dst[0] += src[0];
    dst[1] += src[1];
    dst[2] += src[2];
}

void vecf_sub(float *src, float *dst) {
    dst[0] -= src[0];
    dst[1] -= src[1];
    dst[2] -= src[2];
}

AABB aabb_merge(AABB a, AABB b) {
    return (AABB){
        .min = {fminf(a.min[0], b.min[0]), fminf(a.min[1], b.min[1]), fminf(a.min[2], b.min[2])},
        .max = {fmaxf(a.max[0], b.max[0]), fmaxf(a.max[1], b.max[1]), fmaxf(a.max[2], b.max[2])}
    };
}

Vec calc_centroid(AABB aabb) {
    return (Vec){
        .x = (aabb.min[0] + aabb.max[0]) / 2.0,
        .y = (aabb.min[1] + aabb.max[1]) / 2.0,
        .z = (aabb.min[2] + aabb.max[2]) / 2.0
    };
}