#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include "common.h"
#include "linmath.h"
#include "linmathd.h"
#include "world.h"

// Calculate stars coordinates through Barnes–Hut simulation.
// All stars are spread in a square quad-tree

// star or quadrant
struct node
{
    double x; // center of mass
    double y;
    double mass;
    double size; // zero for star
};

struct star
{
    struct node; // inheriting node
    double speed_x;
    double speed_y;
} *stars;

struct quad
{
    struct node; // inheriting node
    double geom_x; // geometrical center
    double geom_y;
    double mass_offset;  // distance betwen mass and geometrical center
    struct quad* children[4];
} *quads;


void finalize_world()
{
    free(stars);
    free(quads);
    free(disp_stars);
}

static inline double frand(double min, double max)
{
    return (double)rand()/RAND_MAX * (max-min) + min;
}

// assists qsorting
int mass_ascending(const void *a, const void *b)
{
    if (((struct star*)a)->mass < ((struct star*)b)->mass) return -1;
    if (((struct star*)a)->mass > ((struct star*)b)->mass) return 1;
    return 0;
}

void init_world()
{
    assert(config.stars > 1);
    stars = calloc(config.stars, sizeof(struct star));
    quads = calloc(2 * config.stars, sizeof(struct quad));  // TODO: dynamic reallocation
    disp_stars = malloc(config.stars * sizeof(vec2));

    double rmax = sqrt(config.stars)/7;
    for (int i = 0; i < config.stars; i++) {
        double r = frand(0, rmax);
        double dir = frand(0, 2*M_PI);
        stars[i].x = r * cos(dir);
        stars[i].y = r * sin(dir);
        stars[i].speed_x =  config.star_speed * pow(r, 0.25) * sin(dir);
        stars[i].speed_y = -config.star_speed * pow(r, 0.25) * cos(dir);
        stars[i].mass = frand(0.5, 1.5);
    }
    qsort(stars, config.stars, sizeof(struct star), mass_ascending);  // increases accumulation accuracy
    /*
    config.stars = 3;
    stars[0].x = 0.1;
    stars[0].y = 0;
    stars[0].speed_x = 0;
    stars[0].speed_y = -0.1;
    stars[0].mass = 1;
    stars[1].x = -0.1;
    stars[1].y = 0;
    stars[1].speed_x = 0;
    stars[1].speed_y = 0.1;
    stars[1].mass = 1;
    stars[2].x = 1000;
    stars[2].y = 1000;
    */
}

void update_speed(struct star* star, struct quad* node, double time)
{
    double dx = node->x - star->x;
    double dy = node->y - star->y;
    double distance_sqr = dx*dx + dy*dy;
    if (sqrt(distance_sqr) >= node->size * config.accuracy) {
        double angle = atan2(dy, dx);
        double accel = time * config.gravity * node->mass / (distance_sqr + config.epsilon);
        star->speed_x += accel * cos(angle);
        star->speed_y += accel * sin(angle);
    } else {
        for (int i = 0; i < 4; i++)
            if (node->children[i] != NULL && node->children[i] != (struct quad*)star)
                update_speed(star, node->children[i], time);
    }
}

static inline int get_quadrant(const struct quad *quad, const struct star *star)
{
    int quadrant = 0;
    if (star->x > quad->geom_x)
        quadrant++;
    if (star->y > quad->geom_y)
        quadrant += 2;
    return quadrant;
}

void world_frame(double time)
{
    if (time > 1/config.min_fps)
        time = 1/config.min_fps;
    time *= config.speed;


    //************************
    // Build Barnes-Hut qtree
    //************************

    perf_build = glfwGetTime();
    // Root node
    double xmin_world = INFINITY;
    double ymin_world = INFINITY;
    double xmax_world = -INFINITY;
    double ymax_world = -INFINITY;
    for (int i = 0; i < config.stars; i++) {
        if (xmin_world > stars[i].x)
            xmin_world = stars[i].x;
        if (xmax_world < stars[i].x)
            xmax_world = stars[i].x;
        if (ymin_world > stars[i].y)
            ymin_world = stars[i].y;
        if (ymax_world < stars[i].y)
            ymax_world = stars[i].y;
    }
    quads[0].geom_x = (xmin_world+xmax_world)/2;
    quads[0].geom_y = (ymin_world+ymax_world)/2;
    double size_x = xmax_world - xmin_world;
    double size_y = ymax_world - ymin_world;
    quads[0].size = size_x > size_y ? size_x : size_y; // keep nodes square
    size_t quad_count = 1; // number of quads

    // Build the tree
    for (struct star* star = stars; star < stars + config.stars; star++) {
        struct quad* quad = &quads[0];
        do {
            // Add star to current quad
            double mass_sum = quad->mass + star->mass;
            quad->x = (quad->x * quad->mass + star->x * star->mass) / mass_sum;
            quad->y = (quad->y * quad->mass + star->y * star->mass) / mass_sum;
            quad->mass = mass_sum;
            int quadrant = get_quadrant(quad, star);
            if (quad->children[quadrant] == NULL) {
                quad->children[quadrant] = (struct quad*)star;
            } else if (quad->children[quadrant]->size == 0) {
                struct star* old_star = (struct star*)(quad->children[quadrant]);
                struct quad* new_quad = &quads[quad_count];
                quad_count++;
                new_quad->x = old_star->x;
                new_quad->y = old_star->y;
                new_quad->mass = old_star->mass;
                new_quad->size = quad->size/2;
                double shift = quad->size/4;
                new_quad->geom_x = quad->geom_x + (quadrant&0x1 ? shift : -shift);
                new_quad->geom_y = quad->geom_y + (quadrant&0x2 ? shift : -shift);
                double dx = new_quad->x - new_quad->geom_x;
                double dy = new_quad->y - new_quad->geom_y;
                new_quad->mass_offset = sqrt(dx*dx + dy*dy);
                new_quad->children[get_quadrant(new_quad, old_star)] = (struct quad*)old_star;
                quad->children[quadrant] = new_quad;
            }
            quad = quad->children[quadrant];
        } while (quad->size);
    }
    perf_build = glfwGetTime() - perf_build;


    //*************************************
    // Calculate acceleration and position
    //*************************************

    perf_accel = glfwGetTime();
    for (struct star* star = stars; star < stars + config.stars; star++)
        update_speed(star, &quads[0], time);
    for (struct star* star = stars; star < stars + config.stars; star++) {
        star->x += star->speed_x * time;
        star->y += star->speed_y * time;
    }
    perf_accel = glfwGetTime() - perf_accel;

    for (int i = 0; i < config.stars; i++) {
        disp_stars[i][0] = stars[i].x;
        disp_stars[i][1] = stars[i].y;
    }
    memset(quads, 0, quad_count * sizeof(struct quad));
}
