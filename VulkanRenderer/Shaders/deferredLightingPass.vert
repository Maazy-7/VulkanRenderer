#version 450

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoords;

layout (location = 0) out vec2 TexCoords;
layout (location = 1) out mat4 InvProjMat;

layout (push_constant) uniform pushConstant 
{
	mat4 invProjMat;
} matrix;

void main() 
{
	TexCoords = inTexCoords;
	InvProjMat = matrix.invProjMat;
	gl_Position = vec4(inPosition, 1.0, 1.0);
}