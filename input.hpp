#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <GLFW/glfw3.h>

// all members must be resettable with memset(0)
extern struct input {
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
} input;

void init_input(GLFWwindow* window);
void process_input(GLFWwindow* window);
void finalize_input();

#endif // INPUT_H
