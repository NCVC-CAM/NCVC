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
	m_fb = m_db = m_sb = m_tb = 0;
}

CFrameBuffer::CFrameBuffer(GLsizei w, GLsizei h, BOOL bBindKeep)
{
	m_fb = m_db = m_sb = m_tb = 0;
	Create(w, h, bBindKeep);
}

CFrameBuffer::~CFrameBuffer()
{
	Bind(FALSE);
	Delete();
}

void CFrameBuffer::Create(GLsizei w, GLsizei h, BOOL bBindKeep)
{
#ifdef _DEBUG
	GLenum	glError;
#endif
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
#endif
	// Depth/Stencil RBO作成
	::glGenRenderbuffersEXT(1, &m_db);
	::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_db);
//	::glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8, w, h);
	::glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, w, h);
	::glGenRenderbuffersEXT(1, &m_sb);
	::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_sb);
	::glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX, w, h);
	::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
#ifdef _DEBUG
	glError = ::glGetError();
	g_dbg.printf( "CFrameBuffer::Create() RBO error=%d", glError);
#endif
	// FBO作成
	::glGenFramebuffersEXT(1, &m_fb);
	::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fb);
	m_bBind = TRUE;
#ifdef _DEBUG
	glError = ::glGetError();
	g_dbg.printf( "CFrameBuffer::Create() FBO error=%d", glError);
#endif

	// FBOにアタッチ
#ifdef _DEPTH_TEXTURE_
	::glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_tb, 0);
#endif
//	::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, m_rb);
	::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,   GL_RENDERBUFFER_EXT, m_db);
	::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_sb);
#ifdef _DEBUG
	glError = ::glGetError();
	g_dbg.printf( "CFrameBuffer::Create() attach error=%d", glError);
#endif

//	if ( !bBindKeep )
//		Bind(FALSE);
	Bind(bBindKeep);
}

void CFrameBuffer::Delete(void)
{
	if ( m_tb )
		::glDeleteTextures(1, &m_tb);
	if ( m_db )
		::glDeleteRenderbuffersEXT(1, &m_db);
	if ( m_sb )
		::glDeleteRenderbuffersEXT(1, &m_sb);
	if ( m_fb )
		::glDeleteFramebuffersEXT(1, &m_fb);
	m_fb = m_db = m_sb = m_tb = 0;
}

void CFrameBuffer::Bind(BOOL bind)
{
	::glGetError();		// error flash
	if ( bind ) {
		if ( m_fb && !m_bBind ) {
			m_bBind = TRUE;
			::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fb);
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
