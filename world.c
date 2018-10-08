#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "linmath.h"
#include "linmathd.h"
#include "world.h"

vec2* pos_display;
vecd2* pos;
vecd2* speed;
vecd2* force;

void finalize_world()
{
    free(pos_display);
    free(pos);
    free(speed);
    free(force);
}

static double frand(double min, double max)
{
    return (double)rand()/RAND_MAX * (max-min) + min;
}

void init_world()
{
    pos_display = malloc(config.particles * sizeof(vec2));
    pos = malloc(config.particles * sizeof(vecd2));
    speed = malloc(config.particles * sizeof(vecd2));
    force = malloc(config.particles * sizeof(vecd2));
    for (int i = 0; i < config.particles/2; i++) {
        pos[i] = (vecd2){ frand(-2, 2) - 3, frand(-2, 2) };
        speed[i].y = -0.5;
    }
    for (int i = config.particles/2; i < config.particles; i++) {
        pos[i] = (vecd2) { frand(-2, 2) + 3, frand(-2, 2) };
        speed[i].y = 0.5;
    }
    /*
    pos[0].x = 0;
    pos[0].y = 0;
    pos[1].x = 0.1;
    pos[1].y = 0;
    pos[2].x = 10;
    pos[2].y = 10;
    speed[0].x = 0;
    speed[0].y = -0.02;
    speed[1].x = 0;
    speed[1].y = 0.02;
    speed[2].x = 10;
    speed[2].y = 10;
    */
}

void world_frame(double time)
{
    if (time > 1/config.min_fps)
        time = 1/config.min_fps;
    const double g = 0.02;
    const double epsilon = 0.1; // TODO: reduce to zero

    memset(force, 0, sizeof(vecd2) * config.particles);
    for (int i = 0; i < config.particles-1; i++) {
        for (int k = i+1; k < config.particles; k++) {
            vecd2 diff = vecd2_sub(pos[k], pos[i]);
            double sqr = vecd2_sqr(diff);
            if (sqr == 0)
                continue;
            double angle = vecd2_angle(diff);
            double _f = g / (sqr + epsilon);
            vecd2 f = (vecd2){ _f * cos(angle), _f * sin(angle) };
            force[i].x += f.x;
            force[i].y += f.y;
            force[k].x -= f.x;
            force[k].y -= f.y;
        }
    }
    for (int i = 0; i < config.particles; i++) {
        vecd2 _speed = (vecd2){ speed[i].x + time * config.speed * force[i].x, speed[i].y + time * config.speed * force[i].y };
        pos[i].x += time * config.speed * (speed[i].x + _speed.x) / 2;
        pos[i].y += time * config.speed * (speed[i].y + _speed.y) / 2;
        speed[i] = _speed;
    }
    to_vec2_array(pos, pos_display, config.particles);
}
