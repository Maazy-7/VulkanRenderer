#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_EXT_nonuniform_qualifier : require

struct Vertex 
{
	vec4 pos;
	vec4 normal;
	vec4 uv;
	vec4 tangent;
};

layout (buffer_reference, scalar) buffer VertexBufferRef
{
	Vertex vertices[];
};

layout (buffer_reference, scalar) buffer IndexBufferRef
{
	uint indices[];
};

struct MeshAddresses 
{
	uint64_t vertexAddress;
	uint64_t indexAddress;
	uint32_t albedoTextureId;
    uint32_t normalMapTextureId;
    uint32_t roughnessTextureId;
    uint32_t metallicTextureId;
};

layout (set = 2, binding = 0) uniform accelerationStructureEXT tlas;

//ssbo which stores the MeshAddresses struct, indexed by the instance index in the tlas
layout(set = 2, binding = 6, scalar) buffer MeshAddressBuffer {
    MeshAddresses addresses[];
} meshLayouts;

//global texture descriptor array
layout(set = 1, binding = 0) uniform sampler2D textures[];

layout (location = 0) rayPayloadInEXT vec3 reflectionColor;
layout (location = 1) rayPayloadEXT float shadowVisibility;//used to calculate shadow using another traceray call

hitAttributeEXT vec2 barycentricCoords;

//descriptor set owned by vulkan rendering backend, a per frame set for frame data
layout (set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMat;
    vec4 viewPos;
    vec4 lightPos;
} ubo;

const float PI = 3.14159265359;
const float EPS = 1e-5;

vec3 fresnelSchlick(float cosTheta, vec3 F0);
float distributionGGX(vec3 N, vec3 H, float roughness);
float geometrySchlickGGX(float NdotV, float k);
float geometrySmith(vec3 N, vec3 V, vec3 L, float k);

float hash21(vec2 p) 
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

