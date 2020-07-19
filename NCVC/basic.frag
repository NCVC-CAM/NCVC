/*
#version 400

in vec3 Color;

out vec4 FragColor;

void main()
{
	FragColor = vec4(Color, 1.0);
}
*/
uniform sampler2D depth;
void main()
{
//	gl_FragColor = texture2D(depth, gl_TexCoord[0].xy);
	gl_FragColor = gl_Color;
}
