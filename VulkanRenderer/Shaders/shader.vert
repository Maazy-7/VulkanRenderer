#version 450

layout (push_constant) uniform ModelMatrix
{
    mat4 model;
} modelMatrix;

layout (set = 0, binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMat;
    vec4 viewPos;
    vec4 lightPos;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec3 fragNormal;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out vec3 fragPos;
layout (location = 3) out vec4 fragPosLightSpace;
layout (location = 4) out vec4 viewPos;
layout (location = 5) out vec4 lightPos;

void main() 
{
    fragPos = vec3(modelMatrix.model * vec4(inPosition, 1.0));
    fragPosLightSpace = ubo.lightSpaceMat * vec4(fragPos, 1.0);
	gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);
	fragNormal = mat3(transpose(inverse(modelMatrix.model))) * inNormal;
    fragTexCoord = inTexCoord;
    viewPos = ubo.viewPos;
    lightPos = ubo.lightPos;
}