#version 460
#extension GL_EXT_multiview : require

layout (push_constant) uniform ModelMatrix 
{
	mat4 matrix;
} modelMatrix;

layout (std140, set = 0, binding = 0) uniform ShadowMatrices 
{
	mat4 matrices[6];
} shadowMatrices;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inTangent;

layout (location = 0) out vec4 FragPos;

void main() 
{
	FragPos = modelMatrix.matrix * vec4(inPosition, 1.0);
	gl_Position = shadowMatrices.matrices[gl_ViewIndex] * FragPos;
}
