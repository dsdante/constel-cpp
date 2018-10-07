#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "common.h"
#include "graphics.h"
#include "input.h"
#include "world.h"

#define TARGET_FPS 60

void exit_finalize(int code)
{
    finalize_input();
    finalize_graphics();
    finalize_world();
    exit(code);
}

// returns actual frame duration
double frame_sleep()
{
    static double last_time = NAN;
    if (isnan(last_time))
        last_time = glfwGetTime() - 1.0/TARGET_FPS;
    double last_interval = glfwGetTime() - last_time;
    double sleep_interval = 1.0/TARGET_FPS - last_interval;
    if (sleep_interval > 0) {
        double intpart;
        const struct timespec sleep_ts = { sleep_interval, 1e+9*modf(sleep_interval,&intpart) };
        nanosleep(&sleep_ts, NULL);
    }
    last_interval = glfwGetTime() - last_time;
    last_time += last_interval;
    fps = 1 / last_interval;
    return last_interval;
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    init_world();
    GLFWwindow* window = init_graphics();
    if (!window)
        exit_finalize(1);
    init_input(window);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double time = frame_sleep();
        glfwPollEvents();
        process_input(window);
        world_frame(time);
        draw();
    }

    exit_finalize(0);
}

