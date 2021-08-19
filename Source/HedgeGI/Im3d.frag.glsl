#version 330

in vec4 fColor;
in vec4 fPosition;

out vec4 oResult;

uniform vec4 uRect;
uniform sampler2D uTexture;

uniform float uDiscardFactor;

void main() 
{
    vec3 texCoord = fPosition.xyz / fPosition.w;
    texCoord.xy = texCoord.xy * 0.5 + 0.5;

    float cmpDepth = texture(uTexture, texCoord.xy * uRect.zw + uRect.xy).w;
    oResult = fColor * (cmpDepth < texCoord.z ? uDiscardFactor : 1.0);
}