#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "math/vec.h"

typedef struct Triangle {
    Vec a;
    Vec b;
    Vec c;
    Vec n;
} Triangle;

#endif
