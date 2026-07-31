#version 450

layout (location = 0) out vec4 FragColor;

layout (push_constant) uniform Color
{
	layout (offset = 64) vec4 uniformColor;
}color;

void main() 
{
	FragColor = color.uniformColor;
}