#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

typedef struct Vec {
    double x;
    double y;
    double z;
} Vec;

static inline Vec vec(double x, double y, double z) {
    return (Vec) {.x = x, .y = y, .z = z};
}

static inline double len_sqr(Vec vec) {
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

static inline Vec v_min(Vec a, Vec b) {
    return (Vec){.x = fmin(a.x, b.x), .y = fmin(a.y, b.y), .z = fmin(a.z, b.z)};
}

static inline Vec v_max(Vec a, Vec b) {
    return (Vec){.x = fmax(a.x, b.x), .y = fmax(a.y, b.y), .z = fmax(a.z, b.z)};
}

static inline Vec neg(Vec vec) {
    return (Vec){.x = -vec.x, .y = -vec.y, .z = -vec.z};
}

static inline Vec reciproc(Vec vec) {
    return (Vec){.x = 1.0 / vec.x, .y = 1.0 / vec.y, .z = 1.0 / vec.z};
}

static inline Vec v_add(Vec a, Vec b) {
    return (Vec){.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}

static inline Vec v_sub(Vec a, Vec b) {
    return (Vec){.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z,};
}

static inline Vec scale(Vec vec, double value) {
    return (Vec){.x = vec.x * value, .y = vec.y * value, .z = vec.z * value,};
}

static inline double dot(Vec a, Vec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec hadamard(Vec a, Vec b) {
    return (Vec){.x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z};
}

static inline Vec cross(Vec a, Vec b) {
    return (Vec){
        .x = a.y * b.z - a.z * b.y,
        .y = a.z * b.x - a.x * b.z,
        .z = a.x * b.y - a.y * b.x
    };
}

double len(Vec vec);
Vec normalize(Vec vec);
Vec rotate(Vec v, Vec r);

#endif