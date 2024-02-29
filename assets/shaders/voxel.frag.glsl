#version 330 core

in vec3 v_position;
in vec3 v_normal;
in vec2 v_texCoord;
in float v_occlusion;

uniform vec3 u_cameraPosition;
uniform mat4 u_viewProjection;
uniform sampler2D u_atlas;
uniform vec4 u_color;

out vec4 color;

vec3 applyFog( in vec3  rgb,      // original color of the pixel
               in float dist,     // camera to point distance
               in vec3  rayOri,   // camera position
               in vec3  rayDir,   // camera to point vector
               in vec3  sunDir )  // camera to point vector
{
    const float a = 0.0025;
    const float b = .1;

    float fogAmount = (a/b) * exp(-rayOri.y*b) * (1.0-exp( -dist*rayDir.y*b ))/rayDir.y;
    float sunAmount = max( dot( rayDir, sunDir ), 0.0 );
    vec3  fogColor  = mix( vec3(0.5,0.6,0.7), // bluish
                           vec3(1.0,0.9,0.7), // yellowish
                           pow(sunAmount,8.0) );
    return mix( rgb, fogColor, fogAmount );
}

void main()
{
    vec3 lightPos = vec3(3, 10, 3);
    vec3 lightDirection = normalize(lightPos);

    float lightIntensity = 0.2;

    float dirLight = lightIntensity * max(dot(lightDirection, v_normal), 0);
    float maxAmbient = 0.5;
    float minAmbient = 0.2;

    float light = min((dirLight + (maxAmbient - minAmbient)) * v_occlusion + minAmbient, 1);

    vec4 tex = texture(u_atlas, v_texCoord);
    color = u_color * light * tex;
    color.a = tex.a;

    vec3 dir = v_position - u_cameraPosition;
    float dist = length(dir);
    color.rgb = applyFog(color.rgb, dist, u_cameraPosition, dir / dist, lightDirection);
}