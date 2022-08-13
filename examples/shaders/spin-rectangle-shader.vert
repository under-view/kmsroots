#version 450  // GLSL 4.5
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 outColor;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(binding = 0) uniform uniform_buffer {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

void main() {
  gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
  outColor = inColor;
}
