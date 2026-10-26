#ifndef RAY_H
#define RAY_H

#include "math/vec.h"

typedef struct Ray {
    Vec o;
    Vec v;
    Vec inv_v;
} Ray;

typedef struct SoARay {
    _Alignas(32) float ox[8];
    _Alignas(32) float oy[8];
    _Alignas(32) float oz[8];
    _Alignas(32) float vx[8];
    _Alignas(32) float vy[8];
    _Alignas(32) float vz[8];
} SoARay;

Ray create_ray(Vec o, Vec v);

#endif