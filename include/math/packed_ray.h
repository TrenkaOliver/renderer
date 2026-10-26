#ifndef PACKED_RAY_H
#define PACKED_RAY_H

#include "math/packed_vec.h"

typedef struct PackedRay {
    ps_Vec o;
    ps_Vec v;
    ps_Vec inv_v;
} PackedRay;

static inline PackedRay packed_create_ray(const ps_Vec o, const ps_Vec v) {
    const ps_Vec inv_v = ps_reciprocal(v);
    
    return (PackedRay) {
        .o = o,
        .v = v,
        .inv_v = inv_v,
    };
}

#endif