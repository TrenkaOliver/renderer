#ifndef RENDER_H
#define RENDER_H

#include <stdio.h>
#include <stdint.h>
#include "math/vec.h"
#include "math/packed_vec.h"
#include "scene/scene.h"
#include "camera/camera.h"

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

typedef struct PackedPixel {
    uint8_t r[8];
    uint8_t g[8];
    uint8_t b[8];
} PackedPixel;

Pixel create_pixel(unsigned char r, unsigned char g, unsigned char b);
Pixel color_to_pixel(Vec c);
PackedPixel color_to_packed_pixel(ps_Vec c);

int render(FILE *f, Scene *scene, Camera *cam, RenderSettings *settings);

#endif