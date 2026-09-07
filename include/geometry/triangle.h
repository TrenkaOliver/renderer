#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "math/vec.h"
#include "stdint.h"

typedef struct Triangle {
    int const_normal;
    int has_texture;
    Vec a;
    Vec b;
    Vec c;
    Vec na;
    Vec nb;
    Vec nc;
    Vec ng;
    Vec ta;
    Vec tb;
    Vec tc;
} Triangle;

typedef struct SoATriangle {
    // float *ax;
    // float *ay;
    // float *az;

    // float *bx;
    // float *by;
    // float *bz;

    // float *cx;
    // float *cy;
    // float *cz;

    // float *nax;
    // float *nay;
    // float *naz;

    // float *nbx;
    // float *nby;
    // float *nbz;

    // float *ncx;
    // float *ncy;
    // float *ncz;

    // float *ngx;
    // float *ngy;
    // float *ngz;

    // float *tax;
    // float *tay;

    // float *tbx;
    // float *tby;

    float *arr;
    uint32_t offset;

    uint32_t *obj_idx;
    Material **mat_ptr_arr;
} SoATriangle;

#endif
