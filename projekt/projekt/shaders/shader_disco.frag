#version 430 core

uniform vec3 color;

uniform vec3 lightColor;
uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec3 pointlightDir;

uniform float time;

in vec3 vNormal;
in vec3 vPosition;
in vec3 worldPos;


out vec4 outColor;


void main()
{

	vec3 N = normalize(vNormal);

	//diffuse:
	vec3 ambient = 0.4 * lightColor;
	//vec3 lightDir = normalize(lightPos - worldPos);
	vec3 lightDir = normalize(pointlightDir);
	float intensityDiffuse = max(dot(lightDir, N), 0.0);

	//specular:
	vec3 viewDir = normalize(cameraPos - worldPos);
	vec3 halfwayDir = normalize(viewDir + lightDir);
	vec3 reflectDir = reflect(-lightDir, N);
	float intensitySpecular = pow(max(dot(N, halfwayDir), 0.0), 32.0);
	vec3 diffuse = intensityDiffuse * lightColor;
	vec3 specular = intensitySpecular * lightColor;

	vec3 discoColor = vec3(
		sin(time * 2.0) * 0.5 + 0.5,
		sin(time * 2.5 + 2.0) * 0.5 + 0.5,
		sin(time * 3.0 + 4.0) * 0.5 + 0.5
	);

	vec3 mixedColor = mix(color, discoColor, 0.7);
	

	outColor = vec4(ambient + diffuse + specular, 1.0) * vec4(mixedColor, 1.0);
	
}
