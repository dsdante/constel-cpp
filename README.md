# Constel++
My ongoing C++ rewriting of own [galaxy model](https://github.com/dsdante/constel)

Stars in a [Barnes–Hut quad-tree](https://en.wikipedia.org/wiki/Barnes%E2%80%93Hut_simulation) are processed in parallel using the [velocity Verlet method](https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet), then drawn as OpenGL particles.


### Requirements
Arch Linux:  
\# pacman -S glew glfw-x11 freetype2

Debian/Ubuntu (presumed):  
\# apt-get install libglew-dev libglfw3-dev libfreetype6-dev


### Control
Mouse dragging: pan  
Mouse wheel: zoom  
F, double click: fullscreen  
Physical and visual options can be set in constel.conf.


### To do
 * Cross-platform code (GCC and MSVC) and multithreading (Linux and Windows)
 * Get rid of linmath.h
 * Reduce entropy
 * Collisions and merging
 * Mean field method
 * Fancy effects