#include <assert.h>
#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "input.h"

struct input input = { 0 };

static double scroll = 0;
static double mousex = 0;
static double mousey = 0;

static void glfw_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;
    bool pressed = (action == GLFW_PRESS);

    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (pressed && !mods)
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

static void glfw_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
    bool pressed = (action == GLFW_PRESS);
    switch(button) {
    case GLFW_MOUSE_BUTTON_LEFT:
        input.mouse_left = pressed;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        input.mouse_right = pressed;
        break;
    }
}

static void glfw_scroll(GLFWwindow *window, double xoffset, double yoffset)
{
    scroll += yoffset;
}

void init_input(GLFWwindow* window)
{
    assert(window != NULL);
    glfwSetKeyCallback(window, glfw_key);
    glfwSetMouseButtonCallback(window, glfw_mouse_button);
    glfwSetScrollCallback(window, glfw_scroll);
}

void process_input(GLFWwindow *window)
{
    double cur_mousex, cur_mousey;
    glfwGetCursorPos(window, &cur_mousex, &cur_mousey);
    input.scroll = scroll;
    scroll = 0;
    if (input.mouse_left) {
        input.panx = cur_mousex - mousex;
        input.pany = cur_mousey - mousey;
    } else {
        input.panx = 0;
        input.pany = 0;
    }
    mousex = cur_mousex;
    mousey = cur_mousey;
}

void finalize_input()
{
}
