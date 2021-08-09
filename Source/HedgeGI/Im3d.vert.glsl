#version 330

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec4 fColor;

void main() 
{
	fColor = aColor.abgr;
	gl_Position = uProjection * (uView * vec4(aPosition.xyz, 1.0));
}