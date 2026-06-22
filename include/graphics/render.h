#ifndef RENDER_H
#define RENDER_H

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

typedef struct RenderSettings {
    int height;
    int width;
    int max_depth;
    int aa_samples;
} RenderSettings;

Pixel pixel(unsigned char r, unsigned char g, unsigned char b);
Pixel color_to_pixel(Vec c);

int render(FILE *f, Scene *scene, Camera *cam, RenderSettings *settings);

#endif