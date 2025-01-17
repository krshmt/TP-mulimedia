#version 150 core

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

in vec3 in_pos;
in vec3 in_normal;

out vec4 color;
out vec3 lightDir;
out vec3 eyeVec;
out vec3 out_normal;

float random (vec2 st) {
  return fract(sin(dot(st.xy,vec2(12.9898,78.233)))*43758.5453123);
}

void main(void)
{color = vec4(-in_normal, 0.0);
  vec4 vVertex = view*model * vec4(in_pos, 1.0);
  eyeVec = -vVertex.xyz;
  vec4 LightSource_position=vec4(0.0,0.0,10.0,0.0);
  lightDir=vec3(LightSource_position.xyz - vVertex.xyz);

  out_normal =vec3(view*model*vec4(in_normal.x,in_normal.y,in_normal.z, 0.0));
  
  gl_Position = mvp * vec4( in_pos, 1.0 ); // classique
}
