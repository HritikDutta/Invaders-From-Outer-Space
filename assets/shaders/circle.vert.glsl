#version 330 core

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 localPosition;
layout(location = 2) in vec4 color;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in float fade;
layout(location = 5) in float texIndex;

uniform sampler2D u_textures[10];
uniform mat4 u_viewProjection;

out vec3  v_localPosition;
out vec4  v_color;
out float v_fade;
out vec2 v_texCoord;
out float v_texIndex;

void main()
{
    v_localPosition = localPosition;
    v_color = color;
    v_fade = fade;
    v_texCoord = texCoord;
    v_texIndex = texIndex;

    gl_Position = u_viewProjection * vec4(worldPosition, 1.0);
}