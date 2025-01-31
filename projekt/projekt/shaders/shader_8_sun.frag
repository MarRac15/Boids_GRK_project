#version 430 core

uniform vec3 color;
uniform float exposition;


out vec4 outColor;
void main()
{
	outColor = vec4(vec3(1.0) - exp(-color*exposition),1);
}
