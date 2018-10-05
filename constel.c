#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "graphics.h"
#include "world.h"

#define TARGET_FPS 60

static void glfw_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;
    bool pressed = (action == GLFW_PRESS);

    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        if (!mods && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_UP:
    case GLFW_KEY_KP_8:
        input.up = pressed;
        break;
    case GLFW_KEY_DOWN:
    case GLFW_KEY_KP_2:
        input.down = pressed;
        break;
    case GLFW_KEY_LEFT:
    case GLFW_KEY_KP_4:
        input.left = pressed;
        break;
    case GLFW_KEY_RIGHT:
    case GLFW_KEY_KP_6:
        input.right = pressed;
        break;
    }
}

void exit_finalize(int code)
{
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
    return last_interval;
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    init_world();
    GLFWwindow* window = init_graphics();
    if (!window)
        exit_finalize(-1);
    glfwSetKeyCallback(window, glfw_key);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double time = frame_sleep();
        world_frame(time);
        draw();
        glfwPollEvents();
    }

    exit_finalize(0);
}

