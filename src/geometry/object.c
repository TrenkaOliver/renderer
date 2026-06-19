#include <math.h>
#include "geometry/object.h"

Line line(Vec o, Vec v) {
    return (Line) {
        .o = o,
        .v = normalize(v)
    };
}

Line line_from_origo(double x, double y, double z) {
    return (Line) {
        .o = vec(x, y, z),
        .v = normalize(vec(x, y, z)),
    };
}

// Sphere create_sphere(Vec o, double r, Material m) {
//     return (Sphere) {
//         .o = o,
//         .r = r,
//         .m = m,
//     };
// }

// Plane create_plane(Vec o, Vec n, Material m) {
//     return (Plane) {
//         .o = o,
//         .n = normalize(n),
//         .m = m,
//     };
// }

// Triangle create_triangle(Vec a, Vec b, Vec c, Material m) {
//     return (Triangle) {
//         .a = a,
//         .b = b,
//         .c = c,
//         .m = m,
//     };
// }

void move_triangle(Triangle *triangle, Vec v) {
    triangle->a = v_add(triangle->a, v);
    triangle->b = v_add(triangle->b, v);
    triangle->c = v_add(triangle->c, v);
}

