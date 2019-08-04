#ifndef INPUT_H
#define INPUT_H

#include <memory>
#include <GLFW/glfw3.h>

class Input
{
private:
    static constexpr double double_click_interval = 0.5;  // maximum double click interval in seconds
    static const int double_click_tolerance = 4;  // maximum double click mouse slip in pixels

    // GLFW callbacks
    static void glfw_key(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_mouse_button(GLFWwindow* window, int button, int action, int mods);
    static void glfw_scroll(GLFWwindow* window, double xoffset, double yoffset);

    Input() = default;

    GLFWwindow* window;
    double prev_mousex;
    double prev_mousey;

public:
    static Input instance;

    void initialize(GLFWwindow* window);
    void frame();

    // All members are zero-initialized
    bool up;
    bool down;
    bool left;
    bool right;
    bool mouse_left;
    bool mouse_middle;
    bool mouse_right;
    bool double_click;
    int f;
    double scroll;
    int panx;
    int pany;
};

extern Input& input;

#endif  // INPUT_H
