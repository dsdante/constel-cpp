#define _GNU_SOURCE
#include <errno.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "common.h"
#include "input.h"
#include "linmath.h"

#define ZOOM_SENSITIVITY 1.2

static int win_width = 1024; // actual size of the client area
static int win_height = 1024;


// ============================== Text rendering ==============================

static GLuint text_shader = GL_INVALID_VALUE;
static GLint text_char_pos_attrib;
static GLint text_texture_uniform;
static GLint text_color_uniform;
static GLint text_projection_uniform;
static GLint text_pos_uniform;
static mat4x4 text_projection;
static GLuint text_vbo = GL_INVALID_VALUE;
static FT_Library ft;
static char* text_buff = NULL;
static size_t text_buff_length = 1024;

enum align
{
    align_right = 0x1,
    align_bottom = 0x2,
    align_top_left = 0x0,
    align_top_right = align_right,
    align_bottom_left = align_bottom,
    align_bottom_right = align_right | align_bottom,
};

struct font_point
{
    GLfloat x;
    GLfloat y;
    GLfloat s;
    GLfloat t;
};

static struct font
{
    int size;   // font size
    int height; // distance between baselines
    GLuint tex; // texture object
    int tex_w;  // width of texture in pixels
    int tex_h;  // height of texture in pixels
    struct c
    {
        int dx; // advance.x
        int dy; // advance.y
        int w;  // bitmap.width;
        int h;  // bitmap.height;
        int x;  // bitmap_left;
        int y;  // bitmap_top;
        int tx; // x offset of the glyph in the texture
        int ty; // y offset of the glyph in the texture
    } c[128];   // characters
} font;

// Inspired by https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_02
static void new_font(const char* font_path, int size, struct font* font)
{
    font->tex = GL_INVALID_VALUE;
    const int texture_max_width = 1024;
    FT_Face face;
    if (FT_New_Face(ft, font_path, 0, &face)) {
        fprintf(stderr, "Cannot open font '%s'\n", font_path);
        return;
    }

    font->size = size;
    font->height = size * face->height / face->units_per_EM;
    FT_Set_Pixel_Sizes(face, 0, size);
    FT_GlyphSlot g = face->glyph;
    int roww = 0;
    int rowh = 0;
    font->tex_w = 0;
    font->tex_h = 0;
    memset(font->c, 0, sizeof(font->c));
    for (int i = 32; i < 128; i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "Font '%s': cannot load character '%c'\n", font_path, i);
            continue;
        }
        if (roww + g->bitmap.width + 1 >= texture_max_width) {
            if (roww > font->tex_w)
                font->tex_w = roww;
            font->tex_h += rowh;
            roww = 0;
            rowh = 0;
        }
        roww += g->bitmap.width + 1;
        if (g->bitmap.rows > rowh)
            rowh = g->bitmap.rows;
    }
    if (roww > font->tex_w)
        font->tex_w = roww;
    font->tex_h += rowh;

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &font->tex);
    glBindTexture(GL_TEXTURE_2D, font->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, font->tex_w, font->tex_h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int ox = 0;
    int oy = 0;
    rowh = 0;
    for (int i = 32; i < 128; i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "Font '%s': cannot load character '%c'\n", font_path, i);
            continue;
        }
        if (ox + g->bitmap.width + 1 >= texture_max_width) {
            oy += rowh;
            rowh = 0;
            ox = 0;
        }
        glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, g->bitmap.width, g->bitmap.rows,
                GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);
        font->c[i].dx = g->advance.x >> 6;
        font->c[i].dy = g->advance.y >> 6;
        font->c[i].w = g->bitmap.width;
        font->c[i].h = g->bitmap.rows;
        font->c[i].x = g->bitmap_left;
        font->c[i].y = g->bitmap_top;
        font->c[i].tx = ox;
        font->c[i].ty = oy;
        if (g->bitmap.rows > rowh)
            rowh = g->bitmap.rows;
        ox += g->bitmap.width + 1;
    }

    FT_Done_Face(face);
}

