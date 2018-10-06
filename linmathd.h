#ifndef LINMATHD_H
#define LINMATHD_H

#include <math.h>
#include "linmath.h"

typedef struct
{
    double x;
    double y;
} vecd2;

static inline vecd2 vecd2_sub(vecd2 a, vecd2 b)
{
    return (vecd2){ a.x-b.x, a.y-b.y };
}

static inline double vecd2_sqr(vecd2 vec)
{
    return vec.x*vec.x + vec.y*vec.y;
}

static inline double vecd2_len(vecd2 vec)
{
    return sqrt(vecd2_sqr(vec));
}

static inline double vecd2_angle(vecd2 vec)
{
    return atan2(vec.y, vec.x);
}

static inline void to_vec2_array(vecd2 *from, vec2 *to, int count)
{
    double *_from = (double*)from;
    float *_to = (float*)to;
    for (int i = 0; i < 2*count; i++)
        _to[i] = _from[i]; // TODO: SIMD
}

#endif // LINMATHD_H
