#include "camera.h"
#include "math.h"

Vec WORLD_UP = {.x = 0.0, .y = 0.0, .z = 1.0};

Camera create_camera(Vec position, Vec rotation, double fov) {
    Vec forward, right, up;

    forward.x = sin(rotation.x) * sin(rotation.y) * cos(rotation.z) - cos(rotation.x) * sin(rotation.z);
    forward.y = sin(rotation.x) * sin(rotation.y) * sin(rotation.z) + cos(rotation.y) * cos(rotation.z);
    forward.z = sin(rotation.x) * cos(rotation.y);

    forward = normalize(forward);
    
    right = normalize(cross(forward, WORLD_UP));
    up = cross(right, forward);

    return (Camera) {.position = position, .forward = forward, .right = right, .up = up, .fov = fov};
}

Camera create_look_at_camera(Vec position, Vec look_at, double fov) {
    Vec forward, right, up;

    forward = normalize(v_sub(look_at, position));
    right = normalize(cross(forward, WORLD_UP));
    up = cross(right, forward);

    return (Camera) {.position = position, .forward = forward, .right = right, .up = up, .fov = fov};
}
