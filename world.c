#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "linmath.h"
#include "linmathd.h"
#include "world.h"

vec2* pos_display; // display values, float
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
    pos_display = malloc(config.stars * sizeof(vec2));
    pos = malloc(config.stars * sizeof(vecd2));
    speed = malloc(config.stars * sizeof(vecd2));
    force = malloc(config.stars * sizeof(vecd2));
    for (int i = 0; i < config.stars; i++) {
        double d = frand(0, 3);
        double dir = frand(0, 2*M_PI);
        pos[i] = (vecd2){ d * cos(dir), d * sin(dir) };
        speed[i] = (vecd2){ config.star_speed * pow(d, 0.25) * sin(dir), -config.star_speed * pow(d, 0.25) * cos(dir) };
    }
    /*
    config.stars = 3;
    config.gravity = 0.02;
    config.epsilon = 0.1;
    config.speed = 2;
    pos[0].x = 0.1;
    pos[0].y = 0;
    pos[1].x = -0.1;
    pos[1].y = 0;
    pos[2].x = 100;
    pos[2].y = 100;
    speed[0].x = 0;
    speed[0].y = -0.01;
    speed[1].x = 0;
    speed[1].y = 0.01;
    speed[2].x = 100;
    speed[2].y = 100;
    */
}

void world_frame(double time)
{
    if (time > 1/config.min_fps)
        time = 1/config.min_fps;

    // Calculate forces
    memset(force, 0, sizeof(vecd2) * config.stars);
    for (int i = 0; i < config.stars-1; i++) {
        for (int k = i+1; k < config.stars; k++) {
            vecd2 diff = vecd2_sub(pos[k], pos[i]);
            double sqr = vecd2_sqr(diff);
            if (sqr == 0)
                continue;
            double angle = vecd2_angle(diff);
            double _f = config.gravity / (sqr + config.epsilon);
            vecd2 f = (vecd2){ _f * cos(angle), _f * sin(angle) };
            force[i].x += f.x;
            force[i].y += f.y;
            force[k].x -= f.x;
            force[k].y -= f.y;
        }
    }

    // Calculate speed and coordinates
    for (int i = 0; i < config.stars; i++) {
        vecd2 _speed = (vecd2){ speed[i].x + time * config.speed * force[i].x, speed[i].y + time * config.speed * force[i].y };
        pos[i].x += time * config.speed * (speed[i].x + _speed.x) / 2;
        pos[i].y += time * config.speed * (speed[i].y + _speed.y) / 2;
        speed[i] = _speed;
    }
    to_vec2_array(pos, pos_display, config.stars);
}
