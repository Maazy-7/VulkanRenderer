#version 450

layout (set = 1, binding = 0) uniform sampler2D texSampler;
layout (set = 1, binding = 1) uniform sampler2D shadowMap;

layout (location = 0) in vec3 fragNormal;
layout (location = 1) in vec2 fragTexCoord;
layout (location = 2) in vec3 fragPos;

layout (location = 0) out vec4 FragColor;

layout (push_constant) uniform LightData 
{
	layout(offset = 64) vec4 lightPos;
	vec4 viewPos;
	vec4 objectColor;
} lightData;

void main() 
{
	vec3 norm = normalize(fragNormal);
	vec3 lightColor = vec3(0.3);
	vec3 texColor;
	if (lightData.objectColor.w > 0.0) 
	{
		texColor = lightData.objectColor.xyz;
	}
	else { texColor = texture(texSampler, fragTexCoord).rgb; }
	vec3 ambient = 0.1 * texColor;

	vec3 lightDir = normalize(vec3(lightData.lightPos) - fragPos);
	float diff = max(dot(lightDir, norm), 0.0);
	vec3 diffuse = texColor * diff;

	vec3 viewDir = normalize(vec3(lightData.viewPos) - fragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(norm, halfwayDir), 0.0), 32);
	vec3 specular = spec * lightColor;

	FragColor = vec4(texture(shadowMap, fragTexCoord));//vec4(ambient + diffuse + specular, 1.0);
}