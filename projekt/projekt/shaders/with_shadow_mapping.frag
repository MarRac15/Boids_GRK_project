#version 430 core

uniform vec3 color;

float AMBIENT = 0.1;

uniform vec3 lightColor;
uniform vec3 cameraPos;
//uniform vec3 pointlightDir;

uniform sampler2D depthMap;
uniform sampler2D colorTexture;

//in vec3 vNormal;
in vec3 vPosition;
in vec3 worldPos;
in vec4 sunSpacePos;
in vec3 lightDir_TS;
in vec3 viewDir_TS;
in vec3 reflectDir_TS;


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



	vec3 lightDir = normalize(lightDir_TS);
	vec3 viewDir = normalize(viewDir_TS);
	vec3 normalVector = vec3(0,0,1);
	//vec3 normalVector = normalize(vNormal);
	float intensityDiffuse = dot(normalVector,-lightDir);
	intensityDiffuse = max(intensityDiffuse,0.0);

	//vec3 V = normalize(cameraPos - vPosition);
	vec3 V = normalize(viewDir_TS);
	vec3 R = normalize(reflectDir_TS);
	//vec3 R = reflect(-lightDir,normalVector);

	float intensitySpecular = dot(V,R);
	intensitySpecular = max(intensitySpecular,0.0);
	intensitySpecular = pow(intensitySpecular,32);

	float shadow = (1.0-calculateShadow(sunSpacePos));

	//float distance = length(vPosition - lightPos);
	vec4 textureColor = texture2D(colorTexture, texCoord);
	vec3 fragColor = lightColor* (color * min(1,AMBIENT+shadow*(intensityDiffuse +intensitySpecular)));
	outColor = fragColor * vec4(textureColor.r, textureColor.g, textureColor.b, 1.0);
	
}
