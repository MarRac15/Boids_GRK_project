#version 330 core
layout (location = 0) in vec3 vPos;

out vec3 TexCoords;

uniform mat4 transformation;

void main()
{
    TexCoords = vPos;
    gl_Position = transformation * vec4(vPos, 1.0);
} 