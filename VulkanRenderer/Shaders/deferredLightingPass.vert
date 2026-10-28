#version 460

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoords;

layout (location = 0) out vec2 TexCoords;
layout (location = 1) out vec4 viewPos;
layout (location = 2) out vec4 lightPos;
layout (location = 3) out mat4 InvViewProjMat;

layout (push_constant) uniform pushConstant 
{
	mat4 invViewProjMat;
} matrix;

layout (set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMat;
    vec4 viewPos;
    vec4 lightPos;
} ubo;

void main() 
{
	TexCoords = inTexCoords;
	InvViewProjMat = matrix.invViewProjMat;
    viewPos = ubo.viewPos;
    lightPos = ubo.lightPos;
	gl_Position = vec4(inPosition, 1.0, 1.0);
}
