#version 460

layout (set = 1, binding = 0) uniform sampler2D gDepthMap;
layout (set = 1, binding = 1) uniform sampler2D gNormalMetallicMap;
layout (set = 1, binding = 2) uniform sampler2D gAlbedoRoughnessMap;
layout (set = 1, binding = 3) uniform samplerCubeShadow shadowCubemap;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec4 viewPos;
layout (location = 2) in vec4 lightPos;
layout (location = 3) in mat4 InvViewProjMat;

layout (location = 0) out vec4 outFragPos;

layout (push_constant) uniform p 
{
    layout (offset = 64) vec4 val;
}boolean;

const float PI = 3.14159265359;
const float EPS = 1e-5;

vec3 getPositionFromDepthMap(vec2 uv)
{
    float depth = texture(gDepthMap, TexCoords).r;
    vec4 ndcPos = vec4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, depth, 1.0);
    vec4 worldPosH = InvViewProjMat * ndcPos;
    vec3 worldPos = worldPosH.xyz / worldPosH.w;
    return worldPos;
}

float calculateCubemapShadow(vec3 fragPosition, vec3 fragNormal) 
{
	vec3 len = fragPosition - lightPos.xyz;
	vec3 dir = normalize(-len);
	float ref = length(len)/25.0;
	float bias = max(0.05* (1.0 - dot(fragNormal, dir)), 0.005);
	return texture(shadowCubemap, vec4(normalize(len), ref - bias));
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) 
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5);
}

float distributionGGX(vec3 N, vec3 H, float roughness) 
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N,H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return a2 / max(denom, EPS);
}

float geometrySchlickGGX(float NdotV, float k) 
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float k) 
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, k) * geometrySchlickGGX(NdotL, k);
}

void main() 
{
    vec3 FragPos = getPositionFromDepthMap(TexCoords);
    vec4 NormalMetallicMap = texture(gNormalMetallicMap, TexCoords).xyzw;
    vec4 AlbedoRoughnessMap = texture(gAlbedoRoughnessMap, TexCoords).xyzw;
    vec3 norm = normalize(NormalMetallicMap.xyz);
    float metallic = NormalMetallicMap.w;
    vec3 albedo = AlbedoRoughnessMap.xyz;
    float roughness = clamp(AlbedoRoughnessMap.w, 0.04, 1.0);

    vec3 viewDir = normalize(viewPos.xyz - FragPos);
    vec3 lightDir = normalize(lightPos.xyz - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 lightColor = vec3(1.0);

    //Cook Torrance brdf terms
    float NdotL = max(dot(norm, lightDir), 0.0);
    float NdotV = max(dot(norm, viewDir), 0.0);
    float NdotH = max(dot(norm, halfwayDir), 0.0);
    float VdotH = max(dot(viewDir, halfwayDir), 0.0);

    vec3 F0 = vec3(0.04);//fresnel for dielectrics is usualy 0.04 at normal incidence
    F0 = mix(F0, albedo, metallic);//metals use the albedo as their fresnel, so default F0 mixed with albedo based of metallicness

    vec3 F = fresnelSchlick(VdotH, F0);
    float D = distributionGGX(norm, halfwayDir, roughness);
    float k = roughness + 1.0;
    k = (k * k) / 8.0;//
    float G = geometrySmith(norm, viewDir, lightDir, k);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(NdotV * NdotL, EPS);
    vec3 specular = numerator / denominator;

    //diffuse term
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * (albedo / PI);

    float shadow = calculateCubemapShadow(FragPos, norm);

    //final color directly from light
    vec3 Lo = (diffuse + specular) * lightColor * NdotL * shadow;
    
    //indirect lighting/ambient - simple value for now
    vec3 ambient = vec3(0.1) * albedo * (1.0 - metallic);
    vec3 color = ambient + Lo;
    color = color / (color + 1.0);
    color = pow(color, vec3(1.0/2.2));
    outFragPos = vec4(color, 1.0);
}
