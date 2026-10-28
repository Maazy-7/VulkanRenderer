#version 450 core

layout (push_constant) uniform MatrixData 
{
	mat4 lightSpaceMatrix;
	mat4 modelMatrix;
} matrixData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inTangent;

void main() 
{
	gl_Position = matrixData.lightSpaceMatrix * matrixData.modelMatrix * vec4(inPosition, 1.0);
}