#ifndef HITRESULT_H
#define HITRESULT_H

#include "geometry/vec.h"
#include "light.h"

typedef struct HitResult {
    Vec point;
    Vec normal;
    double t;
    Material *material;
} HitResult;

#endif