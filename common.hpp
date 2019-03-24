#ifndef COMMON_H
#define COMMON_H

#include <string>
#include "linmath.h"

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
    bool show_status;
    std::string font;
    double text_size;
    vec4 text_color;
} config;

extern vec2* disp_star_position;
extern vec3* disp_star_color;
extern double perf_build;
extern double perf_accel;
extern double perf_draw;

const std::string read_file(const std::string& filename);
double frame_sleep();
void init_config(const std::string& filename);
void finalize_config();
float get_fps(size_t frame);
float get_fps_period(float period);
void add_fps(float value);

#endif // COMMON_H
