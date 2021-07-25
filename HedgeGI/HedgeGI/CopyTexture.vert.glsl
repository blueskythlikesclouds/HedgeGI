#version 330

layout (location = 0) in vec2 aPosition;

out vec2 fTexCoord;

uniform vec4 uRect;

void main()
{
    fTexCoord = aPosition * 0.5 + 0.5;
    fTexCoord = fTexCoord * uRect.zw + uRect.xy;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
