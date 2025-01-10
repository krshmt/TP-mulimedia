#version 150 core

in vec3 lightDir;
in vec3 eyeVec;
in vec3 out_normal;

out vec4 frag_color;

void main(void)
{
    vec3 L = normalize(lightDir);
    vec3 N = normalize(out_normal);
    vec3 E = normalize(eyeVec);
    vec3 R = reflect(-L, N);

    float intensity = max(dot(L, N), 0.0);
    float specular = pow(max(dot(R, E), 0.0), 2.0);

    vec4 final_color = vec4(0.2, 0.2, 0.2, 1.0);
    final_color += 0.6 * intensity;
    final_color += vec4(0.8, 0.8, 0.8, 1.0) * specular;

    frag_color = final_color;
}