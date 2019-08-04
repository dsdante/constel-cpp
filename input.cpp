#include <cassert>
#include <cstdlib>
#include <limits>
#include "input.hpp"

Input Input::instance;
constexpr Input& input = Input::instance;

void Input::glfw_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    assert(window == input.window && "Different window");

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
        if (pressed)
            input.f++;
        break;
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        if (pressed && !mods)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    default:
        // Do nothing.
        break;
    }
}

void Input::glfw_mouse_button(GLFWwindow* window, int button, int action, int mods)
{
    assert(window == input.window && "Different window");

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
              && std::abs(cur_mousex-clickx) <= double_click_tolerance
              && std::abs(cur_mousey-clicky) <= double_click_tolerance) {
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
    default:
        // Do nothing.
        break;
    }
}

void Input::glfw_scroll(GLFWwindow* window, double xoffset, double yoffset)
{
    assert(window == input.window && "Different window");
    input.scroll += yoffset;
}

void Input::initialize(GLFWwindow* window)
{
    assert(window != nullptr);
    this->window = window;
    glfwGetCursorPos(window, &prev_mousex, &prev_mousey);
    glfwSetKeyCallback(window, glfw_key);
    glfwSetMouseButtonCallback(window, glfw_mouse_button);
    glfwSetScrollCallback(window, glfw_scroll);
}

void Input::frame()
{
    // Lost at each main loop iteration
    panx = 0;
    pany = 0;
    scroll = 0;
    double_click = false;
    f = 0;

    glfwPollEvents();

    double mousex, mousey;
    glfwGetCursorPos(window, &mousex, &mousey);
    if (mouse_left || mouse_middle) {
        panx = static_cast<int>(mousex - prev_mousex);
        pany = static_cast<int>(mousey - prev_mousey);
    }
    prev_mousex = mousex;
    prev_mousey = mousey;
}
