#ifndef GRAPHICS_H
#define GRAPHICS_H

GLFWwindow* init_graphics();
void draw();
void finalize_graphics();
void update_view(int xoffset, int yoffset, double scroll);

#endif // GRAPHICS_H
