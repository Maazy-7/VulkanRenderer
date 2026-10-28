#version 450

layout (push_constant) uniform ModelMatrix
{
    mat4 model;
} modelMatrix;

layout (set = 0, binding = 0) uniform UniformBufferObject //don't need lighting values here
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
layout (location = 3) in vec3 inTangent;

layout (location = 0) out vec2 fragTexCoord;
//layout (location = 1) out vec4 FragPos; since position will be computed from depth buffer frag pos isn't needed
layout (location = 1) out vec3 fragNormal;
layout (location  = 2) out mat3 TBNmatrix;

void main() 
{
    gl_Position = ubo.proj * ubo.view * modelMatrix.model * vec4(inPosition, 1.0);
    fragNormal = transpose(inverse(mat3(modelMatrix.model))) * inNormal;
    fragTexCoord = inTexCoord;
    
    //creating TBN matrix
    vec3 T = normalize(inTangent);
    vec3 N = normalize(inNormal);
    T = normalize(T - dot(N,T) * N);
    vec3 B = cross(N,T);
    TBNmatrix = mat3(T, B, N);
}