#include "math/vec.h"
#include "math/ray.h"

Ray create_ray(Vec o, Vec v) {
    return (Ray) {
        .o = o,
        .v = normalize(v)
    };
}