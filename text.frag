#version 130

varying vec2 texture_pos;
uniform sampler2D texture;
uniform vec4 color;

void main(void)
{
    gl_FragColor = vec4(color.rgb, color.a * texture2D(texture, texture_pos).a);
}
