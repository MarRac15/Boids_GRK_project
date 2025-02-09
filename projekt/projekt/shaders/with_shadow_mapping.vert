#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
//layout(location = 2) in vec2 vertexTexCoord;
//layout(location = 3) in vec3 vertexTangent;
//layout(location = 4) in vec3 vertexBitangent;

uniform mat4 transformation;
uniform mat4 modelMatrix;
uniform mat4 lightVP;


//out vec2 texCoord;
out vec3 vNormal;
out vec3 vPosition;
out vec3 worldPos;
out vec4 sunSpacePos;

void main()
{
	//texCoord = vertexTexCoord;
	vNormal = (modelMatrix * vec4(vertexNormal,0.0)).xyz;
	vPosition = (modelMatrix * vec4(vertexPosition,1.0)).xyz;
	sunSpacePos = lightVP*modelMatrix * vec4(vertexPosition,1.0);
	worldPos = (modelMatrix* vec4(vertexPosition,1)).xyz;


	//vec3 viewDir = normalize(cameraPos - worldPos);
	//vec3 lightDir = normalize(lightPos - worldPos);
	//vec3 lightDir = pointlightDir;
	//vec3 reflectDir = reflect(-pointlightDir,vNormal);

	gl_Position = transformation * vec4(vertexPosition, 1.0);
}
