#version 330

in vec2 fTexCoord;
in float fScale;

out vec4 oColor;

uniform sampler2D uTexture;
uniform float uGamma;

void main()
{
    oColor = vec4(texture(uTexture, fTexCoord).rgb * fScale, 1);
    oColor.rgb = pow(clamp(oColor.rgb, 0, 1), vec3(uGamma));
}