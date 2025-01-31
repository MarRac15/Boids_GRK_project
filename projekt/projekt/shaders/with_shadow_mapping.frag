#version 430 core

uniform vec3 color;

float AMBIENT = 0.1;

uniform vec3 lightColor;
uniform vec3 cameraPos;

uniform vec3 pointlightDir;

uniform sampler2D depthMap;

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



	vec3 lightDir = normalize(pointlightDir);

	vec3 normalVector = normalize(vNormal);
	float intensityDiffuse = dot(normalVector,-lightDir);
	intensityDiffuse = max(intensityDiffuse,0.0);

	vec3 V = normalize(cameraPos - vPosition);
	vec3 R = reflect(-lightDir,normalVector);

	float intensitySpecular = dot(V,R);
	intensitySpecular = max(intensitySpecular,0.0);
	intensitySpecular = pow(intensitySpecular,32);

	float shadow = (1.0-calculateShadow(sunSpacePos));

	//float distance = length(vPosition - lightPos);
	vec3 fragColor = lightColor* (color * min(1,AMBIENT+shadow*(intensityDiffuse +intensitySpecular)));
	outColor = vec4(fragColor,1.0);
	
}
