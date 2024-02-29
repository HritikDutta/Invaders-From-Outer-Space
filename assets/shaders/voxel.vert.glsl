#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in float occlusion;

uniform vec3 u_cameraPosition;
uniform mat4 u_viewProjection;
uniform sampler2D u_atlas;
uniform vec4 u_color;

out vec3 v_position;
out vec3 v_normal;
out vec2 v_texCoord;
out float v_occlusion;

void main()
{
    gl_Position = (u_viewProjection * vec4(position, 1));
    v_position = position;
    v_normal = normal;
    v_texCoord = texCoord;
    v_occlusion = occlusion;
}