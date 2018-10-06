#include <errno.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "input.h"
#include "linmath.h"
#include "world.h"

#define MULTISAMPLING 8
#define FONT "DejaVuSansMono.ttf"
#define ZOOM_SENSITIVITY 1.2
#define SCROLL_SENSITIVITY 0.001767

double fps = 0;

static GLFWwindow* window = NULL;
static bool glfw_initialized = false;
static float zoom = 0.07;
static int win_width = 1024;
static int win_height = 1024;
static vec2 view_center = { 0, 0 };
static mat4x4 projection;
static GLint projection_uniform;

static GLuint star_shader;
static GLuint star_buffer = 0;
static unsigned int star_vbo = 0;
static const GLfloat star_vertices[] = {
    0,  0.05,
    -0.0433,  -0.025,
    0.0433,   -0.025,
};

static GLuint text_shader;
static GLint text_coord_attrib;
static GLint text_texture_uniform;
static GLint text_color_uniform;
struct point {
    GLfloat x;
    GLfloat y;
    GLfloat s;
    GLfloat t;
};
static GLuint text_vbo = 0;
FT_Library ft;
FT_Face face;

static void glfw_error(int error, const char* description)
{
    fprintf(stderr, "GLFW error: %s\n", description);
}

void update_view()
{
    zoom *= pow(ZOOM_SENSITIVITY, input.scroll);
    view_center[0] -= (float)SCROLL_SENSITIVITY * input.panx / zoom;
    view_center[1] += (float)SCROLL_SENSITIVITY * input.pany / zoom;
    mat4x4_identity(projection);
    float span = 1.0f / zoom / (win_width>win_height ? win_height : win_width);
    mat4x4_ortho(projection,
            -span*win_width + view_center[0],
            span*win_width + view_center[0],
            -span*win_height + view_center[1],
            span*win_height + view_center[1],
            -1, 1);
}

void glfw_resize(GLFWwindow* window, int width, int height)
{
    win_width = width;
    win_height = height;
    glViewport(0, 0, win_width, win_height);
    update_view();
}

void finalize_graphics()
{
    if (star_buffer != GL_INVALID_VALUE) {
        glDeleteBuffers(1, (const GLuint[]){ star_buffer });
        star_buffer = GL_INVALID_VALUE;
    }
    if (star_vbo != GL_INVALID_VALUE) {
        glDeleteBuffers(1, (const GLuint[]){ star_vbo });
        star_vbo = GL_INVALID_VALUE;
    }
    if (text_vbo != GL_INVALID_VALUE) {
        glDeleteBuffers(1, (const GLuint[]){ text_vbo });
        text_vbo = GL_INVALID_VALUE;
    }
    glDeleteProgram(star_shader);
    if (window) {
        glfwDestroyWindow(window);
        window = NULL;
    }
    if (glfw_initialized) {
        glfwTerminate();
        glfw_initialized = false;
    }
}

// Must be freed by the caller
static GLchar* read_file(const char *filename, GLint *length)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));
        *length = 0;
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    rewind(file);
    GLchar* buffer = (GLchar*)malloc((*length+1)*sizeof(GLchar));
    *length = fread(buffer, 1, *length, file);
    fclose(file);
    buffer[*length] = '\0';
    return buffer;
}

static void gl_log(GLuint object)
{
    GLint log_length = 0;
    if (glIsShader(object))
      glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    else if (glIsProgram(object))
      glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    else {
      fputs("gl_log: not a shader or a text_shader\n", stderr);
      return;
    }
    GLchar* log = (GLchar*)malloc(log_length);
    if (glIsShader(object))
      glGetShaderInfoLog(object, log_length, NULL, log);
    else if (glIsProgram(object))
      glGetProgramInfoLog(object, log_length, NULL, log);
    fprintf(stderr, "%s\n", log);
    free(log);
}

