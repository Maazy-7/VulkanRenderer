#version 460

layout (push_constant) uniform PushConstant 
{
	layout (offset = 64) vec4 lightPosAndFarPlane;
} pushConstant;

layout (location = 0) in vec4 FragPos;

void main() 
{
	float lightDistance = length(FragPos.xyz - pushConstant.lightPosAndFarPlane.xyz);
	float depth = clamp(lightDistance / pushConstant.lightPosAndFarPlane.w, 0.0, 1.0);
	gl_FragDepth = depth;
}
