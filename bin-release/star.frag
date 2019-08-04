#version 130

uniform sampler2D texture;
in vec2 texture_pos;
in vec3 f_star_color;

void main()
{
    gl_FragColor = vec4(f_star_color, texture2D(texture, texture_pos).a);
}
