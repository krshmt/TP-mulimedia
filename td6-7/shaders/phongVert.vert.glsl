#version 150 core

uniform mat4 u_mvp;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

in vec3 a_pos;
in vec3 a_normal;

out vec4 v_color;
out vec3 v_lightDir;
out vec3 v_eyeVec;
out vec3 v_normal;

float random(vec2 st) {
  return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main(void) {
  v_color = vec4(-a_normal, 0.0);
  vec4 vertex = u_view * u_model * vec4(a_pos, 1.0);
  v_eyeVec = -vertex.xyz;
  vec4 lightPos = vec4(0.0, 0.0, 10.0, 0.0);
  v_lightDir = vec3(lightPos.xyz - vertex.xyz);

  v_normal = vec3(u_view * u_model * vec4(a_normal.x, a_normal.y, a_normal.z, 0.0));

  gl_Position = u_mvp * (vec4(a_pos, 1.0) + 0.1 * random(a_pos.xy));
}