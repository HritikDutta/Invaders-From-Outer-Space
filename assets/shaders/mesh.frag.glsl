#version 330 core

in vec3 v_normal;
in vec4 v_color;

out vec4 color;

void main()
{
    vec3 lightPos = vec3(3, 2, 10);
    vec3 lightDirection = normalize(lightPos - vec3(0, 0, 0));
    float light = min(max(dot(lightDirection, v_normal), 0) + 0.4, 0.9);
    color = light * v_color;
    color.a = 1;
}