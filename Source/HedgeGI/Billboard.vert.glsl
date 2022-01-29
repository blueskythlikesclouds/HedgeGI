#version 330

layout (location = 0) in vec2 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

uniform vec2 uRectPos;
uniform vec2 uRectSize;

uniform sampler2D uTexture;

out vec2 fTexCoord;

void main()
{
    fTexCoord = aPosition * vec2(0.5, -0.5) + 0.5;
    gl_Position = vec4(uRectPos + aPosition * uRectSize, 0, 1);
}