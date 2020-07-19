// FrameBuffer.h : CFrameBuffer クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

enum FBOTYPE
{
	FRAMEBUFFER, RANDERDEPTH, RENDERSTENCIL, TEXTUREBUFFER
};

class CFrameBuffer
{
	BOOL	m_bBind;
	GLuint	m_fb, m_db, m_sb, m_tb;
	GLsizei	m_w, m_h;

public:
	CFrameBuffer();
	CFrameBuffer(GLsizei, GLsizei, BOOL = FALSE);
	~CFrameBuffer();

	void	Create(GLsizei, GLsizei, BOOL = FALSE);
	void	Delete(void);
	void	Bind(BOOL);

	GLuint	GetBufferID(FBOTYPE e) {
		switch ( e ) {
		case FRAMEBUFFER:	return m_fb;
		case RANDERDEPTH:	return m_db;
		case RENDERSTENCIL:	return m_sb;
		case TEXTUREBUFFER:	return m_tb;
		}
		return 0;
	}
};
