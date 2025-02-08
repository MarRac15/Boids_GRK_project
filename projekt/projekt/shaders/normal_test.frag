#version 430 core

uniform sampler2D colorTexture;
uniform sampler2D normalSampler;
uniform vec3 lightColor;

in vec2 texCoord;
in vec3 fragPos_TS;
in vec3 lightPos_TS;
in vec3 viewDir_TS;
in mat3 TBN;

out vec4 outColor;

void main()
{

    vec3 N = texture(normalSampler, texCoord).rgb;
    N = normalize(N * 2.0 - 1.0); 
    vec3 texColor = texture(colorTexture, texCoord).rgb;


    vec3 lightDir = normalize(lightPos_TS - fragPos_TS);
    vec3 viewDir = normalize(viewDir_TS - fragPos_TS);
    vec3 reflectDir = reflect(-lightDir, N);
    vec3 halfwayDir = normalize(lightDir + viewDir);  

    //Phong:
    vec3 ambient = 0.1 * texColor;
    float diffuse = max(dot(N, lightDir), 0.0);
    vec3 diff = diffuse * texColor;
    float specular = pow(max(dot(N, halfwayDir), 0.0), 32.0);
    vec3 spec = vec3(0.2) * specular;

    vec3 phong = ambient + diff + spec;
    
    
    outColor = vec4(phong, 1.0);
}
