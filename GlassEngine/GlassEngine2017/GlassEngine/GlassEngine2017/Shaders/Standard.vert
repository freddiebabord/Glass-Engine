#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 viewProjection;
	int currentMesh;
	vec3 eyePosition;
} ubo;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out int meshID;
layout(location = 3) out vec3 eyePos;
layout(location = 4) out vec3 fragPos;


void main() {
    gl_Position = ubo.viewProjection * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
	meshID = ubo.currentMesh;
	eyePos = ubo.eyePosition;
	fragPos = vec3(ubo.model * vec4(inPosition, 1.0f));
}