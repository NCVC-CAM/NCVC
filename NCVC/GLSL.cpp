// GLSL.cpp : ŽÀ‘•ƒtƒ@ƒCƒ‹
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GLSL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////

CGLSL::CGLSL()
{
	m_handle = 0;
	m_bLink = FALSE;
}

CGLSL::~CGLSL()
{
	if ( m_handle )
		::glDeleteProgram(m_handle);
}

GLint CGLSL::GetUniformLocation(LPCTSTR lpszName)
{
    return ::glGetUniformLocation(m_handle, lpszName);
}

BOOL CGLSL::CompileShaderFromFile(LPCTSTR lpszFile, GLSLShaderType type)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
	CString	strTmp, strBuf;

	try {
		CStdioFile	fp(lpszFile,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		// “Ç‚Ýž‚ÝÙ°Ìß
		while ( fp.ReadString(strTmp) )
			strBuf += strTmp + gg_szReturn;
	}
	catch (CFileException* e) {
		e->Delete();
		return FALSE;
	}

	return CompileShaderFromFile(strBuf, type);
}

BOOL CGLSL::CompileShaderFromFile(const CString& strSource, GLSLShaderType type)
{
	if ( m_handle == 0 ) {
		m_handle = ::glCreateProgram();
		if ( m_handle == 0 )
			return FALSE;
	}

	GLuint	shaderHandle = 0;
	switch ( type ) {
	case VERTEX:
		shaderHandle = ::glCreateShader(GL_VERTEX_SHADER);
		break;
	case FRAGMENT:
		shaderHandle = ::glCreateShader(GL_FRAGMENT_SHADER);
		break;
	case GEOMETRY:
		shaderHandle = ::glCreateShader(GL_GEOMETRY_SHADER);
		break;
	case TESS_CONTROL:
		shaderHandle = ::glCreateShader(GL_TESS_CONTROL_SHADER);
		break;
	case TESS_EVALUATION:
		shaderHandle = ::glCreateShader(GL_TESS_EVALUATION_SHADER);
		break;
	default:
		return FALSE;
	}
	if ( shaderHandle == 0 )
		return FALSE;

	LPCTSTR	lpszSource = strSource.operator LPCSTR();
	::glShaderSource(shaderHandle, 1, &lpszSource, NULL);

	::glCompileShader(shaderHandle);

	GLint	nResult;
	::glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &nResult);
	if( nResult == GL_FALSE ) {
		int len = 0;
		::glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &len);
		if( len > 0 ) {
			char* c_log = new char[len];
			int written = 0;
			::glGetShaderInfoLog(shaderHandle, len, &written, c_log);
			m_strError = c_log;
			delete [] c_log;
		}
		return FALSE;
	}

	::glAttachShader(m_handle, shaderHandle);
	::glDeleteShader(shaderHandle);

	return TRUE;
}

BOOL CGLSL::Link(void)
{
	if ( m_handle==0 || m_bLink )
		return FALSE;

	::glLinkProgram(m_handle);

	GLint	nResult;
	::glGetProgramiv(m_handle, GL_LINK_STATUS, &nResult);
	if ( nResult == GL_FALSE ) {
		int len = 0;
		::glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &len);
		if( len > 0 ) {
			char* c_log = new char[len];
			int written = 0;
			::glGetProgramInfoLog(m_handle, len, &written, c_log);
			m_strError = c_log;
			delete [] c_log;
		}
		return FALSE;
	}

	m_bLink = TRUE;
	return TRUE;
}

void CGLSL::Use(BOOL used)
{
	if ( used ) {
		if ( m_handle>0 && m_bLink )
			::glUseProgram(m_handle);
	}
	else
		::glUseProgram(0);
}

void CGLSL::SetUniform(LPCTSTR lpszName, GLfloat val)
{
	GLint	loc = GetUniformLocation(lpszName);
	if ( loc >= 0 )
		::glUniform1f(loc, val);
}

void CGLSL::SetUniform(LPCTSTR lpszName, GLint val)
{
	GLint	loc = GetUniformLocation(lpszName);
	if ( loc >= 0 )
		::glUniform1i(loc, val);
}

CString CGLSL::GetErrorStr(void)
{
	return m_strError;
}
