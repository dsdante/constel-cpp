#ifndef WORLD_H
#define WORLD_H
#include <stdbool.h>
#include "linmath.h"

extern vec2* pos_display;

void init_world();
void world_frame(double time);
void finalize_world();

#endif // WORLD_H
