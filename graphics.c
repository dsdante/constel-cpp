#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "linmath.h"
#include "world.h"

#define SCALE 1
#define MULTISAMPLING 8

static GLFWwindow* window = NULL;
static GLuint shader_program;
static GLuint vertex_buffer = 0;
GLint shader_position;
const int vertice_count = 3;
const GLfloat vertices[] = {
    0,  0.01,
    -0.00866,  -0.005,
    0.00866,   -0.005,
};
unsigned int instance_vbo;
static mat4x4 projection;
static GLint projection_uniform;

static void show_info_log(GLuint object, PFNGLGETSHADERIVPROC gl_get_iv, PFNGLGETSHADERINFOLOGPROC gl_get_info_log)
{
    GLint log_length;
    char *log;
    gl_get_iv(object, GL_INFO_LOG_LENGTH, &log_length);
    log = malloc(log_length);
    gl_get_info_log(object, log_length, NULL, log);
    fprintf(stderr, "%s\n", log);
    free(log);
}

static void glfw_error(int error, const char* description)
{
    fprintf(stderr, "GLFW error: %s\n", description);
}

void glfw_resize(GLFWwindow* window, int width, int height)
{
//    int size = width > height ? height : width;
//    glViewport((width-size)/2, (height-size)/2, size, size);
    glViewport(0, 0, width, height);
    mat4x4_identity(projection);
    if (width > height)
        mat4x4_ortho(projection, -(float)width/height/SCALE, (float)width/height/SCALE, -1.0f/SCALE, 1.0f/SCALE, -1, 1);
    else
        mat4x4_ortho(projection, -1.0f/SCALE, 1.0f/SCALE, -(float)height/width/SCALE, (float)height/width/SCALE, -1, 1);
}

void finalize_graphics()
{
    if (vertex_buffer)
        glDeleteBuffers(1, (const GLuint[]){ vertex_buffer });
    glDeleteProgram(shader_program);
    if (window)
        glfwDestroyWindow(window);
    glfwTerminate();
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
    GLchar* buffer = malloc((*length+1)*sizeof(GLchar));
    *length = fread(buffer, 1, *length, file);
    fclose(file);
    buffer[*length] = '\0';
    return buffer;
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
        show_info_log(shader, glGetShaderiv, glGetShaderInfoLog); // @suppress("Symbol is not resolved")
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLFWwindow* init_graphics()
{
    // Init OpenGL and GLFW
    glfwSetErrorCallback(glfw_error);
    if (!glfwInit()) {
        fputs("glfwInit() failed\n", stderr);
        finalize_graphics();
        return NULL;
    }
    glfwWindowHint(GLFW_SAMPLES, MULTISAMPLING);
    if (!(window = glfwCreateWindow(1024, 1024, "Constel", NULL, NULL))) {
        fputs("glfwCreateWindow() failed\n", stderr);
        finalize_graphics();
        return NULL;
    }
    glfwSetFramebufferSizeCallback(window, glfw_resize);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - width) / 2, (mode->height - height) / 2); // center of the primary screen
    glfwGetFramebufferSize(window, &width, &height);
    glfw_resize(window, width, height);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fputs("glewInit() failed\n", stderr);
        finalize_graphics();
        return NULL;
    }

    // Init shaders
    GLuint vertex_shader = make_shader(GL_VERTEX_SHADER, "simple.vert");
    GLuint fragment_shader = make_shader(GL_FRAGMENT_SHADER, "red.frag");
    if (!vertex_shader || !fragment_shader) {
        fputs("Failed to make a shader\n", stderr);
        finalize_graphics();
        return NULL;
    }
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    //glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glDetachShader(shader_program, vertex_shader);
    glDetachShader(shader_program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    GLint program_ok;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &program_ok);
    if (!program_ok) {
        fputs("Failed to link shader program:\n", stderr);
        show_info_log(shader_program, glGetProgramiv, glGetProgramInfoLog); // @suppress("Symbol is not resolved")
        finalize_graphics();
        return NULL;
    }

    // Init data
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);
    projection_uniform = glGetUniformLocation(shader_program, "projection");

    return window;
}

void draw()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * PARTICLE_COUNT, pos, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glInterleavedArrays(GL_V2F, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
    glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, (const GLfloat*)projection);
    glUseProgram(shader_program);
    glDrawArraysInstanced(GL_TRIANGLES, 0, PARTICLE_COUNT, PARTICLE_COUNT);
    glfwSwapBuffers(window);
}
