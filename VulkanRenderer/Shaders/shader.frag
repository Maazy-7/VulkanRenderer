#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

void main() 
{
	FragColor = vec4(fragTexCoord.y, 0.0, 0.0, 1.0);
}