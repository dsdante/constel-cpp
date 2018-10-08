#version 130
#extension GL_ARB_draw_instanced: enable

uniform mat4 projection;
in vec2 aPos;
in vec4 aColor;
in vec2 aOffset;
out vec4 fColor;

void main()
{
    gl_Position = projection * vec4(aPos + aOffset, 0.0, 1.0);
    fColor = aColor;
}
