#version 330 core

in vec2 v_texCoord;
in vec4 v_color;
in float v_texIndex;

uniform sampler2D u_textures[10];

out vec4 color;

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    vec4 sample = texture(u_textures[int(v_texIndex)], v_texCoord);
    ivec2 size = textureSize(u_textures[int(v_texIndex)], 0).xy;

    float dx = dFdx(v_texCoord.x) * size.x;
    float dy = dFdy(v_texCoord.y) * size.y;
    float toPixels = 8.0 * inversesqrt(dx * dx + dy * dy);
    float sigDist = median(sample.r, sample.g, sample.b);
    float w = fwidth(sigDist);
    float alpha = smoothstep(0.5 - w, 0.5 + w, sigDist);
    
    color = v_color * vec4(1, 1, 1, alpha);
}