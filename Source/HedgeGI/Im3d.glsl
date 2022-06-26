#version 330

#define VertexData \
	_VertexData \
	{ \
		noperspective float edgeDistance; \
		noperspective float size; \
		smooth vec4 color; \
		vec4 position; \
	}

#define ANTI_ALIASING 2.0

#ifdef VERTEX_SHADER
	layout(location = 0) in vec4 aPositionAndSize;
	layout(location = 1) in vec4 aColor;

	uniform mat4 uView;
	uniform mat4 uProjection;
	
	out VertexData fData;
	
	void main() 
	{
		fData.color = aColor.abgr;

		#if !defined(TRIANGLES)
			fData.color.a *= smoothstep(0.0, 1.0, aPositionAndSize.w / ANTI_ALIASING);
		#endif

		fData.size = max(aPositionAndSize.w, ANTI_ALIASING);

		gl_Position = uProjection * (uView * vec4(aPositionAndSize.xyz, 1.0));
		fData.position = gl_Position;

		#if defined(POINTS)
			gl_PointSize = fData.size;
		#endif
	}
#endif

#ifdef GEOMETRY_SHADER
	layout(lines) in;
	layout(triangle_strip, max_vertices = 4) out;
	
	uniform vec2 uViewport;
	
	in VertexData fData[];
	out VertexData fDataOut;
	
	void main() 
	{
		vec2 pos0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
		vec2 pos1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
		
		vec2 dir = pos0 - pos1;
		dir = normalize(vec2(dir.x, dir.y * uViewport.y / uViewport.x));
		vec2 tng0 = vec2(-dir.y, dir.x);
		vec2 tng1 = tng0 * fData[1].size / uViewport;
		tng0 = tng0 * fData[0].size / uViewport;
		
		gl_Position = vec4((pos0 - tng0) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw); 
		fDataOut.edgeDistance = -fData[0].size;
		fDataOut.size = fData[0].size;
		fDataOut.color = fData[0].color;
		fDataOut.position = gl_Position;
		EmitVertex();
		
		gl_Position = vec4((pos0 + tng0) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
		fDataOut.color = fData[0].color;
		fDataOut.edgeDistance = fData[0].size;
		fDataOut.size = fData[0].size;
		fDataOut.position = gl_Position;
		EmitVertex();
		
		gl_Position = vec4((pos1 - tng1) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
		fDataOut.edgeDistance = -fData[1].size;
		fDataOut.size = fData[1].size;
		fDataOut.color = fData[1].color;
		fDataOut.position = gl_Position;
		EmitVertex();
		
		gl_Position = vec4((pos1 + tng1) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
		fDataOut.color = fData[1].color;
		fDataOut.size = fData[1].size;
		fDataOut.edgeDistance = fData[1].size;
		fDataOut.position = gl_Position;
		EmitVertex();
	}
#endif

#ifdef FRAGMENT_SHADER
	in VertexData fData;

	out vec4 oColor;

	uniform vec4 uRect;
	uniform sampler2D uTexture;

	uniform float uDiscardFactor;
		
	void main() 
	{
		oColor = fData.color;
		
		#if defined(LINES)
			float d = abs(fData.edgeDistance) / fData.size;
			d = smoothstep(1.0, 1.0 - (ANTI_ALIASING / fData.size), d);
			oColor.a *= d;
			
		#elif defined(POINTS)
			float d = length(gl_PointCoord.xy - vec2(0.5));
			d = smoothstep(0.5, 0.5 - (ANTI_ALIASING / fData.size), d);
			oColor.a *= d;
			
		#endif		

		vec3 texCoord = fData.position.xyz / fData.position.w;
		texCoord.xy = texCoord.xy * 0.5 + 0.5;

		float cmpDepth = texture(uTexture, texCoord.xy * uRect.zw + uRect.xy).w;
		oColor = oColor * (cmpDepth < texCoord.z ? uDiscardFactor : 1.0);
	}
#endif

