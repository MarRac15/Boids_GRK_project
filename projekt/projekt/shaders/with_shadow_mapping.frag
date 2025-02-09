#version 430 core

uniform vec3 color;

//float AMBIENT = 0.1;

uniform vec3 lightColor;
uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec3 pointlightDir;

uniform sampler2D depthMap;
//uniform sampler2D colorTexture;

in vec3 vNormal;
in vec3 vPosition;
in vec3 worldPos;
in vec4 sunSpacePos;


out vec4 outColor;



float calculateShadow(vec4 lightSpacePos){
	vec3 normalizedPos = lightSpacePos.xyz/lightSpacePos.w;
	normalizedPos = normalizedPos*0.5 +0.5;
	float currentDepth = normalizedPos.z;

	float closestDepth = texture(depthMap,normalizedPos.xy).r;

	float bias = 0.05;

	float shadow = currentDepth -bias> closestDepth ? 1.0:0.0;

	return shadow;

}

void main()
{


	vec3 N = normalize(vNormal);

	//diffuse:
	vec3 ambient = 0.3 * lightColor;
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
	

	//shadow mapping leftovers:
	//float shadow = (1.0-calculateShadow(sunSpacePos));
	//float distance = length(vPosition - lightPos);
	//vec3 fragColor = lightColor* (color * min(1,AMBIENT+shadow*(intensityDiffuse +intensitySpecular)));
	//outColor = vec4(fragColor,1.0);

	outColor = vec4(ambient + diffuse + specular, 1.0) * vec4(color, 1.0);
	
}
