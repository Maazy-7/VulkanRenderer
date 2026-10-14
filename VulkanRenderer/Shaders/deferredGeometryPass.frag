#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) out vec4 gNormalMetallic;//xyz normal and w metallic
layout (location = 1) out vec4 gAlbedoRoughness;//xyz albedo/diffuse and w roughness

layout (set = 1, binding = 0) uniform sampler2D textures[];

layout (push_constant) uniform push_constants 
{
	layout (offset = 64) ivec4 index;//x = albedo/diffuse, y = metallic, z = roughness, w is unused
}textureIndices;

layout (location = 0) in vec2 fragTexCoord;
layout (location = 1) in vec3 fragNormal;

void main() 
{
	//glDepth = fragPos, depth written automatically
	gNormalMetallic = vec4(normalize(fragNormal), texture(textures[nonuniformEXT(textureIndices.index.y)], fragTexCoord).r);
	gAlbedoRoughness.xyz = texture(textures[nonuniformEXT(textureIndices.index.x)], fragTexCoord).xyz;
	gAlbedoRoughness.w = texture(textures[nonuniformEXT(textureIndices.index.z)], fragTexCoord).r;
}