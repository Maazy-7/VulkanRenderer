#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) out vec4 gNormal;//vec4 for normal only really contains vec3 info but soon might add something useful for the 4th component
layout (location = 1) out vec4 gAlbedoSpec;

layout (set = 1, binding = 0) uniform sampler2D textures[];

layout (push_constant) uniform push_constants 
{
	layout (offset = 64) ivec4 index;//all vec4 x = diffuse texture, y equals specular texture
}textureIndices;

layout (location = 0) in vec2 fragTexCoord;
layout (location = 1) in vec3 fragNormal;

void main() 
{
	//glDepth = fragPos, depth written automatically
	gNormal = vec4(normalize(fragNormal), 1.0);
	gAlbedoSpec.xyz = texture(textures[nonuniformEXT(textureIndices.index.x)], fragTexCoord).xyz;
	gAlbedoSpec.w = texture(textures[nonuniformEXT(textureIndices.index.y)], fragTexCoord).r;
}