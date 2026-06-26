#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "math/vec.h"

typedef struct Triangle {
    int const_normal;
    int has_texture;
    Vec a;
    Vec b;
    Vec c;
    Vec ng;
    Vec na;
    Vec nb;
    Vec nc;
    Vec ta;
    Vec tb;
    Vec tc;
} Triangle;

#endif
