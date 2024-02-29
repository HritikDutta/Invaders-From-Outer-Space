#version 440 core

layout (location = 0) in vec3 position;

uniform mat4 u_matrix;
uniform samplerCube u_skybox;

out vec3 v_texCoord;

void main()
{
    v_texCoord = position;
    vec4 pos = u_matrix * vec4(position, 1);
    gl_Position = pos.xyww;
}