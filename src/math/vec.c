#include "math.h"
#include "math/vec.h"

Vec vec(double x, double y, double z) {
    return (Vec) {
        .x = x, 
        .y = y, 
        .z = z
    };
}

double len_sqr(Vec vec) {
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

double len(Vec vec) {
    return sqrt(len_sqr(vec));
}

Vec v_min(Vec a, Vec b) {
    return (Vec){
        .x = fmin(a.x, b.x),
        .y = fmin(a.y, b.y),
        .z = fmin(a.z, b.z),
    };
}

Vec v_max(Vec a, Vec b) {
    return (Vec){
        .x = fmax(a.x, b.x),
        .y = fmax(a.y, b.y),
        .z = fmax(a.z, b.z),
    };
}

Vec neg(Vec vec) {
    return scale(vec, -1);
}

Vec normalize(Vec vec) {
    double l;

    if ((l = len(vec)) == 0) 
        return vec;
    else 
        return v_div(vec, l);
}

Vec v_add(Vec a, Vec b) {
    return (Vec) {
        .x = a.x + b.x, 
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

Vec v_sub(Vec a, Vec b) {
    return (Vec) {
        .x = a.x - b.x, 
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

Vec scale(Vec vec, double value) {
    return (Vec) {
        .x = vec.x * value,
        .y = vec.y * value,
        .z = vec.z * value,
    };
}

Vec v_div(Vec vec, double value) {
    return (Vec) {
        .x = vec.x / value,
        .y = vec.y / value,
        .z = vec.z / value,
    };
}

double dot(Vec a, Vec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec cross(Vec a, Vec b) {
    return (Vec) {
        .x = a.y * b.z - a.z * b.y,
        .y = a.z * b.x - a.x * b.z,
        .z = a.x * b.y - a.y * b.x
    };
}

Vec hadamard(Vec a, Vec b) {
    return (Vec) {
        .x = a.x * b.x,
        .y = a.y * b.y,
        .z = a.z * b.z,
    };
}

Vec rotate(Vec v, Vec r) {
    Vec out;
    double sinx, siny, sinz, cosx, cosy, cosz;

    sinx = sin(r.x);
    cosx = cos(r.x);
    siny = sin(r.y);
    cosy = cos(r.y);
    sinz = sin(r.z);
    cosz = cos(r.z);

    out.x = (v.x * cosy + (v.y * sinx + v.z * cosx) * siny) * cosz - (v.y * cosx - v.z * sinx) * sinz;
    out.y = (v.x * cosy + (v.y * sinx + v.z * cosx) * siny) * sinz + (v.y * cosx - v.z * sinx) * cosz;
    out.z = -v.x * siny + (v.y * sinx + v.z * cosx) * cosy;

    return out;
}