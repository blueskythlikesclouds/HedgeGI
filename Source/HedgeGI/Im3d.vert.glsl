#version 330

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec4 fColor;
out vec4 fPosition;

void main() 
{
	vec4 viewPos = uView * vec4(aPosition.xyz, 1.0);

	fColor = aColor.abgr;
	fColor.a = mix(fColor.a, fColor.a * 0.5, clamp((viewPos.z + 500.0) / 500.0, 0, 1));

	gl_Position = uProjection * viewPos;
	fPosition = gl_Position;
}