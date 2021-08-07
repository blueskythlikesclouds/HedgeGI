#version 330

layout (location = 0) in vec2 aPosition;

out vec2 fTexCoord;
out float fScale;

uniform vec4 uRect;
uniform float uMiddleGray;
uniform sampler2D uAvgLuminanceTex;

void main()
{
    fTexCoord = aPosition * 0.5 + 0.5;
    fTexCoord = fTexCoord * uRect.zw + uRect.xy;

    fScale = (1.0 / min(0.001 + texelFetch(uAvgLuminanceTex, ivec2(0, 0), 8).r, 65504)) * uMiddleGray;

    gl_Position = vec4(aPosition, 0.0, 1.0);
}
