#include "math/packed_ray.h"

PackedRay packed_create_ray(ps_Vec o, ps_Vec v) {
    ps_Vec nv = ps_normalize(v);
    ps_Vec inv_v = ps_reciprocal(nv);
    
    return (PackedRay) {
        .o = o,
        .v = nv,
        .inv_v = inv_v,
    };
}