#version 130
#extension GL_ARB_draw_instanced: enable
#extension GL_ARB_explicit_attrib_location: enable

// Star quad
const float star_size = 0.5;  // equals to graphics.h::update_view()::star_size
const vec2 star[] = vec2[](
    vec2(-star_size, -star_size),
    vec2(-star_size,  star_size),
    vec2( star_size, -star_size),
    vec2( star_size,  star_size)
);

// Star quad
const vec2 texture_coords[] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
);

uniform mat4 projection;
uniform int texture_size;
layout(location=1) in vec2 star_position;
layout(location=2) in vec3 star_color;
out vec2 texture_pos;
out vec3 f_star_color;

void main()
{
    gl_Position = projection * vec4(star_position + star[gl_VertexID], 0, 1);
    texture_pos = texture_coords[gl_VertexID];
    f_star_color = star_color;
}