static GLuint make_shader(GLenum type, const char *filename)
{
    GLint length;
    GLchar *source = read_file(filename, &length);
    if (!source)
        return 0;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar* const*)&source, &length);
    free(source);
    glCompileShader(shader);
    GLint shader_ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
    if (!shader_ok) {
        fprintf(stderr, "Failed to compile shader '%s':\n", filename);
        gl_log(shader);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint make_program(const char *vertex_file, const char *fragment_file)
{
    GLuint program = glCreateProgram();
    if (vertex_file) {
        GLuint shader = make_shader(GL_VERTEX_SHADER, vertex_file);
        if (!shader)
            return 0;
        glAttachShader(program, shader);
    }
    if (fragment_file) {
        GLuint shader = make_shader(GL_FRAGMENT_SHADER, fragment_file);
        if(!shader)
            return 0;
        glAttachShader(program, shader);
    }
    glLinkProgram(program);
    GLint link_ok;
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
        fputs("Shader linking failed:\n", stderr);
        gl_log(program);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

GLFWwindow* init_graphics()
{
    // Init OpenGL and GLFW
    glfwSetErrorCallback(glfw_error);
    glfw_initialized = glfwInit();
    if (!glfw_initialized) {
        fputs("glfwInit failed\n", stderr);
        finalize_graphics();
        return NULL;
    }
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, MULTISAMPLING);
    if (!(window = glfwCreateWindow(win_width, win_height, "Constel", NULL, NULL))) {
        fputs("glfwCreateWindow failed\n", stderr);
        finalize_graphics();
        return NULL;
    }
    glfwSetFramebufferSizeCallback(window, glfw_resize);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - win_width) / 2, (mode->height - win_height) / 2); // center of the primary screen
    glfwGetFramebufferSize(window, &win_width, &win_height);
    glfw_resize(window, win_width, win_height);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fputs("glewInit() failed\n", stderr);
        finalize_graphics();
        return NULL;
    }

    // Init stars
    glGenBuffers(1, &star_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, star_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(star_vertices), star_vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &star_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, star_vbo);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);
    star_shader = make_program("star.vert", "star.frag");
    if (!star_shader) {
        finalize_graphics();
        return NULL;
    }
    projection_uniform = glGetUniformLocation(star_shader, "projection");

    // Init text
    glGenBuffers(1, &text_vbo);
    if (FT_Init_FreeType(&ft)) {
        fputs("FT_Init_FreeType failed\n", stderr);
        finalize_graphics();
        return NULL;
    }
    if (FT_New_Face(ft, FONT, 0, &face)) {
        fputs("Cannot open font " FONT "\n", stderr);
        finalize_graphics();
        return NULL;
    }
    text_shader = make_program("text.vert", "text.frag");
    if(text_shader == 0) {
        finalize_graphics();
        return NULL;
    }
    text_coord_attrib = glGetAttribLocation(text_shader, "coord");
    text_texture_uniform = glGetUniformLocation(text_shader, "tex");
    text_color_uniform = glGetUniformLocation(text_shader, "color");
    if(text_coord_attrib == -1 || text_texture_uniform == -1 || text_color_uniform == -1) {
        finalize_graphics();
        return NULL;
    }

    return window;
}

void draw_text(const char *text, float x, float y, float sx, float sy)
{
    const char *p;
    FT_GlyphSlot g = face->glyph;

    /* Create a texture that will be used to hold one "glyph" */
    GLuint tex;

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(text_texture_uniform, 0);

    /* We require 1 byte alignment when uploading texture data */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* Clamping to edges is important to prevent artifacts when scaling */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* Linear filtering usually looks best for text */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* Set up the VBO for our vertex data */
    glEnableVertexAttribArray(text_coord_attrib);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glVertexAttribPointer(text_coord_attrib, 4, GL_FLOAT, GL_FALSE, 0, 0);

    /* Loop through all characters */
    for (p = text; *p; p++) {
        /* Try to load and render the character */
        if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
            continue;

        /* Upload the "bitmap", which contains an 8-bit grayscale image, as an alpha texture */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, g->bitmap.width, g->bitmap.rows, 0, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);

        /* Calculate the vertex and texture coordinates */
        float x2 = x + g->bitmap_left * sx;
        float y2 = -y - g->bitmap_top * sy;
        float w = g->bitmap.width * sx;
        float h = g->bitmap.rows * sy;

        struct point box[4] = {
            {x2, -y2, 0, 0},
            {x2 + w, -y2, 1, 0},
            {x2, -y2 - h, 0, 1},
            {x2 + w, -y2 - h, 1, 1},
        };

        /* Draw the character on the screen */
        glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        /* Advance the cursor to the start of the next character */
        x += (g->advance.x >> 6) * sx;
        y += (g->advance.y >> 6) * sy;
    }

    glDisableVertexAttribArray(text_coord_attrib);
    glDeleteTextures(1, &tex);
}

void draw()
{
    if (input.scroll || input.panx || input.pany)
        update_view();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);

    glBlendFunc(GL_ONE, GL_ONE);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * PARTICLE_COUNT, pos_display, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, star_buffer);
    glInterleavedArrays(GL_V2F, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, star_vbo);
    glUseProgram(star_shader);
    glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, (const GLfloat*)projection);
    glDrawArraysInstanced(GL_TRIANGLES, 0, PARTICLE_COUNT, PARTICLE_COUNT);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(text_shader);
    GLfloat transparent_green[4] = { 0, 1, 0, 0.7 };
    FT_Set_Pixel_Sizes(face, 0, 48);
    char fps_str[10];
    sprintf(fps_str, "%.1f FPS", fps);
    glUniform4fv(text_color_uniform, 1, transparent_green);
    draw_text(fps_str, -0.9, 0.8, 2.0/win_width, 2.0/win_height);

    glDisable(GL_BLEND);
    glfwSwapBuffers(window);
}
