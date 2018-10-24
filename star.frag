#version 140

uniform int stars;
uniform samplerBuffer star_colors;
uniform samplerBuffer star_positions;

void main()
{
    gl_FragColor = vec4(0, 0, 0, 1);
    for (int i = 0; i < 500; i++) {
        float len = length(gl_FragCoord.xy - texelFetch(star_positions, i).xy * 30 - vec2(960, 590));
        if (len > 100)
            continue;   
        gl_FragColor += texelFetch(star_colors, i) / (len*len);
    }
}
