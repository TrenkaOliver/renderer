#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "math/vec.h"

typedef struct Triangle {
    int const_normal;
    Vec a;
    Vec b;
    Vec c;
    Vec ng;
    Vec na;
    Vec nb;
    Vec nc;
} Triangle;

#endif
