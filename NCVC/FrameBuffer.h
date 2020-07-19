// FrameBuffer.h : CFrameBuffer クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

enum FBOTYPE
{
	FRAMEBUFFER, RANDERBUFFER, TEXTUREBUFFER
};

class CFrameBuffer
{
	static	GLuint	ms_uBind;	// 現在bindされているFB
	GLuint	m_fb, m_rb, m_tb;
	GLsizei	m_w, m_h;

public:
	CFrameBuffer();
	CFrameBuffer(GLsizei, GLsizei, BOOL = FALSE);
	~CFrameBuffer();

	BOOL	Create(GLsizei, GLsizei, BOOL = FALSE);
	void	Delete(void);
	BOOL	Bind(BOOL);
	BOOL	IsBind(void) const {
		return ms_uBind > 0;
	}

	GLuint	GetBufferID(FBOTYPE e) {
		switch ( e ) {
		case FRAMEBUFFER:	return m_fb;
		case RANDERBUFFER:	return m_rb;
		case TEXTUREBUFFER:	return m_tb;
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
