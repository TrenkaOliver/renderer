#include "accel/cmp.h"
#include "geometry/object.h"

int x_compare(const void *a, const void *b) {
    Vec a_center, b_center;

    a_center = calc_centroid(((Object *)a)->aabb);
    b_center = calc_centroid(((Object *)b)->aabb);

    if (a_center.x < b_center.x) return -1;
    if (a_center.x > b_center.x) return 1;
    return 0;
    
}

int y_compare(const void *a, const void *b) {
    Vec a_center, b_center;

    a_center = calc_centroid(((Object *)a)->aabb);
    b_center = calc_centroid(((Object *)b)->aabb);

    if (a_center.y < b_center.y) return -1;
    if (a_center.y > b_center.y) return 1;
    return 0;
}

int z_compare(const void *a, const void *b) {
    Vec a_center, b_center;

    a_center = calc_centroid(((Object *)a)->aabb);
    b_center = calc_centroid(((Object *)b)->aabb);

    if (a_center.z < b_center.z) return -1;
    if (a_center.z > b_center.z) return 1;
    return 0;
}