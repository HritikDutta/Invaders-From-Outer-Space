#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 color;
layout(location = 3) in float texIndex;

uniform sampler2D u_textures[10];

out vec2 v_texCoord;
out vec4 v_color;
out float v_texIndex;

void main()
{
    v_texCoord = texCoord;
    v_color = color;
    v_texIndex = texIndex;
    gl_Position = vec4(position, 1.0);
}