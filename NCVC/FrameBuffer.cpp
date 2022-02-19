// FrameBuffer.cpp : 実装ファイル
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FrameBuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#define	_DEPTH_TEXTURE_

GLuint	CFrameBuffer::ms_uBind = 0;

/////////////////////////////////////////////////////////////////////////////

CFrameBuffer::CFrameBuffer(GLsizei w, GLsizei h)
{
	m_fb = m_rb = m_tb = 0;
	Create(w, h);
}

CFrameBuffer::~CFrameBuffer()
{
	Delete();
}

BOOL CFrameBuffer::Create(GLsizei w, GLsizei h)
{
	m_w = w;	m_h = h;

	GetGLError();		// error flash
#ifdef _DEPTH_TEXTURE_
	// Texture
	::glGenTextures(1, &m_tb);
	::glBindTexture(GL_TEXTURE_2D, m_tb);
	::glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
//	::glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_NONE);	// 廃止？？
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	::glBindTexture(GL_TEXTURE_2D, 0);
#else
	// Depth/Stencil RBO作成
	::glGenRenderbuffersEXT(1, &m_rb);
	::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_rb);
	::glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8, w, h);
	::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
#endif
	if ( GetGLError() != GL_NO_ERROR )
		return FALSE;

	// FBO作成
	::glGenFramebuffersEXT(1, &m_fb);
	::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fb);
	if ( GetGLError() != GL_NO_ERROR )
		return FALSE;

	// FBOにアタッチ
#ifdef _DEPTH_TEXTURE_
	::glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_tb, 0);
#else
	::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, m_rb);
#endif
	if ( GetGLError() != GL_NO_ERROR )
		return FALSE;

	Bind(TRUE);

	return TRUE;
}

void CFrameBuffer::Delete(void)
{
	if (ms_uBind!=0 && ms_uBind==m_fb )
		Bind(FALSE);
	if ( m_tb )
		::glDeleteTextures(1, &m_tb);
	if ( m_rb )
		::glDeleteRenderbuffersEXT(1, &m_rb);
	if ( m_fb )
		::glDeleteFramebuffersEXT(1, &m_fb);
	m_fb = m_rb = m_tb = 0;
}

BOOL CFrameBuffer::Bind(BOOL bind)
{
	BOOL	bResult = TRUE;

	GetGLError();		// error flash
	if ( bind ) {
		if ( m_fb ) {
			if ( ms_uBind != m_fb ) {
				ms_uBind = m_fb;
				::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fb);
			}
			::glViewport(0, 0, m_w, m_h);
		}
		else
			bResult = FALSE;
	}
	else if ( ms_uBind > 0 ) {
		ms_uBind = 0;
		::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
#ifdef _DEBUG
	GetGLError();
#endif

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
GLenum DbgGetGLError(char* szFile, UINT nLine)
{
	GLenum glError = ::glGetError();

	if ( glError != GL_NO_ERROR ) {
		switch(glError) {
		case GL_INVALID_ENUM:
			printf("GL_INVALID_ENUM File=%s, line=%d\n", szFile, nLine);
			break;
		case GL_INVALID_VALUE:
			printf("GL_INVALID_VALUE File=%s, line=%d\n", szFile, nLine);
			break;
		case GL_INVALID_OPERATION:
			printf("GL_INVALID_OPERATION File=%s, line=%d\n", szFile, nLine);
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			printf("GL_INVALID_FRAMEBUFFER_OPERATION File=%s, line=%d\n", szFile, nLine);
			break;
		case GL_OUT_OF_MEMORY:
			printf("GL_OUT_OF_MEMORY File=%s, line=%d\n", szFile, nLine);
			break;
		default:
			printf("GL_??? File=%s, line=%d\n", szFile, nLine);
			break;
		}
	}

	return glError;
}
#endif
