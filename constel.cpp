#include <memory>
#include <string>

#include <stdlib.h>
#include <time.h>
#include <GLFW/glfw3.h>

#include "common.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "world.hpp"

void exit_finalize(int code)
{
    finalize_graphics();
    finalize_world();
    exit(code);
}

int main(int argc, char **argv)
{
    time_t seed = time(NULL);
    //printf("Random seed: 0x%lx\n", seed);
    srand(seed);
    std::string config_file;
    if (argc >= 2)
        config_file = argv[1];
    config.load(config_file);
    init_world();
    GLFWwindow* window = init_graphics();
    if (!window)
        exit_finalize(1);
    input.initialize(window);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double time = frame_sleep();
        input.frame();
        world_frame(time);
        draw();
    }

    exit_finalize(0);
}
