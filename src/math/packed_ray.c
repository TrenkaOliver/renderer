#include "math/packed_ray.h"

ps_Ray create_ray(ps_Vec o, ps_Vec v) {
    ps_Vec nv = ps_normalize(v);
    ps_Vec inv_v = ps_reciprocal(nv);
    
    return (ps_Ray) {
        .o = o,
        .v = nv,
        .inv_v = inv_v,
    };
}