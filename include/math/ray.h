#ifndef RAY_H
#define RAY_H

#include "math/vec.h"

typedef struct Ray {
    Vec o;
    Vec v;
    Vec inv_v;
} Ray;

Ray create_ray(Vec o, Vec v);

#endif