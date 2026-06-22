#ifndef BOX_H
#define BOX_H

#include "math/vec.h"

typedef struct Box {
    Vec center;
    Vec axes[3];
    Vec half_size;
} Box;

#endif