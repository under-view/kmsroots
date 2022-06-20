#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
  vec4 gl_Position;
};

layout(location = 0) in vec2 i_Position;
layout(location = 1) in vec3 i_Color;
layout(location = 0) out vec3 v_Color;

void main() {
  gl_Position = vec4(i_Position, 0.0, 1.0);
  v_Color = i_Color;
}
