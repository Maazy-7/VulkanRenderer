#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) out vec4 gNormalMetallic;//xyz normal and w metallic
layout (location = 1) out vec4 gAlbedoRoughness;//xyz albedo/diffuse and w roughness

layout (set = 1, binding = 0) uniform sampler2D textures[];

layout (push_constant) uniform push_constants 
{
	layout (offset = 64) vec4 index;//x = albedo/diffuse, y = metallic, z = roughness, w is unused
}textureIndices;

layout (location = 0) in vec2 fragTexCoord;
layout (location = 1) in vec3 fragNormal;

void main() 
{
	//glDepth = fragPos, depth written automatically
	
	//temp - this if statement is used to determine if material maps should be used for metallic and roughness or if a constant value should be used
	vec4 values = textureIndices.index;
	if (!(values.w > 0)) 
	{
		ivec4 indices = ivec4(textureIndices.index);
		gNormalMetallic = vec4(normalize(fragNormal), texture(textures[nonuniformEXT(indices.y)], fragTexCoord).r);
		gAlbedoRoughness.xyz = texture(textures[nonuniformEXT(indices.x)], fragTexCoord).xyz;
		gAlbedoRoughness.w = texture(textures[nonuniformEXT(indices.z)], fragTexCoord).r;
	}
	else 
	{	//x is the metallic value, y the roughness value, and z is a contant value representing a color, could be either red, blue or green
		gNormalMetallic = vec4(normalize(fragNormal), values.x);
		gAlbedoRoughness.xyz = vec3(values.z, 0.0, 0.0);//just red for now
		gAlbedoRoughness.w = values.y;
	}
}