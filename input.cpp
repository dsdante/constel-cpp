#include <cassert>
#include <cstdlib>
#include <limits>
#include "input.hpp"

const double double_click_interval = 0.5;
const int double_click_tolerance = 4;

struct input input = { 0 };

static double prev_mousex;
static double prev_mousey;

static void glfw_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;
    bool pressed = (action == GLFW_PRESS);

    switch (key) {
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
    case GLFW_KEY_F:
        input.f += pressed;
        break;
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        if (pressed && !mods)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

static void glfw_mouse_button(GLFWwindow* window, int button, int action, int mods)
{
    static double double_click_start = -std::numeric_limits<double>::infinity();
    static double clickx = 0;
    static double clicky = 0;

    bool pressed = (action == GLFW_PRESS);
    double cur_mousex, cur_mousey;
    glfwGetCursorPos(window, &cur_mousex, &cur_mousey);
    switch(button) {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        input.mouse_left = pressed;
        if (!pressed)
            break;
        double time = glfwGetTime();
        if (time - double_click_start <= double_click_interval
              && abs(cur_mousex-clickx) <= double_click_tolerance
              && abs(cur_mousey-clicky) <= double_click_tolerance) {
            input.double_click = true;
            input.mouse_left = false;
            double_click_start = -std::numeric_limits<double>::infinity();
        } else {
            double_click_start = time;
            glfwGetCursorPos(window, &clickx, &clicky);
        }
        break;
    }
    case GLFW_MOUSE_BUTTON_MIDDLE:
        input.mouse_middle = pressed;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        input.mouse_right = pressed;
        break;
    }
}

static void glfw_scroll(GLFWwindow* window, double xoffset, double yoffset)
{
    input.scroll += yoffset;
}

void init_input(GLFWwindow* window)
{
    assert(window != NULL);
    glfwSetKeyCallback(window, glfw_key);
    glfwSetMouseButtonCallback(window, glfw_mouse_button);
    glfwSetScrollCallback(window, glfw_scroll);
    glfwGetCursorPos(window, &prev_mousex, &prev_mousey);
}

void process_input(GLFWwindow* window)
{
    // Reset all toggles
    input.panx = 0;
    input.pany = 0;
    input.scroll = 0;
    input.double_click = false;
    input.f = false;

    glfwPollEvents();

    double mousex, mousey;
    glfwGetCursorPos(window, &mousex, &mousey);
    if (input.mouse_left || input.mouse_middle) {
        input.panx = mousex - prev_mousex;
        input.pany = mousey - prev_mousey;
    }
    prev_mousex = mousex;
    prev_mousey = mousey;
}

void finalize_input()
{
}
