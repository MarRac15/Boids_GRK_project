#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;

uniform mat4 transformation;
uniform mat4 modelMatrix;
uniform vec3 lightPos;
uniform vec3 cameraPos;

out vec2 texCoord;
out vec3 fragPos_TS;
out vec3 lightPos_TS;
out vec3 viewDir_TS;
out mat3 TBN;

void main()
{
    texCoord = vertexTexCoord;


    vec3 fragPos = (modelMatrix * vec4(vertexPosition, 1.0)).xyz;
    vec3 normal = normalize(mat3(modelMatrix) * vertexNormal);
    vec3 tangent = normalize(mat3(modelMatrix) * vertexTangent);
    vec3 bitangent = cross(normal, tangent);


    TBN = mat3(tangent, bitangent, normal);
    fragPos_TS = TBN * fragPos;
    lightPos_TS = TBN * lightPos;
    viewDir_TS = TBN * cameraPos;

    gl_Position = transformation * vec4(vertexPosition, 1.0);
}
