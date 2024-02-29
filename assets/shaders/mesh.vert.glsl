#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

uniform mat4 u_viewProjection;
uniform mat4 u_transform;

out vec3 v_normal;
out vec4 v_color;

void main()
{
    gl_Position = (u_viewProjection * u_transform * vec4(position, 1));
    v_normal = normal;
    v_color = color;
}