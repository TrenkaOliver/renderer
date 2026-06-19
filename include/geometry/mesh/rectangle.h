#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "geometry/object.h"

typedef struct Rectangle {
    Triangle *abd;
    Triangle *bcd;
} Rectangle;


Rectangle create_rectangle(Vec position, Vec rotation, Vec size, Material *m);

void move_rectangle(Rectangle *rect, Vec v);
void rotate_rectangle(Rectangle *rect, Vec rotation);


#endif