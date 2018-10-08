#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define DEFAULT_CONFIG_FILE "constel.conf"
#define CONFIG_GROUP "Constel"
#define CONFIG_STARS "Stars"
#define CONFIG_GRAVITY "Gravity"
#define CONFIG_EPSILON "Epsilon"
#define CONFIG_SPEED "Speed"
#define CONFIG_MINFPS "MinFPS"
#define CONFIG_MAXFPS "MaxFPS"
#define CONFIG_MSAA "MSAA"
#define CONFIG_STARCOLOR "StarColor"
#define CONFIG_TEXTCOLOR "TextColor"
#define CONFIG_FONT "Font"
#define CONFIG_TEXTSIZE "TextSize"
#define FPS_BUFF_SIZE 256

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

// ============================== Configuration ===============================

struct config config = {
    .stars = 400,
    .gravity = 0.02,
    .epsilon = 0.1,
    .speed = 2,
    .min_fps = 30,
    .max_fps = 60,
    .star_color = (vec4){ 1, 0.8, 0, 1 },
    .font = "/usr/share/fonts/TTF/FreeSans.ttf",
    .text_size = 14,
    .text_color = (vec4){ 0, 1, 0, 1 },
};

char* config_lines[1024] = { NULL };
size_t config_line_lengths[1024] = { 0 };

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
        } else if (!strncmp(key, CONFIG_GRAVITY, sizeof(CONFIG_GRAVITY)-1)) {
            sscanf(value, "%lf", &config.gravity);
        } else if (!strncmp(key, CONFIG_EPSILON, sizeof(CONFIG_EPSILON)-1)) {
            sscanf(value, "%lf", &config.epsilon);
        } else if (!strncmp(key, CONFIG_SPEED, sizeof(CONFIG_SPEED)-1)) {
            sscanf(value, "%lf", &config.speed);
        } else if (!strncmp(key, CONFIG_MINFPS, sizeof(CONFIG_MINFPS)-1)) {
            sscanf(value, "%lf", &config.min_fps);
        } else if (!strncmp(key, CONFIG_MAXFPS, sizeof(CONFIG_MAXFPS)-1)) {
            sscanf(value, "%lf", &config.max_fps);
        } else if (!strncmp(key, CONFIG_MSAA, sizeof(CONFIG_MSAA)-1)) {
            sscanf(value, "%d", &config.msaa);
        } else if (!strncmp(key, CONFIG_STARCOLOR, sizeof(CONFIG_STARCOLOR)-1)) {
            sscanf(value, "%f %f %f %f", &config.star_color[0], &config.star_color[1], &config.star_color[2], &config.star_color[3]);
        } else if (!strncmp(key, CONFIG_TEXTCOLOR, sizeof(CONFIG_TEXTCOLOR)-1)) {
            sscanf(value, "%f %f %f %f", &config.text_color[0], &config.text_color[1], &config.text_color[2], &config.text_color[3]);
        } else if (!strncmp(key, CONFIG_FONT, sizeof(CONFIG_FONT)-1)) {
            config.font = value;
        } else if (!strncmp(key, CONFIG_TEXTSIZE, sizeof(CONFIG_TEXTSIZE)-1)) {
            sscanf(value, "%lf", &config.text_size);
        }
    }
    fclose(file);
}

// =============================== FPS counter ================================

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
