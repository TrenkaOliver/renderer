#ifndef PACKED_RAY_H
#define PACKED_RAY_H

#include "math/packed_vec.h"

typedef struct ps_Ray {
    ps_Vec o;
    ps_Vec v;
    ps_Vec inv_v;
} ps_Ray;

ps_Ray create_ray(ps_Vec o, ps_Vec v);

#endif