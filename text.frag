#version 130

uniform sampler2D texture;
uniform vec4 color;
in vec2 texture_pos;

void main(void)
{
    gl_FragColor = vec4(color.rgb, color.a * texture2D(texture, texture_pos).a);
}
