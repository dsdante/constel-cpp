#ifndef WORLD_H_
#define WORLD_H_
#include <stdbool.h>
#include "linmath.h"

#define PARTICLE_COUNT 300

struct input {
    bool up;
    bool down;
    bool left;
    bool right;
};

extern vec2 pos[PARTICLE_COUNT];
extern vec2 speed[PARTICLE_COUNT];
extern struct input input;

void init_world();
void world_frame(double time);
void finalize_world();

#endif // WORLD_H_
