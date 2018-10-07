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
#include "input.h"
#include "linmath.h"
#include "world.h"

#define MULTISAMPLING 8
#define FONT "/usr/share/fonts/TTF/DejaVuSansMono.ttf"
#define ZOOM_SENSITIVITY 1.2
#define SCROLL_SENSITIVITY 0.01767

double fps = 0;
static GLFWwindow* window = NULL;
static bool glfw_initialized = false;
static int win_width = 1024;
static int win_height = 1024;
static bool need_update = true;

static vec2 view_center = { 0, 0 };
static float zoom = 1;
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
static GLint text_projection_uniform;
static mat4x4 text_projection;
static GLuint text_vbo = 0;
static FT_Library ft;

struct point {
    GLfloat x;
    GLfloat y;
    GLfloat s;
    GLfloat t;
};

struct font
{
    int size;
    GLuint tex;     // texture object
    unsigned int w; // width of texture in pixels
    unsigned int h; // height of texture in pixels
    struct
    {
        float ax;   // advance.x
        float ay;   // advance.y
        float bw;   // bitmap.width;
        float bh;   // bitmap.height;
        float bl;   // bitmap_left;
        float bt;   // bitmap_top;
        float tx;   // x offset of glyph in texture coordinates
        float ty;   // y offset of glyph in texture coordinates
    } c[128];       // character information
} *status_font = NULL;

static struct font* new_font(const char* font_path, int size)
{
    const int texture_max_width = 1024;
    FT_Face face;
    if (FT_New_Face(ft, FONT, 0, &face)) {
        fputs("Cannot open font '" FONT "'\n", stderr);
        return NULL;
    }

    struct font *font = (struct font*)malloc(sizeof(struct font));
    font->size = size;
    FT_Set_Pixel_Sizes(face, 0, size);
    FT_GlyphSlot g = face->glyph;
    unsigned int roww = 0;
    unsigned int rowh = 0;
    font->w = 0;
    font->h = 0;
    memset(font->c, 0, sizeof(font->c));
    for (int i = 32; i < 128; i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "Font '%s': cannot load character '%c'\n", font_path, i);
            continue;
        }
        if (roww + g->bitmap.width + 1 >= texture_max_width) {
            if (roww > font->w)
                font->w = roww;
            font->h += rowh;
            roww = 0;
            rowh = 0;
        }
        roww += g->bitmap.width + 1;
        if (g->bitmap.rows > rowh)
            rowh = g->bitmap.rows;
    }
    if (roww > font->w)
        font->w = roww;
    font->h += rowh;

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &font->tex);
    glBindTexture(GL_TEXTURE_2D, font->tex);
    //glUniform1i(text_texture_uniform, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, font->w, font->h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
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
        glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, g->bitmap.width, g->bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);
        font->c[i].ax = g->advance.x >> 6;
        font->c[i].ay = g->advance.y >> 6;
        font->c[i].bw = g->bitmap.width;
        font->c[i].bh = g->bitmap.rows;
        font->c[i].bl = g->bitmap_left;
        font->c[i].bt = g->bitmap_top;
        font->c[i].tx = ox / (float)font->w;
        font->c[i].ty = oy / (float)font->h;
        if (g->bitmap.rows > rowh)
            rowh = g->bitmap.rows;
        ox += g->bitmap.width + 1;
    }

    return font;
}

static void delete_font(struct font *font)
{
    glDeleteTextures(1, &font->tex);
    free(font);
}

#define ALIGN_TOPLEFT 0
#define ALIGN_BOTTOMLEFT 1
#define ALIGN_TOPRIGHT 2
#define ALIGN_BOTTOMRIGHT 3

