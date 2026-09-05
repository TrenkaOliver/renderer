#ifndef PACKED_RAY_H
#define PACKED_RAY_H

#include "math/packed_vec.h"

typedef struct PackedRay {
    ps_Vec o;
    ps_Vec v;
    ps_Vec inv_v;
} PackedRay;

PackedRay packed_create_ray(ps_Vec o, ps_Vec v);

#endif