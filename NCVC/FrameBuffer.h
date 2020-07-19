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
	BOOL	m_bBind;
	GLuint	m_fb, m_rb, m_tb;
	GLsizei	m_w, m_h;

public:
	CFrameBuffer();
	CFrameBuffer(GLsizei, GLsizei, BOOL = FALSE);
	~CFrameBuffer();

	BOOL	Create(GLsizei, GLsizei, BOOL = FALSE);
	void	Delete(void);
	void	Bind(BOOL);
	BOOL	IsBind(void) const {
		return m_bBind;
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
