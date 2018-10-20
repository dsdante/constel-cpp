#version 130

uniform mat4 projection;
attribute vec4 coord;
varying vec2 texture_pos;

void main(void)
{
    gl_Position = projection * vec4(coord.xyz, 1);
    texture_pos = coord.zw;
}
