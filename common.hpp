#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <unordered_map>
#include "linmath.h"

struct vecd2
{
    double x;
    double y;
};

class Config
{
private:
    enum class Parameter
    {
        stars,
        galaxy_density,
        star_speed,
        gravity,
        epsilon,
        accuracy,
        speed,
        min_fps,
        max_fps,
        default_zoom,
        msaa,
        show_status,
        font,
        text_size,
        text_color,
    };

    // Hashing and comparing std::string ignoring case
    struct IgnoreCase
    {
        // djb2 hashing algorithm
        std::size_t operator()(const std::string& str) const
        {
            std::size_t hash = 5381;
            for (char c : str)
                hash = ((hash << 5) + hash) + std::tolower(c); // NOLINT(hicpp-signed-bitwise)
            return hash;
        }

        bool operator()(const std::string& l, const std::string& r) const
        {
            if (l.length() != r.length())
                return false;
            return std::equal(l.begin(), l.end(), r.begin(),
                    [](char cl, char cr){ return std::tolower(cl) == std::tolower(cr); });
        }
    };

    inline static const std::unordered_map<std::string, Parameter, IgnoreCase, IgnoreCase> parameter_names = {
            {"Stars", Parameter::stars},
            {"GalaxyDens", Parameter::galaxy_density},
            {"StarSpeed", Parameter::star_speed},
            {"Gravity", Parameter::gravity},
            {"Epsilon", Parameter::epsilon},
            {"Accuracy", Parameter::accuracy},
            {"Speed", Parameter::speed},
            {"MinFPS", Parameter::min_fps},
            {"MaxFPS", Parameter::max_fps},
            {"DefaultZoom", Parameter::default_zoom},
            {"MSAA", Parameter::msaa},
            {"ShowStatus", Parameter::show_status},
            {"Font", Parameter::font},
            {"TextSize", Parameter::text_size},
            {"TextColor", Parameter::text_color},
    };

public:
    void load(const std::string& filename);

    std::string filename = "constel.conf";
    int stars = 7000;
    double galaxy_density = 10;
    double star_speed = 1.4;  // star starting speed factor
    double gravity = 0.002;
    double epsilon = 2;  // minimum effective distance
    double accuracy = 0.7;  // minimum effective distance
    double speed = 1;  // simulation speed factor
    double min_fps = 40;  // maximum simulation frame = 1/FPS
    double max_fps = 60;
    double default_zoom = 25;
    int msaa = 0;  // anti-aliasing samples
    bool show_status = true;
    std::string font = "/usr/share/fonts/TTF/DejaVuSansMono.ttf";
    double text_size = 14;
    vec4 text_color = { 0, 1, 0, 1 };
};

extern Config config;

extern vec2* disp_star_position;
extern vec3* disp_star_color;
extern double perf_build;
extern double perf_accel;
extern double perf_draw;

std::string read_file(const std::string& filename);
double frame_sleep();
float get_fps(size_t frame);
float get_fps_period(float period);
void add_fps(float value);

#endif // COMMON_H
