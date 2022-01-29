#version 330

in vec2 fTexCoord;

uniform vec4 uColor;
uniform sampler2D uTexture;

out vec4 oColor;

void main()
{
    oColor = uColor * texture(uTexture, fTexCoord);
}
