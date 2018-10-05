#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "linmath.h"
#include "world.h"

vec2 _pos[PARTICLE_COUNT];
vec2 _speed[PARTICLE_COUNT];
vec2 pos_swap[PARTICLE_COUNT];
vec2 speed_swap[PARTICLE_COUNT];
vec2* pos = _pos;
vec2* speed = _speed;

struct input input = { false, false, false, false };

void finalize_world()
{
}

static double frand(double min, double max)
{
    return (double)rand()/RAND_MAX * (max-min) + min;
}

void init_world()
{
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        pos[i][0] = frand(-1, 1);
        pos[i][1] = frand(-1, 1);
        speed[i][0] = frand(-0.1, 0.1);
        speed[i][1] = frand(-0.1, 0.1);
    }
    pos[0][0] = 0;
    pos[0][1] = 0;
    pos[1][0] = 0.05;
    pos[1][1] = 0;
    pos[2][0] = 10;
    pos[2][1] = 10;
    speed[0][0] = 0;
    speed[0][1] = -0.02;
    speed[1][0] = 0;
    speed[1][1] = 0.02;
    speed[2][0] = 10;
    speed[2][1] = 10;
}

void world_frame(double time)
{
    const double g = 0.0001;

    memcpy(pos_swap, pos, sizeof(vec2) * PARTICLE_COUNT);
    memcpy(speed_swap, speed, sizeof(vec2) * PARTICLE_COUNT);

    for (int i = 0; i < PARTICLE_COUNT; i++) {
        double forcex = 0;
        double forcey = 0;
        for (int k = 0; k < PARTICLE_COUNT; k++) {
            if (i == k)
                continue;
            double dx = pos[k][0] - pos[i][0];
            double dy = pos[k][1] - pos[i][1];
            double angle = atan2(dy, dx);
            double f = g / (dx*dx + dy*dy);
            forcex += f * cos(angle);
            forcey += f * sin(angle);
        }
        speed_swap[i][0] += time * forcex;
        speed_swap[i][1] += time * forcey;
        pos_swap[i][0] += time * speed_swap[i][0];
        pos_swap[i][1] += time * speed_swap[i][1];
        memcpy(pos, pos_swap, sizeof(vec2) * PARTICLE_COUNT);
        memcpy(speed, speed_swap, sizeof(vec2) * PARTICLE_COUNT);
    }
}
