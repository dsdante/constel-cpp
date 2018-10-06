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
struct atlas
{
    GLuint tex;     // texture object

    unsigned int w;         // width of texture in pixels
    unsigned int h;         // height of texture in pixels

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
} *status_atlas;

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

void delete_atlas(struct atlas *atlas)
{
    glDeleteTextures(1, &atlas->tex);
    free(atlas);
}

void finalize_graphics()
{
    delete_atlas(status_atlas);
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

struct atlas* new_atlas(FT_Face face, int height)
{
    const int texture_max_width = 1024;
    struct atlas *atlas = (struct atlas*) malloc(sizeof(struct atlas));
    FT_Set_Pixel_Sizes(face, 0, height);
    FT_GlyphSlot g = face->glyph;
    unsigned int roww = 0;
    unsigned int rowh = 0;
    atlas->w = 0;
    atlas->h = 0;
    memset(atlas->c, 0, sizeof(atlas->c));

    for (int i = 32; i < 128; i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "Loading character %c failed!\n", i);
            continue;
        }
        if (roww + g->bitmap.width + 1 >= texture_max_width) {
            if (roww > atlas->w)
                atlas->w = roww;
            atlas->h += rowh;
            roww = 0;
            rowh = 0;
        }
        roww += g->bitmap.width + 1;
        if (g->bitmap.rows > rowh)
            rowh = g->bitmap.rows;
    }
    if (roww > atlas->w)
        atlas->w = roww;
    atlas->h += rowh;

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &atlas->tex);
    glBindTexture(GL_TEXTURE_2D, atlas->tex);
    glUniform1i(text_texture_uniform, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, atlas->w, atlas->h, 0, GL_ALPHA,
    GL_UNSIGNED_BYTE, 0);
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
            fprintf(stderr, "Loading character %c failed!\n", i);
            continue;
        }
        if (ox + g->bitmap.width + 1 >= texture_max_width) {
            oy += rowh;
            rowh = 0;
            ox = 0;
        }
        glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, g->bitmap.width, g->bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);
        atlas->c[i].ax = g->advance.x >> 6;
        atlas->c[i].ay = g->advance.y >> 6;
        atlas->c[i].bw = g->bitmap.width;
        atlas->c[i].bh = g->bitmap.rows;
        atlas->c[i].bl = g->bitmap_left;
        atlas->c[i].bt = g->bitmap_top;
        atlas->c[i].tx = ox / (float) atlas->w;
        atlas->c[i].ty = oy / (float) atlas->h;
        if (g->bitmap.rows > rowh)
            rowh = g->bitmap.rows;
        ox += g->bitmap.width + 1;
    }

    return atlas;
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
    status_atlas = new_atlas(face, 48);

    return window;
}

void draw_text(const char *text, struct atlas *a, float x, float y, float sx, float sy)
{
    const uint8_t *p;

    /* Use the texture containing the atlas */
    glBindTexture(GL_TEXTURE_2D, a->tex);
    glUniform1i(text_texture_uniform, 0);

    /* Set up the VBO for our vertex data */
    glEnableVertexAttribArray(text_coord_attrib);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glVertexAttribPointer(text_coord_attrib, 4, GL_FLOAT, GL_FALSE, 0, 0);

    struct point coords[6 * strlen(text)];
    int c = 0;

    /* Loop through all characters */
    for (p = (const uint8_t *) text; *p; p++) {
        /* Calculate the vertex and texture coordinates */
        float x2 = x + a->c[*p].bl * sx;
        float y2 = -y - a->c[*p].bt * sy;
        float w = a->c[*p].bw * sx;
        float h = a->c[*p].bh * sy;

        /* Advance the cursor to the start of the next character */
        x += a->c[*p].ax * sx;
        y += a->c[*p].ay * sy;

        /* Skip glyphs that have no pixels */
        if (!w || !h)
            continue;

        coords[c++] = (struct point ) { x2, -y2, a->c[*p].tx, a->c[*p].ty };
        coords[c++] = (struct point ) { x2 + w, -y2, a->c[*p].tx + a->c[*p].bw / a->w,
                        a->c[*p].ty };
        coords[c++] = (struct point ) { x2, -y2 - h, a->c[*p].tx, a->c[*p].ty
                        + a->c[*p].bh / a->h };
        coords[c++] = (struct point ) { x2 + w, -y2, a->c[*p].tx + a->c[*p].bw / a->w,
                        a->c[*p].ty };
        coords[c++] = (struct point ) { x2, -y2 - h, a->c[*p].tx, a->c[*p].ty
                        + a->c[*p].bh / a->h };
        coords[c++] =
                (struct point ) { x2 + w, -y2 - h, a->c[*p].tx
                                + a->c[*p].bw / a->w, a->c[*p].ty
                                + a->c[*p].bh / a->h };
    }

    /* Draw all the character on the screen in one go */
    glBufferData(GL_ARRAY_BUFFER, sizeof coords, coords, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, c);

    glDisableVertexAttribArray(text_coord_attrib);
}

void draw()
{
    if (input.scroll || input.panx || input.pany)
        update_view();
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFunc(GL_ONE, GL_ONE);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * PARTICLE_COUNT, pos_display, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, star_buffer);
    glInterleavedArrays(GL_V2F, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, star_vbo);
    glUseProgram(star_shader);
    glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, (const GLfloat*)projection);
    glDrawArraysInstanced(GL_TRIANGLES, 0, PARTICLE_COUNT, PARTICLE_COUNT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(text_shader);
    FT_Set_Pixel_Sizes(face, 0, 48);
    glUniform4fv(text_color_uniform, 1, (GLfloat[]){ 0, 1, 0, 0.7 });
    char status_str[64];
    sprintf(status_str, "X: %.2f  Y: %.2f", view_center[0], view_center[1]);
    draw_text(status_str, status_atlas, -0.9, 0.8, 1.0/win_width, 1.0/win_height);
    sprintf(status_str, "Zoom: %.2f%%", zoom*100);
    draw_text(status_str, status_atlas, -0.9, 0.75, 1.0/win_width, 1.0/win_height);
    sprintf(status_str, "%d FPS", (int)(fps+0.5));
    draw_text(status_str, status_atlas, -0.9, 0.70, 1.0/win_width, 1.0/win_height);
    //glUniform4fv(text_color_uniform, 1, (GLfloat[]){ 1, 0, 0, 0.7 });
    //sprintf(status_str, "ACCESS DENIED");
    //draw_text(status_str, status_atlas, -0.25, -0.9, 2.0/win_width, 2.0/win_height);
    glDisable(GL_BLEND);

    glfwSwapBuffers(window);
}