vec3 randomOnUnitSphere(vec2 uv) 
{
    float u = hash21(uv + vec2(1.0, 0.0));
    float v = hash21(uv + vec2(0.0, 2.0));
    
    float theta = u * 2.0 * 3.14159265359;
    float phi = acos(2.0 * v - 1.0);
    
    return vec3(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
}

void main()
{
    //resolving the mesh's data by indexing the mesh addresses ssbo using the instance custom index from the tlas
    MeshAddresses meshAddress = meshLayouts.addresses[gl_InstanceCustomIndexEXT];
    VertexBufferRef vertexBuffer = VertexBufferRef(meshAddress.vertexAddress);
    IndexBufferRef indexBuffer = IndexBufferRef(meshAddress.indexAddress);

    //extracting indices from index buffer using primitive id
    uint indexOffset = gl_PrimitiveID * 3;
    uint i0 = indexBuffer.indices[indexOffset + 0];
    uint i1 = indexBuffer.indices[indexOffset + 1];
    uint i2 = indexBuffer.indices[indexOffset + 2];

    //extracting vertices of primitive using indices
    Vertex v0 = vertexBuffer.vertices[i0];
    Vertex v1 = vertexBuffer.vertices[i1];
    Vertex v2 = vertexBuffer.vertices[i2];

    //computing berycentrics, beta = barycentric.x, gamma = barycentric.y, alpha = 1.0 - beta - gamma, barycentricCoords provided from hardware
    vec3 barycentrics = vec3(1.0 - barycentricCoords.x - barycentricCoords.y, barycentricCoords.x, barycentricCoords.y);
    
    //interpolating vertex attributes using the barycentric coordinates
    vec3 localPos = v0.pos.xyz * barycentrics.x + v1.pos.xyz * barycentrics.y + v2.pos.xyz * barycentrics.z;
    vec2 uv = v0.uv.xy * barycentrics.x + v1.uv.xy * barycentrics.y + v2.uv.xy * barycentrics.z;
    vec3 localNormal = v0.normal.xyz * barycentrics.x + v1.normal.xyz * barycentrics.y + v2.normal.xyz * barycentrics.z;
    vec4 localTangent = vec4(v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent *  barycentrics.z);

    //creating TBN matrix so that normal mapping can be used
    vec3 N = normalize(vec3(vec4(localNormal, 0.0) * gl_WorldToObject3x4EXT));
    vec3 T = normalize(vec3(gl_ObjectToWorldEXT * localTangent));
    T = normalize(T - dot(N, T) * N);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    //unpacking normal from normal map
    vec3 tangentNormal = texture(textures[nonuniformEXT(meshAddress.normalMapTextureId)], uv).rgb;
    //tangentNormal * 2.0 - 1.0;
    vec3 worldNormal = normalize(TBN * tangentNormal);

    //extracting material properties from the material textures
    vec3 albedo = texture(textures[nonuniformEXT(meshAddress.albedoTextureId)], uv).rgb;
    float metallic = texture(textures[nonuniformEXT(meshAddress.metallicTextureId)], uv).r;
    float roughness = texture(textures[nonuniformEXT(meshAddress.roughnessTextureId)], uv).r;
    roughness = clamp(roughness, 0.04, 1.0);

    //computing necessary variables for cook torrance pbr
    vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(localPos, 1.0));//equivalent to frag pos in rasterization fragment shader
    vec3 V = normalize(-gl_WorldRayDirectionEXT);//view direction of surface hit by ray is just negative of hit direction
    vec3 L = normalize(ubo.lightPos.xyz - worldPos);
    vec3 H = normalize(V + L);
    vec3 lightColor = vec3(ubo.lightPos.w);

    float NdotL = max(dot(worldNormal, L), 0.0);
    float NdotV = max(dot(worldNormal, V), 0.0);
    float NdotH = max(dot(worldNormal, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    vec3 F0 = vec3(0.04);//fresnel for dielectrics is usualy 0.04 at normal incidence
    F0 = mix(F0, albedo, metallic);//metals use the albedo as their fresnel, so default F0 mixed with albedo based of metallicness

    vec3 F = fresnelSchlick(VdotH, F0);
    float D = distributionGGX(worldNormal, H, roughness);
    float k = roughness + 1.0;
    k = (k * k) / 8.0;
    float G = geometrySmith(worldNormal, V, L, k);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(NdotV * NdotL, EPS);
    vec3 specular = numerator / denominator;

    //diffuse term
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * (albedo / PI);

    //computing shadow for the reflection using a traceRayEXT call, just like in the rgen shader
    float shadow = 1.0;
    int computeRayTracedShadow = 1;//determines if shadows for the reflection should be computed
    shadowVisibility = 0.0;
    if (computeRayTracedShadow == 1) 
    {
        vec3 rayOrigin = worldPos + worldNormal * 0.001;
        uint shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        float tMin = 0.001;
        float tMax = length(ubo.lightPos.xyz - rayOrigin);//max ray distance is a little after the light object itself
        vec3 randomDir = randomOnUnitSphere(vec2(gl_LaunchIDEXT.xy)) * 0.1;//should use ray payload to ensure random directions are the same
        vec3 lightDirection = normalize((ubo.lightPos.xyz + randomDir) - worldPos);
        traceRayEXT(tlas, shadowRayFlags, 0xFF, 0, 0, 1, rayOrigin, tMin, lightDirection, tMax, 1);
        shadow = shadowVisibility;//shadow ray traced result stored here, if ray intersects something 0 is returned meaning shadow otherwise miss returns 1
    }

    //final color directly from light
    vec3 Lo = (diffuse + specular) * lightColor * NdotL * shadow;
    
    //indirect lighting/ambient - simple value for now
    vec3 ambient = vec3(0.1) * albedo * (1.0 - metallic);
    vec3 color = ambient + Lo;
    //color = color / (color + 1.0);
    //color = pow(color, vec3(1.0/2.2));
    reflectionColor = color;
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