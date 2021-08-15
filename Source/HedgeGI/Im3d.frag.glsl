#version 330

in vec4 fColor;
in vec4 fPosition;

out vec4 oResult;

uniform vec4 uRect;
uniform sampler2D uTexture;

uniform bool uApplyPattern;

void main() 
{
    vec3 texCoord = fPosition.xyz / fPosition.w;
    texCoord.xy = texCoord.xy * 0.5 + 0.5;

    float cmpDepth = texture(uTexture, texCoord.xy * uRect.zw + uRect.xy).w;
    ivec2 pattern = ivec2(gl_FragCoord.xy - 0.5) % 2;
    oResult = fColor * (cmpDepth < texCoord.z || (uApplyPattern && pattern.x == 0 && pattern.y == 1) ? 0.0 : 1.0);
}