// FrameBuffer.cpp : 実装ファイル
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FrameBuffer.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

//#define	_DEPTH_TEXTURE_
// test  GL_TEXTURE_2D -> GL_TEXTURE_RECTANGLE

/////////////////////////////////////////////////////////////////////////////

CFrameBuffer::CFrameBuffer()
{
	m_bBind = FALSE;
	m_fb = m_rb = m_tb = 0;
}

CFrameBuffer::CFrameBuffer(GLsizei w, GLsizei h, BOOL bBindKeep)
{
	m_fb = m_rb = m_tb = 0;
	Create(w, h, bBindKeep);
}

CFrameBuffer::~CFrameBuffer()
{
	Delete();
}

BOOL CFrameBuffer::Create(GLsizei w, GLsizei h, BOOL bBindKeep)
{
	GLenum	glError;

	m_w = w;	m_h = h;

#ifdef _DEPTH_TEXTURE_
	// Texture
	::glGenTextures(1, &m_tb);
	::glBindTexture(GL_TEXTURE_2D, m_tb);
	::glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
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
	glError = ::glGetError();
	if ( glError != GL_NO_ERROR ) {
#ifdef _DEBUG
		g_dbg.printf( "CFrameBuffer::Create() RBO error=%d", glError);
#endif
		return FALSE;
	}

	// FBO作成
	::glGenFramebuffersEXT(1, &m_fb);
	::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fb);
#ifdef _DEBUG
	glError = ::glGetError();
	g_dbg.printf( "CFrameBuffer::Create() FBO error=%d", glError);
#endif

	// FBOにアタッチ
#ifdef _DEPTH_TEXTURE_
	::glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_tb, 0);
#else
	::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, m_rb);
#endif
	glError = ::glGetError();
	if ( glError != GL_NO_ERROR ) {
#ifdef _DEBUG
		g_dbg.printf( "CFrameBuffer::Create() attach error=%d", glError);
#endif
		return FALSE;
	}

	m_bBind = TRUE;
	Bind(bBindKeep);

	return TRUE;
}

void CFrameBuffer::Delete(void)
{
	Bind(FALSE);
	if ( m_tb )
		::glDeleteTextures(1, &m_tb);
	if ( m_rb )
		::glDeleteRenderbuffersEXT(1, &m_rb);
	if ( m_fb )
		::glDeleteFramebuffersEXT(1, &m_fb);
	m_fb = m_rb = m_tb = 0;
}

void CFrameBuffer::Bind(BOOL bind)
{
	::glGetError();		// error flash
	if ( bind ) {
		if ( m_fb ) {
			if ( !m_bBind ) {
				m_bBind = TRUE;
				::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fb);
			}
			::glViewport(0, 0, m_w, m_h);
		}
	}
	else if ( m_fb && m_bBind ) {
		m_bBind = FALSE;
		::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
#ifdef _DEBUG
	GLenum	glError = ::glGetError();
	g_dbg.printf( "CFrameBuffer::Bind(%d) error=%d", bind, glError);
#endif
}
