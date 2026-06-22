#ifndef VECTOR_H
#define VECTOR_H

typedef struct Vec {
    double x;
    double y;
    double z;
} Vec;

Vec vec(double x, double y, double z);

double len_sqr(Vec vec);
double len(Vec vec);

Vec v_min(Vec a, Vec b);
Vec v_max(Vec a, Vec b);

Vec neg(Vec vec);
Vec normalize(Vec vec);

Vec v_add(Vec a, Vec b);
Vec v_sub(Vec a, Vec b);

Vec scale(Vec vec, double value);
Vec v_div(Vec vec, double value);

double dot(Vec a, Vec b);
Vec cross(Vec a, Vec b);
Vec hadamard(Vec a, Vec b);

Vec rotate(Vec v, Vec r);

#endif