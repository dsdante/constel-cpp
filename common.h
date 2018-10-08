#ifndef COMMON_H
#define COMMON_H

#include "linmath.h"

extern struct config
{
    int stars;
    double star_speed;
    double gravity;
    double epsilon;
    double speed;
    double min_fps;
    double max_fps;
    int msaa;
    vec4 star_color;
    char* font;
    double text_size;
    vec4 text_color;
} config;

char* read_file(const char *filename, int *length);
void finalize_config();
void init_config(const char* filename);
float get_fps(int frame);
float get_fps_period(float period);
void set_fps(float value);

#endif // COMMON_H
