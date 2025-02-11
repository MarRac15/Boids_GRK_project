#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 transformation;
uniform mat4 modelMatrix;

uniform vec3 cameraPos;
uniform vec3 pointlightDir;
uniform vec3 lightPos;

out vec2 texCoord;
out vec3 vNormal;
out vec3 vPosition;
out vec3 worldPos;
out vec3 viewDir_TS;
out vec3 lightDir_TS;

void main()
{
	texCoord = vertexTexCoord;
	vNormal = normalize((modelMatrix * vec4(vertexNormal,0.0)).xyz);
	vPosition = (modelMatrix * vec4(vertexPosition,1.0)).xyz;
	worldPos = (modelMatrix* vec4(vertexPosition,1)).xyz;

	vec3 tangent = normalize((modelMatrix * vec4(vertexTangent, 0.0)).xyz);
	vec3 bitangent = normalize((modelMatrix * vec4(vertexBitangent, 0.0)).xyz);
	mat3 TBN = transpose(mat3(tangent, bitangent, vNormal));

	vec3 viewDir = normalize(cameraPos - worldPos);
	vec3 lightDir = normalize(lightPos - worldPos);
	vec3 reflectDir = reflect(-pointlightDir,vNormal);

	viewDir_TS = TBN * viewDir;
	lightDir_TS = TBN * lightDir;

	gl_Position = transformation * vec4(vertexPosition, 1.0);
	
}
