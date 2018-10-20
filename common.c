#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GLFW/glfw3.h>
#include "common.h"
#include "linmath.h"

#define FPS_BUFF_SIZE 256
#define DEFAULT_CONFIG_FILE "constel.conf"
#define CONFIG_STARS "Stars"
#define CONFIG_GALAXY_DENSITY "GalaxyDens"
#define CONFIG_STAR_SPEED "StarSpeed"
#define CONFIG_GRAVITY "Gravity"
#define CONFIG_EPSILON "Epsilon"
#define CONFIG_ACCURACY "Accuracy"
#define CONFIG_SPEED "Speed"
#define CONFIG_MIN_FPS "MinFPS"
#define CONFIG_MAX_FPS "MaxFPS"
#define CONFIG_DEFAULT_ZOOM "DefaultZoom"
#define CONFIG_MSAA "MSAA"
#define CONFIG_STAR_COLOR "StarColor"
#define CONFIG_SHOW_STATUS "ShowStatus"
#define CONFIG_FONT "Font"
#define CONFIG_TEXT_SIZE "TextSize"
#define CONFIG_TEXT_COLOR "TextColor"

vec2* disp_stars = NULL; // display values, float

// Must be freed by the caller
char* read_file(const char *filename, int *length)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));
        *length = 0;
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    rewind(file);
    char* buffer = (char*) malloc((*length + 1) * sizeof(char));
    *length = fread(buffer, 1, *length, file);
    fclose(file);
    buffer[*length] = '\0';
    return buffer;
}

// returns actual frame duration
double frame_sleep()
{
    static double last_time = NAN;
    if (isnan(last_time))
        last_time = glfwGetTime() - 1.0/config.max_fps;
    double last_interval = glfwGetTime() - last_time;
    double sleep_interval = 1.0/config.max_fps - last_interval;
    if (sleep_interval > 0) {
        double intpart;
        const struct timespec sleep_ts = { sleep_interval, 1e+9*modf(sleep_interval,&intpart) };
        nanosleep(&sleep_ts, NULL);
    }
    last_interval = glfwGetTime() - last_time;
    last_time += last_interval;
    set_fps(1 / last_interval);
    return last_interval;
}

// ============================== Configuration ===============================

static float star_color[] = { 1, 0.8, 0, 1 };
static float text_color[] = { 0, 1, 0, 1 };
struct config config = {
    .stars = 7000,
    .galaxy_density = 10,
    .star_speed = 1.4,
    .gravity = 0.002,
    .epsilon = 2,
    .accuracy = 0.7,
    .speed = 1,
    .min_fps = 40,
    .max_fps = 60,
    .default_zoom = 25,
    .star_color = &star_color,
    .show_status = true,
    .font = "/usr/share/fonts/TTF/FreeSans.ttf",
    .text_size = 14,
    .text_color = &text_color,
};

static char* config_lines[1024] = { NULL };  // TODO: config saving
static size_t config_line_lengths[1024] = { 0 };

void finalize_config()
{
    for (int i = 0; i < sizeof(config_lines)/sizeof(config_lines[0]); i++)
        free(config_lines[i]);
}

