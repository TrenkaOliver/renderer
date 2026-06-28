#include "math.h"
#include "math/vec.h"

double len(Vec vec) {
    return sqrt(len_sqr(vec));
}

Vec normalize(Vec vec) {
    double l;

    if ((l = len(vec)) == 0) 
        return vec;
    else 
        return scale(vec, 1.0 / l);
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