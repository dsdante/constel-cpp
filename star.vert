#version 130
#extension GL_ARB_draw_instanced: enable

// Star quad
const vec2 star[] = vec2[](
    vec2(-0.1, -0.1),
    vec2(-0.1,  0.1),
    vec2( 0.1, -0.1),
    vec2( 0.1,  0.1)
);

uniform mat4 projection;
in vec2 star_position;
in vec3 star_color;
out vec3 f_star_color;

void main()
{
    gl_Position = projection * vec4(star_position + star[gl_VertexID], 0, 1);
    f_star_color = star_color;
}
