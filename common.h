#ifndef COMMON_H
#define COMMON_H

#include <math.h>
#include <stdbool.h>
#include "linmath.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct vecd2
{
    double x;
    double y;
};

extern struct config
{
    int stars;
    double galaxy_density;
    double star_speed;  // star starting speed factor
    double gravity;
    double epsilon;  // minimum effective distance
    double accuracy;  // minimum effective distance
    double speed;  // simulation speed factor
    double min_fps;  // maximum sumulation frame = 1/FPS
    double max_fps;
    double default_zoom;
    int msaa;  // anti-alisaing samples
    vec4* star_color;
    bool show_status;
    char* font;
    double text_size;
    vec4* text_color;
} config;

extern vec2* disp_stars;
extern double perf_build;
extern double perf_accel;
extern double perf_draw;

char* read_file(const char *filename, int *length);
double frame_sleep();
void init_config(const char* filename);
void finalize_config();
float get_fps(int frame);
float get_fps_period(float period);
void set_fps(float value);

#endif // COMMON_H