static void draw_text(struct font *font, int x, int y, int align, const char* restrict format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    static char text[1024];
    int length = vsprintf(text, format, argptr);
    va_end(argptr);
    if (length == -1)
        return;

    glBindTexture(GL_TEXTURE_2D, font->tex);
    glUniform1i(text_texture_uniform, 0);

    glEnableVertexAttribArray(text_coord_attrib);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glVertexAttribPointer(text_coord_attrib, 4, GL_FLOAT, GL_FALSE, 0, 0);

    y = win_height - y;
    struct point coords[6 * length];
    int c = 0;
    for (uint8_t *p = (uint8_t*)text; *p; p++) {
        float x2 = x + font->c[*p].bl;
        float y2 = -y - font->c[*p].bt;
        float w = font->c[*p].bw;
        float h = font->c[*p].bh;
        x += font->c[*p].ax;
        y += font->c[*p].ay;
        if (!w || !h)
            continue;
        coords[c++] = (struct point){ x2, -y2, font->c[*p].tx, font->c[*p].ty };
        coords[c++] = (struct point){ x2 + w, -y2, font->c[*p].tx + font->c[*p].bw / font->w, font->c[*p].ty };
        coords[c++] = (struct point){ x2, -y2 - h, font->c[*p].tx, font->c[*p].ty + font->c[*p].bh / font->h };
        coords[c++] = (struct point){ x2 + w, -y2, font->c[*p].tx + font->c[*p].bw / font->w, font->c[*p].ty };
        coords[c++] = (struct point){ x2, -y2 - h, font->c[*p].tx, font->c[*p].ty + font->c[*p].bh / font->h };
        coords[c++] = (struct point){ x2 + w, -y2 - h, font->c[*p].tx + font->c[*p].bw / font->w, font->c[*p].ty + font->c[*p].bh / font->h };
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof coords, coords, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, c);
    glDisableVertexAttribArray(text_coord_attrib);
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

static GLuint make_shader_program(const char *vertex_file, const char *fragment_file)
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

static void update_view()
{
    need_update = false;
    glViewport(0, 0, win_width, win_height);

    zoom *= pow(ZOOM_SENSITIVITY, input.scroll);
    view_center[0] -= (float)SCROLL_SENSITIVITY * input.panx / zoom;
    view_center[1] += (float)SCROLL_SENSITIVITY * input.pany / zoom;
    mat4x4_identity(projection);
    //float span = 1.0f / zoom / (win_width>win_height ? win_height : win_width);
    mat4x4_ortho(projection,
        -win_width/zoom/70 + view_center[0],
        win_width/zoom/70 + view_center[0],
        -win_height/zoom/70 + view_center[1],
        win_height/zoom/70 + view_center[1],
        -1, 1);
    glUseProgram(star_shader);
    glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, (const GLfloat*)projection);

    mat4x4_identity(text_projection);
    mat4x4_ortho(text_projection, 0, win_width, 0, win_height, -1, 1);
    glUseProgram(text_shader);
    glUniformMatrix4fv(text_projection_uniform, 1, GL_FALSE, (const GLfloat*)text_projection);
}

static void glfw_error(int error, const char* description)
{
    fprintf(stderr, "GLFW error: %s\n", description);
}

static void glfw_resize(GLFWwindow* window, int width, int height)
{
    win_width = width;
    win_height = height;
    need_update = true;
}

void finalize_graphics()
{
    if (status_font) {
        delete_font(status_font);
        status_font = NULL;
    }
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
    star_shader = make_shader_program("star.vert", "star.frag");
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
    text_shader = make_shader_program("text.vert", "text.frag");
    if(text_shader == 0) {
        finalize_graphics();
        return NULL;
    }
    text_coord_attrib = glGetAttribLocation(text_shader, "coord");
    text_projection_uniform = glGetUniformLocation(text_shader, "projection");
    text_texture_uniform = glGetUniformLocation(text_shader, "tex");
    text_color_uniform = glGetUniformLocation(text_shader, "color");
    if(text_coord_attrib == -1 || text_texture_uniform == -1 || text_color_uniform == -1) {
        finalize_graphics();
        return NULL;
    }
    status_font = new_font(FONT, 12);
    if (!status_font) {
        finalize_graphics();
        return NULL;
    }

    return window;
}

void draw()
{
    if (need_update || input.scroll || input.panx || input.pany)
        update_view();
    glClear(GL_COLOR_BUFFER_BIT);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * PARTICLE_COUNT, pos_display, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, star_buffer);
    glInterleavedArrays(GL_V2F, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, star_vbo);
    glUseProgram(star_shader);
    glDrawArraysInstanced(GL_TRIANGLES, 0, PARTICLE_COUNT, PARTICLE_COUNT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(text_shader);
    glUniform4fv(text_color_uniform, 1, (GLfloat[]){ 0, 1, 0, 0.7 });
    draw_text(status_font, 20, 30, ALIGN_TOPLEFT, "X: %.2f  Y: %.2f", view_center[0], view_center[1]);
    draw_text(status_font, 20, 50, ALIGN_TOPLEFT, "W: %d  H: %d", win_width, win_height);
    if (zoom > 1)
        draw_text(status_font, 20, 70, ALIGN_TOPLEFT, "Zoom: %ld:1", (long)(zoom+0.5));
    else
        draw_text(status_font, 20, 70, ALIGN_TOPLEFT, "Zoom: 1:%ld", (long)(1.0f/zoom+0.5));
    draw_text(status_font, 20, 90, ALIGN_TOPLEFT, "%d FPS", (int)(fps+0.5));
    glDisable(GL_BLEND);

    glfwSwapBuffers(window);
}
