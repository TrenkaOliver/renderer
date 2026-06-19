#ifndef BOX_H
#define BOX_H

#include "rectangle.h"

typedef struct Box {
    Rectangle x_pos;
    Rectangle x_neg;
    Rectangle y_pos;
    Rectangle y_neg;
    Rectangle z_pos;
    Rectangle z_neg;
} Box;

#endif