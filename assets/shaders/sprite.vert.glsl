#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in float texIndex;

uniform sampler2D u_textures[10];
uniform mat4 u_viewProjection;

out vec2 v_texCoord;
out float v_texIndex;

void main()
{
    v_texCoord = texCoord;
    v_texIndex = texIndex;
    gl_Position = u_viewProjection * vec4(position, 1.0);
}