#version 330

in vec2 fTexCoord;

out float oColor;

uniform sampler2D uTexture;
uniform vec2 uLumMinMax;

void main()
{
    oColor = clamp(dot(texture(uTexture, fTexCoord).rgb, vec3(0.2126, 0.7152, 0.0722)), uLumMinMax.x, uLumMinMax.y);
}