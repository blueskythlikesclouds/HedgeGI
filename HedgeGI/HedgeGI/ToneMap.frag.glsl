#version 330

in vec2 fTexCoord;
in float fAvgLuminance;

out vec4 oColor;

uniform sampler2D uTexture;
uniform bool uApplyGamma;

void main()
{
    oColor = vec4(texture(uTexture, fTexCoord).rgb / fAvgLuminance, 1);

    if (uApplyGamma)
        oColor.rgb = pow(oColor.rgb, vec3(1.0 / 2.2));
}