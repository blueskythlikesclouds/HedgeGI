#version 330

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec4 fColor;

void main() 
{
	vec4 viewPos = uView * vec4(aPosition.xyz, 1.0);

	fColor = aColor.abgr;
	fColor.rgb = mix(fColor.rgb, fColor.rgb * 0.5, clamp((viewPos.z + 500.0) / 500.0, 0, 1));

	gl_Position = uProjection * viewPos;
}