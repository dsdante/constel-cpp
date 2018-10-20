#include <stdlib.h>
#include <time.h>
#include <GLFW/glfw3.h>
#include "common.h"
#include "graphics.h"
#include "input.h"
#include "world.h"

void exit_finalize(int code)
{
    finalize_input();
    finalize_graphics();
    finalize_world();
    finalize_config();
    exit(code);
}

int main(int argc, char **argv)
{
    time_t seed = time(NULL);
    //printf("Random seed: 0x%lx\n", seed);
    srand(seed);
    char* config_file = NULL;
    if (argc >= 2)
        config_file = argv[1];
    init_config(config_file);
    init_world();
    GLFWwindow* window = init_graphics();
    if (!window)
        exit_finalize(1);
    init_input(window);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double time = frame_sleep();
        process_input(window);
        world_frame(time);
        draw();
    }

    exit_finalize(0);
}
