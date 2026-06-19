#ifndef CAMERA_H
#define CAMERA_H

#include "geometry/vec.h"
#include "geometry/object.h"

typedef struct Camera {
    Vec position;
    Vec forward;
    Vec right;
    Vec up;
} Camera;

Camera create_camera(Vec position, Vec rotation);

Camera create_look_at_camera(Vec position, Vec look_at);

#endif