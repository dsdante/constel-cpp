#ifndef WORLD_H_
#define WORLD_H_
#include <stdbool.h>
#include "linmath.h"

#define PARTICLE_COUNT 400

extern vec2 pos_display[PARTICLE_COUNT];

void init_world();
void world_frame(double time);
void finalize_world();

#endif // WORLD_H_
