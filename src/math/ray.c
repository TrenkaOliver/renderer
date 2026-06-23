#include "math/vec.h"
#include "math/ray.h"

Ray create_ray(Vec o, Vec v) {
    Vec nv = normalize(v);
    Vec inv_v = vec(1.0 / nv.x, 1.0 / nv.y, 1.0 / nv.z);
    return (Ray) {
        .o = o,
        .v = nv,
        .inv_v = inv_v,
    };
}