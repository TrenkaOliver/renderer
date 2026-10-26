#ifndef RENDER_H
#define RENDER_H

#include <stdio.h>
#include <stdint.h>
#include "math/vec.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "accel/bvh.h"

typedef struct RenderSettings {
    int height;
    int width;
    int max_depth;
    int aa_samples;
} RenderSettings;

int render(FILE * restrict f, const Scene * restrict scene, const Camera * restrict cam, const RenderSettings * restrict settings, const BVH8Tree * restrict bvh);

#endif