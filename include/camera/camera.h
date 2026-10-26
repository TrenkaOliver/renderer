#ifndef CAMERA_H
#define CAMERA_H

#include "math/vec.h"

typedef struct Camera {
    Vec position;
    Vec forward;
    Vec right;
    Vec up;
    double fov;
} Camera;

Camera create_camera(const Vec position, const Vec rotation, const double fov);

Camera create_look_at_camera(const Vec position, const Vec look_at, const double fov);

#endif