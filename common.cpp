#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>
#include <GLFW/glfw3.h>
#include "common.hpp"

vec2* disp_star_position = nullptr;  // display coordinates, float
vec3* disp_star_color = nullptr;  // star colors

std::string read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::ate);
    std::string content;
    content.reserve(file.tellg());
    file.seekg(0);
    content.assign((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    return content;
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

Config config;

void Config::load(const std::string& filename)
{
    if (!filename.empty())
        this->filename = filename;
    std::ifstream file(this->filename);
    std::string line;
    std::regex regex(R"(^\s*(.+?)\s+(.*?)\s*(?:#.*)?$)");  // (key) (values with spaces) # comment
    std::smatch match;
    while (std::getline(file, line)) {
        if (!std::regex_match(line, match, regex))
            continue;
        try {
            Parameter key = parameter_names.at(match[1].str());
            const std::string& value = match[2].str();
            switch (key) {
            case Parameter::stars:          config.stars          = std::stoi(value); break;
            case Parameter::galaxy_density: config.galaxy_density = std::stod(value); break;
            case Parameter::star_speed:     config.star_speed     = std::stod(value); break;
            case Parameter::gravity:        config.gravity        = std::stod(value); break;
            case Parameter::epsilon:        config.epsilon        = std::stod(value); break;
            case Parameter::accuracy:       config.accuracy       = std::stod(value); break;
            case Parameter::speed:          config.speed          = std::stod(value); break;
            case Parameter::min_fps:        config.min_fps        = std::stod(value); break;
            case Parameter::max_fps:        config.max_fps        = std::stod(value); break;
            case Parameter::default_zoom:   config.default_zoom   = std::stod(value); break;
            case Parameter::msaa:           config.msaa           = std::stoi(value); break;
            case Parameter::show_status:    config.show_status    = IgnoreCase()(value, "true") || (value == "1"); break;
            case Parameter::font:           config.font           = value; break;
            case Parameter::text_size:      config.text_size      = std::stoi(value); break;
            case Parameter::text_color:
                std::stringstream strstr(value);
                strstr >> config.text_color[0] >> config.text_color[1] >> config.text_color[2] >> config.text_color[3];
                break;
            }
        } catch (const std::out_of_range&) {
            // Do nothing.
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
