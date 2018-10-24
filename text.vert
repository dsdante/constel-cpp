#version 130

uniform mat4 projection;
uniform vec2 text_pos;  // position of the whole text
in vec4 char_pos;  // x/y: char position,  z/w: texture offset 
out vec2 texture_pos;

void main(void)
{
    gl_Position = projection * vec4(text_pos + char_pos.xy, 0, 1);
    texture_pos = char_pos.zw;
}
