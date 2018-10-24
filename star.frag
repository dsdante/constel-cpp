#version 130

in vec3 f_star_color;

void main()
{
	gl_FragColor = vec4(f_star_color, 0.5);
}
