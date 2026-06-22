#include <math.h>

#include "render/render.h"
#include "math/vec.h"

Pixel create_pixel(unsigned char r, unsigned char g, unsigned char b) {
    return (Pixel) {
        .r = r, 
        .g = g, 
        .b = b
    };
}

Pixel color_to_pixel(Vec c) {
    return (Pixel) {
        .r = fmin(c.x, 1.0) * 255,
        .g = fmin(c.y, 1.0) * 255,
        .b = fmin(c.z, 1.0) * 255,
    };
}
