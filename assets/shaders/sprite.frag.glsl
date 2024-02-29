#version 330 core

in vec2 v_texCoord;
in float v_texIndex;

uniform sampler2D u_textures[10];
uniform mat4 u_viewProjection;

out vec4 color;

void main()
{
    color = texture(u_textures[int(v_texIndex)], v_texCoord);
}