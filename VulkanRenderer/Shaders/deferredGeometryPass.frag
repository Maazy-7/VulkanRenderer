#version 450

layout (location = 0) out vec4 gNormal;//vec4 for normal only really contains vec3 info but soon might add something useful for the 4th component
layout (location = 1) out vec4 gAlbedoSpec;

layout (set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout (set = 1, binding = 1) uniform sampler2D specularTexture;

layout (location = 0) in vec2 fragTexCoord;
layout (location = 1) in vec3 fragNormal;

void main() 
{
	//glDepth = fragPos, depth written automatically
	gNormal = vec4(normalize(fragNormal), 1.0);
	gAlbedoSpec.xyz = texture(diffuseTexture, fragTexCoord).xyz;
	gAlbedoSpec.w = texture(specularTexture, fragTexCoord).w;
}