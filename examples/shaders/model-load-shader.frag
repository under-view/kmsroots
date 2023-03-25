#version 450  // GLSL 4.5
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 2) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outColor;

void main() {
  vec4 color = texture(samplerColorMap, inTexCoord) * vec4(inColor, 1.0);

  vec3 N = normalize(inNormal);
  vec3 L = normalize(inLightVec);
  vec3 V = normalize(inViewVec);
  vec3 R = reflect(L, N);
  vec3 diffuse = max(dot(N, L), 0.15) * inColor;
  vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
  outColor = vec4(diffuse * color.rgb + specular, 1.0);
}
