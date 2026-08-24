#include "accel/cmp.h"
#include "geometry/object.h"

int x_compare(const void *a, const void *b) {
    const Object *oa = *(const Object * const *)a;
    const Object *ob = *(const Object * const *)b;

    double ax = (oa->aabb.min[0] + oa->aabb.max[0]) * 0.5;
    double bx = (ob->aabb.min[0] + ob->aabb.max[0]) * 0.5;

    return (ax > bx) - (ax < bx);
}

int y_compare(const void *a, const void *b) {
    const Object *oa = *(const Object * const *)a;
    const Object *ob = *(const Object * const *)b;

    double ay = (oa->aabb.min[1] + oa->aabb.max[1]) * 0.5;
    double by = (ob->aabb.min[1] + ob->aabb.max[1]) * 0.5;

    return (ay > by) - (ay < by);
}

int z_compare(const void *a, const void *b) {
    const Object *oa = *(const Object * const *)a;
    const Object *ob = *(const Object * const *)b;

    double az = (oa->aabb.min[2] + oa->aabb.max[2]) * 0.5;
    double bz = (ob->aabb.min[2] + ob->aabb.max[2]) * 0.5;

    return (az > bz) - (az < bz);
}