#ifndef WORLD_H
#define WORLD_H
#include <stdbool.h>
#include "linmath.h"

void init_world();
void world_frame(double time);
void finalize_world();

#endif // WORLD_H
