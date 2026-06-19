#include <stdio.h>
#include "geometry/vec.h"
#include "scene.h"
#include "camera.h"

typedef struct Color {
    double r;
    double g;
    double b;
} Color;

typedef struct Pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Pixel;

Pixel pixel(unsigned char r, unsigned char g, unsigned char b);
Pixel color_to_pixel(Vec c);

int render(FILE *f, int width, int height, Scene *scene, Camera *cam);