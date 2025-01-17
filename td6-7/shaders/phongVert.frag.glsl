#version 150 core

in vec4 color;
in vec3 eyeVec;
in vec3 lightDir;
in vec3 out_normal;

out vec4 frag_color;

void main( void )
{
    vec3 L = normalize(lightDir);
    vec3 N = normalize(out_normal);

    float intensity = max(dot(N, L), 0.0) - 0.8; // "-0.8" est ajouté car le reflect sur la couleur verte est trop forte (avis personnel)
    vec4 final_color = vec4(0.0,0.6,0.0, 1.0);
    vec3 E = normalize(eyeVec);
    vec3 R = reflect(-L, N);
    float specular = pow(max(dot(R, E), 0.0),2);

    final_color += 0.6 * intensity;
    final_color += vec4(0.8,0.8,0.8,1.0) * specular;

    frag_color = final_color;
}

