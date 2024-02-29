#version 330 core

in vec3  v_localPosition;
in vec4  v_color;
in float v_fade;
in vec2 v_texCoord;
in float v_texIndex;

uniform sampler2D u_textures[10];
uniform mat4 u_viewProjection;

out vec4 color;

void main()
{
    // Calculate pixel distance from center
    float dist = length(v_localPosition);
    
    // Get color based on whether the pixel is in circle
    float t = smoothstep(1 - v_fade, 1 + v_fade, dist);
    float col = mix(1.0, 0.0, t);
    
    color = col * v_color * texture(u_textures[int(v_texIndex)], v_texCoord);
}