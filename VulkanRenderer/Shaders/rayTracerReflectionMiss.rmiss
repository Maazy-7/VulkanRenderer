#version 460

#extension GL_EXT_ray_tracing : require

layout (location = 0) rayPayloadInEXT vec3 reflectionColor;

layout (push_constant) uniform pushConstant 
{
	layout (offset = 64) vec4 backgroundColor;
} color;

void main() 
{
	reflectionColor = color.backgroundColor.xyz;
}