#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>
#include <GLFW/glfw3.h>
#include "common.hpp"
#include "linmath.h"

std::string config_filename = "constel.conf";
const std::string config_stars = "Stars";
const std::string config_galaxy_density = "GalaxyDens";
const std::string config_star_speed = "StarSpeed";
const std::string config_gravity = "Gravity";
const std::string config_epsilon = "Epsilon";
const std::string config_accuracy = "Accuracy";
const std::string config_speed = "Speed";
const std::string config_min_fps = "MinFPS";
const std::string config_max_fps = "MaxFPS";
const std::string config_default_zoom = "DefaultZoom";
const std::string config_msaa = "MSAA";
const std::string config_show_status = "ShowStatus";
const std::string config_font = "Font";
const std::string config_text_size = "TextSize";
const std::string config_text_color = "TextColor";

vec2* disp_star_position = NULL; // display coordinates, float
vec3* disp_star_color = NULL; // star colors

bool equal_icase(const std::string& a, const std::string& b)
{
    if (a.length() != b.length())
        return false;
    return std::equal(a.begin(), a.end(), b.begin(),
            [](char ca, char cb){ return std::tolower(ca) == std::tolower(cb); });
}

const std::string read_file(const std::string& filename)
{
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// returns actual frame duration
double frame_sleep()
{
    static double last_time = -1.0;
    if (last_time < 0)
        last_time = glfwGetTime() - 1.0/config.max_fps;
    double last_interval = glfwGetTime() - last_time;
    double sleep_interval = 1.0/config.max_fps - last_interval;
    if (sleep_interval > 0)
        std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6 * sleep_interval)));
    last_interval = glfwGetTime() - last_time;
    last_time += last_interval;
    add_fps(1 / last_interval);
    return last_interval;
}

// ============================== Configuration ===============================

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
    .show_status = true,
    .font = "/usr/share/fonts/TTF/FreeSans.ttf",
    .text_size = 14,
    .text_color = { 0, 1, 0, 1 },
};

void finalize_config()
{
}

void init_config(const std::string& filename)
{
    if (filename.length() > 0)
        config_filename = filename;
    std::ifstream file(config_filename);
    std::string line;
    std::regex regex(R"(^\s*(.+?)\s+(.*?)\s*(?:#.*)?$)");  // (key) (values with spaces) # comment
    std::smatch match;
    while (std::getline(file, line)) {
        if (!std::regex_match(line, match, regex))
            continue;
        auto key = match[1].str();
        auto value = match[2].str();
        if (equal_icase(key, config_stars)) {
            config.stars = std::stoi(value);
        } else if (equal_icase(key, config_galaxy_density)) {
            config.galaxy_density = std::stod(value);
        } else if (equal_icase(key, config_star_speed)) {
            config.star_speed = std::stod(value);
        } else if (equal_icase(key, config_gravity)) {
            config.gravity = std::stod(value);
        } else if (equal_icase(key, config_epsilon)) {
            config.epsilon = std::stod(value);
        } else if (equal_icase(key, config_accuracy)) {
            config.accuracy = std::stod(value);
        } else if (equal_icase(key, config_speed)) {
            config.speed = std::stod(value);
        } else if (equal_icase(key, config_min_fps)) {
            config.min_fps = std::stod(value);
        } else if (equal_icase(key, config_max_fps)) {
            config.max_fps = std::stod(value);
        } else if (equal_icase(key, config_default_zoom)) {
            config.default_zoom = std::stod(value);
        } else if (equal_icase(key, config_msaa)) {
            config.msaa = std::stoi(value);
        } else if (equal_icase(key, config_show_status)) {
            config.show_status = equal_icase(value, "true") || (value == "1");
        } else if (equal_icase(key, config_font)) {
            config.font = value;
        } else if (equal_icase(key, config_text_size)) {
            config.text_size = std::stoi(value);
        } else if (equal_icase(key, config_text_color)) {
            std::stringstream strstr(value);
            strstr >> config.text_color[0] >> config.text_color[1] >> config.text_color[2] >> config.text_color[3];
        }
    }
}


// =========================== Performance counters ===========================

static std::vector<float> fps_buff(256, 0);
static size_t fps_count = 0;
static size_t fps_pointer = 0;

// Get mean FPS among the last [frame] values
float get_fps(size_t frame)
{
    if (fps_count == 0)
        return 0;
    if (frame < 1)
        frame = 1;
    if (frame > fps_count)
        frame = fps_count;
    float sum = 0;
    for (size_t i = (fps_pointer - frame + fps_buff.size()) % fps_buff.size();
             i != fps_pointer; i = (i + 1) % fps_buff.size())
        sum += fps_buff[i];
    return sum / frame;
}

// Get mean FPS for the last [period] seconds, according to the last FPS value
float get_fps_period(float period)
{
    return get_fps(period * fps_buff[(fps_pointer - 1 + fps_buff.size()) % fps_buff.size()]);
}

// Append a new FPS value
void add_fps(float value)
{
    fps_buff[fps_pointer] = value;
    fps_pointer++;
    fps_pointer %= fps_buff.size();
    if (fps_count < fps_buff.size())
        fps_count++;
}
