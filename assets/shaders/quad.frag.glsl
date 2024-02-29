#version 330 core

in vec2 v_texCoord;
in vec4 v_color;
in float v_texIndex;

uniform sampler2D u_textures[10];

out vec4 color;

void main()
{
    color = v_color * texture(u_textures[int(v_texIndex)], v_texCoord);
}