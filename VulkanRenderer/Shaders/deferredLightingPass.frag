#version 450

layout (push_constant) uniform view 
{
	layout (offset = 64) vec4 viewPos;
} ViewPos;

layout (set = 0, binding = 0) uniform UniformBufferObject //TODO put this in vertex shader because thats where it belongs
{
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMat;
    vec4 viewPos;
    vec4 lightPos;
} ubo;

layout (set = 1, binding = 0) uniform sampler2D gDepthMap;
layout (set = 1, binding = 1) uniform sampler2D gNormalMap;
layout (set = 1, binding = 2) uniform sampler2D gAlbedoSpecMap;
layout (set = 1, binding = 3) uniform samplerCubeShadow shadowCubemap;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in mat4 InvProjMat;

layout (location = 0) out vec4 outFragPos;

vec3 getPositionFromDepthMap(vec2 uv) 
{
    float depth = texture(gDepthMap, TexCoords).r;
    vec4 ndcPos = vec4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, depth, 1.0);
    vec4 worldPos = InvProjMat * ndcPos;
    return worldPos.xyz / worldPos.w;
}

float calculateCubemapShadow(vec3 fragPosition, vec3 fragNormal) 
{
	vec3 len = fragPosition - ubo.lightPos.xyz;
	vec3 dir = normalize(-len);
	float ref = length(len)/25.0;
	float bias = max(0.05* (1.0 - dot(fragNormal, dir)), 0.005);
	return texture(shadowCubemap, vec4(normalize(len), ref - bias));
}

void main() 
{
    vec3 norm = normalize(texture(gNormalMap, TexCoords).xyz);
    vec3 lightColor = vec3(0.3);
    vec3 texColor = texture(gAlbedoSpecMap, TexCoords).xyz;
    vec3 texSpecular = vec3(texture(gAlbedoSpecMap, TexCoords).w);
    vec3 ambient = 0.2 * texColor;
    vec3 FragPos = getPositionFromDepthMap(TexCoords);

    vec3 lightDir = normalize(ubo.lightPos.xyz - FragPos);
    float diff = max(dot(lightDir, norm), 0.0);
	vec3 diffuse = texColor * diff;

	vec3 viewDir = normalize(ubo.viewPos.xyz - FragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(norm, halfwayDir), 0.0), 32);
	vec3 specular = spec * lightColor * texSpecular;

    float shadow = calculateCubemapShadow(FragPos, norm);

    outFragPos = vec4(ambient + shadow * (diffuse + specular), 1.0); 
}