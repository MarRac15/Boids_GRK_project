#version 430 core

uniform vec3 color;


uniform vec3 lightColor;
uniform vec3 cameraPos;

uniform sampler2D depthMap;
uniform sampler2D colorTexture;
uniform sampler2D normalSampler;
uniform bool useNormalMapping;

in vec2 texCoord;
in vec3 vNormal;
in vec3 vPosition;
in vec3 worldPos;
in vec4 sunSpacePos;
in vec3 lightDir_TS;
in vec3 viewDir_TS;


out vec4 outColor;




void main()
{
	vec3 N;

	if (useNormalMapping)
	{
	N = texture(normalSampler, texCoord).rgb;
	N = normalize(N*2.0 - 1.0);
	}
	else
	{
	vec3 N = normalize(vNormal);
	}
	
	vec3 textureColor = texture2D(colorTexture, texCoord).rgb;

	//diffuse:
	vec3 ambient = 0.3 * textureColor;
	vec3 lightDir = normalize(lightDir_TS);
	float intensityDiffuse = max(dot(lightDir, N), 0.0);

	//specular:
	vec3 viewDir = normalize(viewDir_TS);
	vec3 halfwayDir = normalize(viewDir + lightDir);
	vec3 reflectDir = reflect(-lightDir, N);
	float intensitySpecular = pow(max(dot(N, halfwayDir), 0.0), 32.0);
	vec3 diffuse = intensityDiffuse * textureColor;
	vec3 specular = vec3(0.2) * intensitySpecular;
	

	//outColor = vec4(N.rgb*0.5+0.5, 1.0);
	outColor = vec4(ambient + diffuse + specular, 1.0);
	
}