static void draw_text(struct font* font, int x, int y, enum align align, const char* restrict format, ...)
{
    int length;
    va_list argptr;
    do {
        va_start(argptr, format);
        length = vsnprintf(text_buff, text_buff_length, format, argptr);
        va_end(argptr);
        if (length >= text_buff_length) {
            text_buff_length = 2 * length;
            text_buff = realloc(text_buff, text_buff_length);
            continue;
        }
    } while (false);
    if (length < 0)
        return;

    vec2 text_pos = { x, win_height - y - font->height };
    y = 0;
    struct font_point coords[6*length];
    struct font_point* coord = coords;
    unsigned char *p = (unsigned char*)text_buff;
    while (*p) {
        int line_length = 0;
        x = 0;

        while (*p != '\0' && *p != '\n') {
            struct c* c = &font->c[*p];
            if (c->w && c->h) {
                int left   = x + c->x;
                int right  = x + c->x + c->w;
                int top    = y + c->y - c->h;
                int bottom = y + c->y;
                float tex_left = (float)c->tx / font->tex_w;
                float tex_right = (float)(c->tx + c->w) / font->tex_w;
                float tex_top = (float)(c->ty + c->h) / font->tex_h;
                float tex_bottom = (float)c->ty / font->tex_h;
                *(coord++) = (struct font_point){ left,  bottom, tex_left,  tex_bottom };
                *(coord++) = (struct font_point){ right, bottom, tex_right, tex_bottom };
                *(coord++) = (struct font_point){ left,  top,    tex_left,  tex_top };
                *(coord++) = (struct font_point){ right, bottom, tex_right, tex_bottom };
                *(coord++) = (struct font_point){ left,  top,    tex_left,  tex_top };
                *(coord++) = (struct font_point){ right, top,    tex_right, tex_top };
                line_length++;
            }
            x += c->dx;
            p++;
        }

        if (align & align_right)
            for (int i = 1; i <= 6*line_length; i++)
                (coord-i)->x -= x;
        y -= font->height;

        if (*p == '\n')
            p++;
    }

    if (align & align_bottom)
        text_pos[1] -= y;  // y is negative
    int n = coord - coords;  // total number of printable characters

    glBindTexture(GL_TEXTURE_2D, font->tex);
    glUseProgram(text_shader);
    glUniform1i(text_texture_uniform, 0);
    glUniform2fv(text_pos_uniform, 1, text_pos);
    glEnableVertexAttribArray(text_char_pos_attrib);
    glVertexAttribPointer(text_char_pos_attrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, n);
    glDisableVertexAttribArray(text_char_pos_attrib);
}


// ============================= Window functions =============================

static bool glfw_initialized = false;
static GLFWwindow* window = NULL;
static bool fullscreen = false;
static bool maximized = true;
static int restored_x;
static int restored_y;
static int restored_width = 1024;
static int restored_height = 1024;
static bool need_update_view = true;

