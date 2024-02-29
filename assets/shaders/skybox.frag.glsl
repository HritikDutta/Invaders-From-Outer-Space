#version 440 core

in vec3 v_texCoord;

uniform mat4 u_matrix;
uniform samplerCube u_skybox;

out vec4 o_color;

void main()
{
    o_color = texture(u_skybox, v_texCoord);
}