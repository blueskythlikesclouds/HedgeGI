#version 330

in vec2 fTexCoord;
in float fScale;

out vec4 oColor;

uniform sampler2D uTexture;
uniform float uGamma;

uniform bool uEnableRgbTable;
uniform sampler2D uRgbTableTex;

void main()
{
    oColor = vec4(texture(uTexture, fTexCoord).rgb * fScale, 1);
    oColor.rgb = pow(clamp(oColor.rgb, 0, 1), vec3(uGamma));

    if (uEnableRgbTable)
    {
        vec2 rgbTableParam = textureSize(uRgbTableTex, 0);

        float cell = oColor.b * (rgbTableParam.y - 1);

        float cellL = floor(cell);
        float cellH = ceil(cell);

        float floatX = 0.5 / rgbTableParam.x;
        float floatY = 0.5 / rgbTableParam.y;
        float rOffset = floatX + oColor.r / rgbTableParam.y * ((rgbTableParam.y - 1) / rgbTableParam.y);
        float gOffset = floatY + oColor.g * ((rgbTableParam.y - 1) / rgbTableParam.y);

        vec2 lutTexCoordL = vec2(cellL / rgbTableParam.y + rOffset, gOffset);
        vec2 lutTexCoordH = vec2(cellH / rgbTableParam.y + rOffset, gOffset);

        vec4 gradedColorL = textureLod(uRgbTableTex, lutTexCoordL, 0);
        vec4 gradedColorH = textureLod(uRgbTableTex, lutTexCoordH, 0);

        oColor.rgb = mix(gradedColorL, gradedColorH, fract(cell)).rgb;
    }
}