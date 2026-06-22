#include <math.h>
#include "geometry/object.h"

Line create_line(Vec o, Vec v) {
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


void move_triangle(Triangle *triangle, Vec v) {
    triangle->a = v_add(triangle->a, v);
    triangle->b = v_add(triangle->b, v);
    triangle->c = v_add(triangle->c, v);
}

