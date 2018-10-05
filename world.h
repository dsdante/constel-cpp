#ifndef WORLD_H_
#define WORLD_H_
#include <stdbool.h>
#include "linmath.h"

#define PARTICLE_COUNT 100

struct input {
    bool up;
    bool down;
    bool left;
    bool right;
};

extern vec2* pos;
extern vec2* speed;
extern struct input input;

void init_world();
void world_frame(double time);
void finalize_world();

#endif // WORLD_H_
