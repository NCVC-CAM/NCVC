uniform sampler2D depth;

void main(void)
{
	gl_FragColor = texture2D(depth, gl_TexCoord[0].xy);
//	gl_FragColor = gl_Color;

}
