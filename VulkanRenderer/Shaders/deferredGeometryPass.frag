#version 460

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) out vec4 gNormalMetallic;//xyz normal and w metallic
layout (location = 1) out vec4 gAlbedoRoughness;//xyz albedo/diffuse and w roughness

layout (set = 1, binding = 0) uniform sampler2D textures[];

layout (push_constant) uniform push_constants 
{
	layout (offset = 64) ivec4 index;//x = albedo/diffuse, y = metallic, z = roughness, w is normal map
}textureIndices;

layout (location = 0) in vec2 fragTexCoord;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in mat3 TBNmatrix;

void main() 
{
	//glDepth = fragPos, depth written automatically
	ivec4 indices = textureIndices.index;
	
	vec3 normal = vec3(0.0);
	if (indices.w < 0) { normal = fragNormal; }//checking if valid normal map exists if not use regular frag normal
	else { normal = texture(textures[nonuniformEXT(indices.w)], fragTexCoord).xyz; normal = normalize(TBNmatrix * normal); }

	gNormalMetallic = vec4(normal, texture(textures[nonuniformEXT(indices.y)], fragTexCoord).r);
	gAlbedoRoughness.xyz = texture(textures[nonuniformEXT(indices.x)], fragTexCoord).xyz;
	gAlbedoRoughness.w = texture(textures[nonuniformEXT(indices.z)], fragTexCoord).r;
}
