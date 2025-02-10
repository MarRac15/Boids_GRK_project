#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

uniform mat4 transformation;
uniform mat4 modelMatrix;


out vec3 vNormal;
out vec3 vPosition;
out vec3 worldPos;

void main()
{

	vNormal = (modelMatrix * vec4(vertexNormal,0.0)).xyz;
	vPosition = (modelMatrix * vec4(vertexPosition,1.0)).xyz;
	worldPos = (modelMatrix* vec4(vertexPosition,1)).xyz;

	gl_Position = transformation * vec4(vertexPosition, 1.0);
}
