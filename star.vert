#version 130

const vec2 screen[] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);

void main()
{
    // Draw a rectangle covering all the screen
    // All the actual drawing is done in the fragment shader
    
    gl_Position = vec4(screen[gl_VertexID], 0.0, 1.0);
}