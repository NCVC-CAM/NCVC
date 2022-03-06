// FrameBuffer.h : CFrameBuffer �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

//#define	_DEPTH_TEXTURE_		// �����I�Ƀf�v�X�l���V�F�[�_�ň�����悤�ɂȂ����Ƃ���

enum FBOTYPE
{
	FRAMEBUFFER, RENDERBUFFER_COLOR, RENDERBUFFER_DEPTH,
#ifdef _DEPTH_TEXTURE_
	TEXTUREBUFFER
#endif
};

class CFrameBuffer
{
	static	GLuint	ms_uBind;	// ����bind����Ă���FB

	GLsizei	m_w, m_h;
	GLuint	m_fb;
	GLuint	m_rbColor, m_rbDepth;
#ifdef _DEPTH_TEXTURE_
	GLuint	m_tb;
#endif

public:
	CFrameBuffer(GLsizei, GLsizei);
	~CFrameBuffer();

	BOOL	Create(GLsizei, GLsizei);
	void	Delete(void);
	BOOL	Bind(BOOL);
	BOOL	IsBind(void) const {
		return ms_uBind > 0;
	}

	GLuint	GetBufferID(FBOTYPE e) {
		switch ( e ) {
		case FRAMEBUFFER:			return m_fb;
		case RENDERBUFFER_COLOR:	return m_rbColor;
		case RENDERBUFFER_DEPTH:	return m_rbDepth;
#ifdef _DEPTH_TEXTURE_
		case TEXTUREBUFFER:			return m_tb;
#endif
		}
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
GLenum	DbgGetGLError(char*, UINT);
#define	GetGLError()	DbgGetGLError(__FILE__, __LINE__)
#else
inline GLenum GetGLError(void)
{
	return ::glGetError();
}
#endif
