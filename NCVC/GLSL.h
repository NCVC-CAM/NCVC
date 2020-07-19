// GLSL.h : CGLSL クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#define	USE_SHADER

enum GLSLShaderType
{
	VERTEX, FRAGMENT, GEOMETRY,
	TESS_CONTROL, TESS_EVALUATION
};

class CGLSL
{
	GLuint	m_handle;
	BOOL	m_bLink;
	CString	m_strError;

	GLint	GetUniformLocation(LPCTSTR);

public:
	CGLSL();
	~CGLSL();

	BOOL	CompileShaderFromFile(LPCTSTR, GLSLShaderType);
	BOOL	CompileShaderFromFile(const CString&, GLSLShaderType);
	BOOL	Link(void);
	void	Use(BOOL = TRUE);

	void	SetUniform(LPCTSTR, GLfloat);
	void	SetUniform(LPCTSTR, GLint);

	CString	GetErrorStr(void);
};
