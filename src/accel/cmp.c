#include "accel/cmp.h"
#include "geometry/object.h"

int x_compare(const void *a, const void *b) {
    const Object *oa = *(const Object * const *)a;
    const Object *ob = *(const Object * const *)b;

    double ax = (oa->aabb.min.x + oa->aabb.max.x) * 0.5;
    double bx = (ob->aabb.min.x + ob->aabb.max.x) * 0.5;

    return (ax > bx) - (ax < bx);
}

int y_compare(const void *a, const void *b) {
    const Object *oa = *(const Object * const *)a;
    const Object *ob = *(const Object * const *)b;

    double ay = (oa->aabb.min.y + oa->aabb.max.y) * 0.5;
    double by = (ob->aabb.min.y + ob->aabb.max.y) * 0.5;

    return (ay > by) - (ay < by);
}

int z_compare(const void *a, const void *b) {
    const Object *oa = *(const Object * const *)a;
    const Object *ob = *(const Object * const *)b;

    double az = (oa->aabb.min.z + oa->aabb.max.z) * 0.5;
    double bz = (ob->aabb.min.z + ob->aabb.max.z) * 0.5;

    return (az > bz) - (az < bz);
}