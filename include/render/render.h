#ifndef RENDER_H
#define RENDER_H

#include <stdio.h>
#include <stdint.h>
#include "math/vec.h"
#include "scene/scene.h"
#include "camera/camera.h"

extern uint64_t visited_nodes;
extern uint64_t primitive_tests;
extern uint64_t first_object_called;
extern uint64_t s_visited_nodes;
extern uint64_t s_first_object_called;

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

Pixel create_pixel(unsigned char r, unsigned char g, unsigned char b);
Pixel color_to_pixel(Vec c);

int render(FILE *f, Scene *scene, Camera *cam, RenderSettings *settings);

#endif