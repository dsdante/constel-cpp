#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "linmath.h"
#include "world.h"

#define SPEED 3

vec2 pos[PARTICLE_COUNT];
vec2 speed[PARTICLE_COUNT];
vec2 pos_swap[PARTICLE_COUNT];
vec2 speed_swap[PARTICLE_COUNT];
vec2 force[PARTICLE_COUNT];

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
    memcpy(pos_swap, pos, sizeof(vec2) * PARTICLE_COUNT);
    memcpy(speed_swap, speed, sizeof(vec2) * PARTICLE_COUNT);
}

void world_frame(double time)
{
    const double g = 0.0001;
    const double epsilon = 0.001; // avoid singularities with r ~= 0

    memset(force, 0, sizeof(vec2) * PARTICLE_COUNT);
    for (int i = 0; i < PARTICLE_COUNT-1; i++) {
        for (int k = i+1; k < PARTICLE_COUNT; k++) {
            double dx = pos[k][0] - pos[i][0];
            double dy = pos[k][1] - pos[i][1];
            if (dx == 0 && dy == 0)
                continue;
            double angle = atan2(dy, dx);
            double f = g / (dx*dx + dy*dy + epsilon);
            double fx = f * cos(angle);
            double fy = f * sin(angle);
            force[i][0] += fx;
            force[i][1] += fy;
            force[k][0] -= fx;
            force[k][1] -= fy;
        }
    }
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        speed_swap[i][0] += time * SPEED * force[i][0];
        speed_swap[i][1] += time * SPEED * force[i][1];
        pos_swap[i][0] += time * SPEED * (speed[i][0] + speed_swap[i][0]) / 2;
        pos_swap[i][1] += time * SPEED * (speed[i][1] + speed_swap[i][1]) / 2;
    }
    memcpy(pos, pos_swap, sizeof(vec2) * PARTICLE_COUNT);
    memcpy(speed, speed_swap, sizeof(vec2) * PARTICLE_COUNT);
}
