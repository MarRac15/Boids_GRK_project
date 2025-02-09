#version 430 core

layout(location = 0) in vec3 vertexPosition;

uniform mat4 heightmapScale;

void main()
{
	gl_Position = heightmapScale * vec4(vertexPosition, 1.0);
	
}
