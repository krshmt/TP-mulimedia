#version 150 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

in vec3 in_pos;
in vec3 in_normal;

out vec3 lightDir;
out vec3 eyeVec;
out vec3 out_normal;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main(void)
{
    mat4 modelview = view * model;
    vec4 vVertex = modelview * vec4(in_pos, 1.0);
    eyeVec = -vVertex.xyz;

    vec4 LightSource_position = vec4(0.0, 0.0, 10.0, 0.0);
    lightDir = vec3(LightSource_position.xyz - vVertex.xyz);

    out_normal = vec3(
        modelview * vec4(
            in_normal.x + 0.5 * random(in_pos.xy),
            in_normal.y + 0.5 * random(in_pos.xz),
            in_normal.z + 0.5 * random(in_pos.yz),
            0.0
        )
    );

    gl_Position = proj * modelview * (vec4(in_pos, 1.0) + 0.1 * random(in_pos.xy));
}