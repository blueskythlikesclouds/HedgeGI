#version 330

layout (location = 0) in vec2 aPosition;

out vec2 fTexCoord;
out float fAvgLuminance;

uniform vec4 uRect;
uniform sampler2D uAvgLuminanceTex;

void main()
{
    fTexCoord = aPosition * 0.5 + 0.5;
    fTexCoord = fTexCoord * uRect.zw + uRect.xy;

    vec3 color = texelFetch(uAvgLuminanceTex, ivec2(0, 0), 8).rgb;
    fAvgLuminance = min((0.001 + dot(color, vec3(0.2126, 0.7152, 0.0722))) * 2.0, 65534);

    gl_Position = vec4(aPosition, 0.0, 1.0);
}
