/*
#version 400

in vec3 VertexPosition;
in vec3 VertexColor;

out vec3 Color;

void main()
{
	Color = VertexColor;
	gl_Position = vec4(VertexPosition, 1.0);
}
*/

precision mediump float;

uniform sampler2D depth;
uniform float N;
uniform float F;
uniform float L;
uniform float R;
uniform float T;
uniform float B;

//uniform sampler2DRect depth;

void main()
{
	float X = (gl_Vertex.x - L) / (R - L);
	float Y = (gl_Vertex.y - B) / (T - B);
//	float X = gl_Vertex.x - L;	// sampler2DRect
//	float Y = gl_Vertex.y - B;
//	gl_TexCoord[0] = vec2(X,Y);

//	float D = texture2D(depth, gl_MultiTexCoord1.xy);
//	float D = texture2D(depth, gl_Vertex.xy);
	float D = texture2D(depth, vec2(X,Y));	// 8x3=24bit
//	float D = texture2DRect(depth, vec2(X,Y));
//	float D = texture2D(depth, vec2(0.0,0.0)).xyz;
//	float D = 0.495912;

	float Z  = D * (F - N) + N;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xy, Z, 1.0);
//	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xy, D, 1.0);
//	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	gl_FrontColor = gl_Color;
//	gl_FrontColor = texture2D(depth, gl_Vertex.xy);
}
