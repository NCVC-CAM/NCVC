uniform float L;
uniform float R;
uniform float T;
uniform float B;

void main(void)
{
	// depth texture
//	gl_TexCoord[0] = gl_Vertex;
//	gl_TexCoord[0] = vec4(gl_Vertex.xy * 2.0 - 1.0, 0.0, 1.0);
	float X = (gl_Vertex.x - L) / (R - L);
	float Y = (gl_Vertex.y - B) / (T - B);
	gl_TexCoord[0] = vec4(X,Y,0.0,1.0);

//	gl_Position = vec4(gl_Vertex.xy * 2.0 - 1.0, 0.0, 1.0);
//	gl_Position = gl_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

//	gl_FrontColor = gl_Color;
}
