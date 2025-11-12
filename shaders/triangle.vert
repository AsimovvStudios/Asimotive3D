#version 450

layout(location = 0) in vec3 in_positions;
layout(location = 1) in vec3 in_colour;

layout(location = 0) out vec3 out_colour;

void main()
{
	gl_Position = vec4(in_positions, 1.0);
	out_colour = in_colour;
}