void init_config(const char* filename)
{
    if (!filename)
        filename = DEFAULT_CONFIG_FILE;
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));
        return;
    }
    for (int n = 0; getline(&config_lines[n], &config_line_lengths[n], file) >= 0; n++) {
        char* key = config_lines[n];
        while(*key == ' ' || *key == '\t')
            key++;
        if (*key == '#' || *key == '\n' || *key == '\0')
            continue;
        char* value = key;
        while(*value != ' ' && *value != '\t' && *value != '\n' && *value != '\0')
            value++;
        while(*value == ' ' || *value == '\t')
            value++;
        if (*value == '\n' || *value == '\0')
            continue;
        char* value_end = value; // remove trailing newline
        while (*value_end)
            value_end++;
        if (*(--value_end) == '\n')
            *value_end = '\0';

        if (!strncmp(key, CONFIG_STARS, sizeof(CONFIG_STARS)-1)) {
            sscanf(value, "%d", &config.stars);
        } else if (!strncmp(key, CONFIG_GALAXY_DENSITY, sizeof(CONFIG_GALAXY_DENSITY)-1)) {
            sscanf(value, "%lf", &config.galaxy_density);
        } else if (!strncmp(key, CONFIG_STAR_SPEED, sizeof(CONFIG_STAR_SPEED)-1)) {
            sscanf(value, "%lf", &config.star_speed);
        } else if (!strncmp(key, CONFIG_GRAVITY, sizeof(CONFIG_GRAVITY)-1)) {
            sscanf(value, "%lf", &config.gravity);
        } else if (!strncmp(key, CONFIG_EPSILON, sizeof(CONFIG_EPSILON)-1)) {
            sscanf(value, "%lf", &config.epsilon);
        } else if (!strncmp(key, CONFIG_ACCURACY, sizeof(CONFIG_ACCURACY)-1)) {
            sscanf(value, "%lf", &config.accuracy);
        } else if (!strncmp(key, CONFIG_SPEED, sizeof(CONFIG_SPEED)-1)) {
            sscanf(value, "%lf", &config.speed);
        } else if (!strncmp(key, CONFIG_MIN_FPS, sizeof(CONFIG_MIN_FPS)-1)) {
            sscanf(value, "%lf", &config.min_fps);
        } else if (!strncmp(key, CONFIG_MAX_FPS, sizeof(CONFIG_MAX_FPS)-1)) {
            sscanf(value, "%lf", &config.max_fps);
        } else if (!strncmp(key, CONFIG_DEFAULT_ZOOM, sizeof(CONFIG_DEFAULT_ZOOM)-1)) {
            sscanf(value, "%lf", &config.default_zoom);
        } else if (!strncmp(key, CONFIG_MSAA, sizeof(CONFIG_MSAA)-1)) {
            sscanf(value, "%d", &config.msaa);
        } else if (!strncmp(key, CONFIG_STAR_COLOR, sizeof(CONFIG_STAR_COLOR)-1)) {
            sscanf(value, "%f %f %f %f", &star_color[0], &star_color[1], &star_color[2], &star_color[3]);
        } else if (!strncmp(key, CONFIG_SHOW_STATUS, sizeof(CONFIG_SHOW_STATUS)-1)) {
            config.show_status = !strncmp(value, "true", 4)
                              || !strncmp(value, "True", 4)
                              || !strncmp(value, "1", 1);
        } else if (!strncmp(key, CONFIG_FONT, sizeof(CONFIG_FONT)-1)) {
            config.font = value;
        } else if (!strncmp(key, CONFIG_TEXT_SIZE, sizeof(CONFIG_TEXT_SIZE)-1)) {
            sscanf(value, "%lf", &config.text_size);
        } else if (!strncmp(key, CONFIG_TEXT_COLOR, sizeof(CONFIG_TEXT_COLOR)-1)) {
            sscanf(value, "%f %f %f %f", &text_color[0], &text_color[1], &text_color[2], &text_color[3]);
        }
    }
    fclose(file);
}


// =========================== Performance counters ===========================

double perf_build;
double perf_accel;
double perf_draw;
static float fps_history[FPS_BUFF_SIZE];
static int fps_count = 0;
static int fps_pointer = 0;

// Get mean FPS among the last [frame] values
float get_fps(int frame)
{
    if (fps_count == 0)
        return 0;
    if (frame < 1)
        frame = 1;
    if (frame > fps_count)
        frame = fps_count;
    float sum = 0;
    for (int i = (fps_pointer - frame + FPS_BUFF_SIZE) % FPS_BUFF_SIZE;
            i != fps_pointer; i = (i + 1) % FPS_BUFF_SIZE)
        sum += fps_history[i];
    return sum / frame;
}

// Get mean FPS for the last [period] seconds, according to the last FPS value
float get_fps_period(float period)
{
    return get_fps(
            period * fps_history[(fps_pointer - 1 + FPS_BUFF_SIZE) % FPS_BUFF_SIZE]);
}

// Append a new FPS value
void set_fps(float value)
{
    fps_history[fps_pointer] = value;
    fps_pointer++;
    fps_pointer %= FPS_BUFF_SIZE;
    if (fps_count < FPS_BUFF_SIZE)
        fps_count++;
}
