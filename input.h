#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

// all members must be resettable with memset(0)
struct input {
    bool up;
    bool down;
    bool left;
    bool right;
    bool mouse_left;
    bool mouse_right;
    double scroll;
    int panx;
    int pany;
};

extern struct input input;
void init_input(GLFWwindow* window);
void process_input();
void finalize_input();

#endif // INPUT_H
