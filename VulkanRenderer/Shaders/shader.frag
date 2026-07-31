#version 450

layout (set = 1, binding = 0) uniform sampler2D texSampler;
layout (set = 1, binding = 1) uniform sampler2DShadow shadowMap;

layout (location = 0) in vec3 fragNormal;
layout (location = 1) in vec2 fragTexCoord;
layout (location = 2) in vec3 fragPos;
layout (location = 3) in vec4 fragPosLightSpace;
layout (location = 4) in vec4 viewPos;
layout (location = 5) in vec4 lightPos;

layout (location = 0) out vec4 FragColor;

layout (push_constant) uniform LightData 
{
	layout (offset = 64) vec4 objectColor;
} lightData;

float calculateShadow(vec4 lightSpaceFragPos, vec3 lightDirection, vec3 normal) 
{
	vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;//perspective division on light space fragPos
	projCoords.xy = projCoords.xy * 0.5 + 0.5;//z coord already in [0,1] so only transforming x and y

	if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) 
	{ return 0.0; }//no shadow if fragment outside lights view frustum

	//shadow bias
	float bias = max(0.05 * (1.0 - dot(fragNormal, lightDirection)), 0.005);
	//hardware accelerated shadow map comparison and pcf
	float visibility = texture(shadowMap, vec3(projCoords.xy, projCoords.z - bias));
	return visibility;
}

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

	vec3 lightDir = normalize(vec3(lightPos) - fragPos);
	float diff = max(dot(lightDir, norm), 0.0);
	vec3 diffuse = texColor * diff;

	vec3 viewDir = normalize(vec3(viewPos) - fragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(norm, halfwayDir), 0.0), 32);
	vec3 specular = spec * lightColor;

	float shadow = calculateShadow(fragPosLightSpace, lightDir, norm);

	FragColor = vec4(ambient + shadow*(diffuse + specular), 1.0);
}