// Dealing with window state changes
static void update_window()
{
    bool toggle_fullscreen = input.double_click || input.f % 2;
    bool toggle_maximize = (maximized != glfwGetWindowAttrib(window, GLFW_MAXIMIZED));

    if (toggle_fullscreen) {
        fullscreen = !fullscreen;
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (fullscreen) {
            if (!maximized) {
                glfwGetWindowPos(window, &restored_x, &restored_y);
                glfwGetWindowSize(window, &restored_width, &restored_height);
            }
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else {
            glfwSetWindowMonitor(window, NULL, restored_x, restored_y, restored_width, restored_height, mode->refreshRate);
            if (maximized)
                glfwMaximizeWindow(window);
        }
    }
    if (toggle_maximize && !fullscreen) {
        maximized = !maximized;
        if (!maximized) {
            glfwSetWindowPos(window, restored_x, restored_y);
            glfwSetWindowSize(window, restored_width, restored_height);
        }
    }
    // If just moving around, track resotred size/position
    if (!fullscreen && !maximized && !toggle_maximize) {
        glfwGetWindowPos(window, &restored_x, &restored_y);
        glfwGetWindowSize(window, &restored_width, &restored_height);
    }
    if (toggle_fullscreen || toggle_maximize)
        need_update_view = true;
}

static void glfw_error(int error, const char* description)
{
    fprintf(stderr, "GLFW error: %s\n", description);
}

static void glfw_move(GLFWwindow* window, int xpos, int ypos)
{
    update_window();
}

static void glfw_resize(GLFWwindow* window, int width, int height)
{
    win_width = width;
    win_height = height;
    update_window();
    need_update_view = true;
}


// ============================= General graphics =============================

static vec2 view_center = { 0, 0 };
static float zoom;

static GLuint star_texture = GL_INVALID_VALUE;
static float* star_texture_values = NULL;
static int star_texture_buff_size = 0;

static GLuint star_shader = GL_INVALID_VALUE;
static mat4x4 projection;
static GLint star_projection_uniform = GL_INVALID_VALUE;
static GLint star_texture_uniform = GL_INVALID_VALUE;
static GLint star_position_attribute = GL_INVALID_VALUE;
static GLint star_color_attribute = GL_INVALID_VALUE;
static GLuint star_position_vbo = GL_INVALID_VALUE;
static GLuint star_color_vbo = GL_INVALID_VALUE;


// Log the last error associated with the object
static void gl_log(GLuint object)
{
    GLint log_length = 0;
    if (glIsShader(object)) {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else if (glIsProgram(object)) {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else {
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

static GLuint make_shader_program(const char *vertex_file, const char *fragment_file)
{
    GLuint program = glCreateProgram();
    if (vertex_file) {
        GLuint shader = make_shader(GL_VERTEX_SHADER, vertex_file);
        if (!shader)
            return GL_INVALID_VALUE;
        glAttachShader(program, shader);
        glDeleteShader(shader);  // mark for deletion
    }
    if (fragment_file) {
        GLuint shader = make_shader(GL_FRAGMENT_SHADER, fragment_file);
        if(!shader)
            return GL_INVALID_VALUE;
        glAttachShader(program, shader);
        glDeleteShader(shader);
    }
    glLinkProgram(program);
    GLint link_ok;
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
        fputs("Shader linking failed:\n", stderr);
        gl_log(program);
        glDeleteProgram(program);
        return GL_INVALID_VALUE;
    }
    return program;
}

// Client area needs recalculation
static void update_view()
{
    need_update_view = false;
    glViewport(0, 0, win_width, win_height);

    view_center[0] -= input.panx / zoom;
    view_center[1] += input.pany / zoom;

    // Update zoom
    if (input.scroll) {
        float new_zoom = zoom * pow(ZOOM_SENSITIVITY, input.scroll);
        struct vecd2 mouse; // mouse position in pixels
        glfwGetCursorPos(window, &mouse.x, &mouse.y);
        mouse.x = mouse.x - 0.5*win_width;
        mouse.y = 0.5*win_height - mouse.y;
        // Preserve the world coordinate under mouse when zooming
        view_center[0] += (float)mouse.x * (1/zoom - 1/new_zoom);
        view_center[1] += (float)mouse.y * (1/zoom - 1/new_zoom);
        zoom = new_zoom;
    }

    // Re-generate the star sprite
    glUseProgram(star_shader);
    if ((input.scroll || !star_texture_values) && zoom < 1000) {
        const float star_size = 0.5;  // equals to star.vert::star_size
        int star_texture_size = 2.0f * star_size * zoom;
        if (star_texture_buff_size < star_texture_size * star_texture_size) {
            star_texture_buff_size = star_texture_size * star_texture_size;
            star_texture_values = realloc(star_texture_values, star_texture_buff_size * sizeof(float));
        }

        for (int x = 0; x < (star_texture_size+1)/2; x++)
        for (int y = 0; y <= x; y++) {
            float dx = (0.5f * star_texture_size - x) / zoom / star_size * 20 ;
            float dy = (0.5f * star_texture_size - y) / zoom / star_size * 20 ;
            float alpha = 1.0f / (dx*dx + dy*dy);
            int x2 = star_texture_size-1-x;
            int y2 = star_texture_size-1-y;
            // exploit symmetry
            star_texture_values[x  + y  * star_texture_size] = alpha;
            star_texture_values[x  + y2 * star_texture_size] = alpha;
            star_texture_values[x2 + y  * star_texture_size] = alpha;
            star_texture_values[x2 + y2 * star_texture_size] = alpha;
            star_texture_values[y  + x  * star_texture_size] = alpha;
            star_texture_values[y  + x2 * star_texture_size] = alpha;
            star_texture_values[y2 + x  * star_texture_size] = alpha;
            star_texture_values[y2 + x2 * star_texture_size] = alpha;
        }
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, star_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, star_texture_size, star_texture_size, 0, GL_ALPHA, GL_FLOAT, star_texture_values);
    }

    mat4x4_identity(projection);
    mat4x4_ortho(projection,
        -0.5f*win_width/zoom + view_center[0],
         0.5f*win_width/zoom + view_center[0],
        -0.5f*win_height/zoom + view_center[1],
         0.5f*win_height/zoom + view_center[1],
        -1, 1);
    glUniformMatrix4fv(star_projection_uniform, 1, GL_FALSE, (const GLfloat*)projection);

    mat4x4_identity(text_projection);
    mat4x4_ortho(text_projection, 0, win_width, 0, win_height, -1, 1);
    glUseProgram(text_shader);
    glUniformMatrix4fv(text_projection_uniform, 1, GL_FALSE, (const GLfloat*)text_projection);
}

void finalize_graphics()
{
    if (star_shader != GL_INVALID_VALUE) {
        glDeleteProgram(star_shader);
        star_shader = GL_INVALID_VALUE;
    }
    if (text_shader != GL_INVALID_VALUE) {
        glDeleteProgram(text_shader);
        text_shader = GL_INVALID_VALUE;
    }
    if (star_position_vbo != GL_INVALID_VALUE) {
        glDeleteBuffers(1, (const GLuint[]){ star_position_vbo });
        star_position_vbo = GL_INVALID_VALUE;
    }
    if (star_color_vbo != GL_INVALID_VALUE) {
        glDeleteBuffers(1, (const GLuint[]){ star_color_vbo });
        star_color_vbo = GL_INVALID_VALUE;
    }
    if (text_vbo != GL_INVALID_VALUE) {
        glDeleteBuffers(1, (const GLuint[]){ text_vbo });
        text_vbo = GL_INVALID_VALUE;
    }
    if (star_texture != GL_INVALID_VALUE) {
        glDeleteTextures(1, &star_texture);
        star_texture = GL_INVALID_VALUE;
    }
    if (star_texture_values) {
        free(star_texture_values);
        star_texture_values = NULL;
    }
    if (font.tex != GL_INVALID_VALUE) {
        glDeleteTextures(1, &font.tex);
        font.tex = GL_INVALID_VALUE;
    }
    if (text_buff) {
        free(text_buff);
        text_buff = NULL;
    }
    if (window) {
        glfwDestroyWindow(window);
        window = NULL;
    }
    if (glfw_initialized) {
        glfwTerminate();
        glfw_initialized = false;
    }
}

GLFWwindow* init_graphics()
{
    // Init OpenGL and GLFW
    glfwSetErrorCallback(glfw_error);
    glfw_initialized = glfwInit();
    if (!glfw_initialized) {
        finalize_graphics();
        return NULL;
    }
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, config.msaa);
    window = glfwCreateWindow(win_width, win_height, "Constel", NULL, NULL);
    if (!window) {
        finalize_graphics();
        return NULL;
    }
    glfwSetWindowPosCallback(window, glfw_move);
    glfwSetFramebufferSizeCallback(window, glfw_resize);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    restored_x = (mode->width - restored_width) / 2;
    restored_y = (mode->height - restored_height) / 2;
    zoom = config.default_zoom;
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fputs("glewInit failed\n", stderr);
        finalize_graphics();
        return NULL;
    }
    glEnable(GL_BLEND);


    // Init stars
    star_shader = make_shader_program("star.vert", "star.frag");
    if (!star_shader) {
        finalize_graphics();
        return NULL;
    }
    star_projection_uniform = glGetUniformLocation(star_shader, "projection");
    star_texture_uniform = glGetUniformLocation(star_shader, "texture");
    star_position_attribute = glGetAttribLocation(star_shader, "star_position");
    star_color_attribute = glGetAttribLocation(star_shader, "star_color");

    glGenBuffers(1, &star_position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, star_position_vbo);
    glVertexAttribDivisor(star_position_attribute, 1);

    glGenBuffers(1, &star_color_vbo);
    glVertexAttribDivisor(star_color_attribute, 1);
    glVertexAttribPointer(star_color_attribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * config.stars, disp_star_color, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, star_color_vbo);
    glEnableVertexAttribArray(star_color_attribute);

    glGenTextures(1, &star_texture);
    glBindTexture(GL_TEXTURE_2D, star_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glUseProgram(star_shader);
    glUniform1i(star_texture_uniform, 1);


    // Init text
    if (config.show_status) {
        text_buff = malloc(text_buff_length * sizeof(*text_buff));
        glGenBuffers(1, &text_vbo);
        if (FT_Init_FreeType(&ft)) {
            fputs("FT_Init_FreeType failed\n", stderr);
            finalize_graphics();
            return NULL;
        }
        text_shader = make_shader_program("text.vert", "text.frag");
        if(text_shader == 0) {
            finalize_graphics();
            return NULL;
        }
        text_projection_uniform = glGetUniformLocation(text_shader, "projection");
        text_pos_uniform = glGetUniformLocation(text_shader, "text_pos");
        text_char_pos_attrib = glGetAttribLocation(text_shader, "char_pos");
        text_texture_uniform = glGetUniformLocation(text_shader, "texture");
        text_color_uniform = glGetUniformLocation(text_shader, "color");
        new_font(config.font, config.text_size, &font);
        if (font.tex == GL_INVALID_VALUE) {
            finalize_graphics();
            return NULL;
        }
    }

    return window;
}

void draw()
{
    double perf_draw_start = glfwGetTime();
    // Update window and client area state
    if (input.double_click || input.f % 2 || (maximized != glfwGetWindowAttrib(window, GLFW_MAXIMIZED)))
        update_window();
    if (need_update_view || input.scroll || input.panx || input.pany)
        update_view();
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw stars
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glUseProgram(star_shader);
    glEnableVertexAttribArray(star_position_attribute);
    glVertexAttribPointer(star_position_attribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * config.stars, disp_star_position, GL_STREAM_DRAW);
    glBindTexture(GL_TEXTURE_2D, star_texture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, config.stars);

    // Draw text
    if (config.show_status) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(text_shader);
        glUniform4fv(text_color_uniform, 1, *config.text_color);

        char zoom_text[64];
        if (zoom >= 2 * config.default_zoom)
            snprintf(zoom_text, sizeof(zoom_text), "%.0fx", zoom/config.default_zoom);
        else
            snprintf(zoom_text, sizeof(zoom_text), "1:%.0f", (float)config.default_zoom/zoom);
        draw_text(&font, win_width - font.c[' '].dx, font.c[' '].dx/2, align_top_right,
                "X: %.2f  Y: %.2f\n"
                "Zoom: %s\n"
                "%.0f FPS",
                view_center[0], view_center[1],
                zoom_text,
                get_fps_period(1)+0.5f);
    }

    glfwSwapBuffers(window);
    perf_draw = glfwGetTime() - perf_draw_start;
}
