// ViewOption.h: •\Ž¦ŒnµÌß¼®Ý‚ÌŠÇ—
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

typedef	struct	tagPENSTYLE {
	LPCTSTR	lpszPenName;	// ÀÞ²±Û¸Þ(ºÝÎÞÎÞ¯¸½)“o˜^—p–¼Ì
	int		nPenType;		// CPen¸×½‚Ö‚ÌÍßÝ½À²Ù
	// --- ˆÈ‰ºDXF—p
	LPCTSTR	lpszDXFname;	// üŽí–¼
	LPCTSTR	lpszDXFpattern;	// üŽíÊßÀ°Ý(º°ÄÞ3)
	int		nDXFdash;		// ÀÞ¯¼­‚Ì’·‚³€–Ú”(º°ÄÞ73==º°ÄÞ49‚Ì”)
	float	dDXFpattern;	// ÊßÀ°Ý‘S‘Ì’·‚³(º°ÄÞ40)
	float	dDXFdash[6];	// ÀÞ¯¼­’·‚³(º°ÄÞ49)
	// --- ˆÈ‰ºOpenGL—p
	GLushort	nGLpattern;	// for glLineStipple
} PENSTYLE;
#define	MAXPENSTYLE		5

#define	VIEWUPDATE_REDRAW			0x0001
#define	VIEWUPDATE_DISPLAYLIST		0x0002
#define	VIEWUPDATE_LIGHT			0x0004
#define	VIEWUPDATE_BOXEL			0x0008
#define	VIEWUPDATE_TEXTURE			0x0010
#define	VIEWUPDATE_ALL				0x001f

#define	COMCOL_RECT			0
#define	COMCOL_SELECT		1
#define	NCCOL_BACKGROUND1	0
#define	NCCOL_BACKGROUND2	1
#define	NCCOL_PANE			2
#define	NCCOL_GUIDEX		3
#define	NCCOL_GUIDEY		4
#define	NCCOL_GUIDEZ		5
#define	NCCOL_G0			6
#define	NCCOL_G1			7
#define	NCCOL_G1Z			8
#define	NCCOL_CYCLE			9
#define	NCCOL_CENTERCIRCLE	10
#define	NCCOL_WORK			11
#define	NCCOL_MAXCUT		12
#define	NCCOL_CORRECT		13
#define	NCCOL_GL_WRK		14
#define	NCCOL_GL_CUT		15
#define	NCCOLLINE_G0		3
#define	NCCOLLINE_G1		4
#define	NCCOLLINE_G1Z		5
#define	NCCOLLINE_CYCLE		6
#define	NCCOLLINE_WORK		7
#define	NCCOLLINE_MAXCUT	8
#define	NCINFOCOL_BACKGROUND1	0
#define	NCINFOCOL_BACKGROUND2	1
#define	NCINFOCOL_TEXT			2
#define	NCVIEWFLG_TRACEMARKER		0
#define	NCVIEWFLG_DRAWCIRCLECENTER	1
#define	NCVIEWFLG_GUIDESCALE		2
#define	NCVIEWFLG_GUIDELENGTH		3
#define	NCVIEWFLG_SOLIDVIEW			4
#define	NCVIEWFLG_G00VIEW			5
#define	NCVIEWFLG_DRAGRENDER		6
#define	NCVIEWFLG_TEXTURE			7
#define	DXFCOL_BACKGROUND1	0
#define	DXFCOL_BACKGROUND2	1
#define	DXFCOL_ORIGIN		2
#define	DXFCOL_CUTTER		3
#define	DXFCOL_START		4
#define	DXFCOL_MOVE			5
#define	DXFCOL_TEXT			6
#define	DXFCOL_WORKER		7

class CViewOption
{
friend	class	CViewSetup1;
friend	class	CViewSetup2;
friend	class	CViewSetup3;
friend	class	CViewSetup4;
friend	class	CViewSetup5;
friend	class	CNCViewGL;		// OpenGL»Îß°Äó‹µ‚É‚æ‚Á‚ÄÌ×¸Þ‚ð‹­§OFF

	DWORD	m_dwUpdateFlg;		// ViewSetup‚É‚æ‚é’¼‘O‚ÌXVó‹µ

	BOOL	m_bMouseWheel;				// Šg‘åk¬‚ÉÏ³½Î²°Ù‚ðŽg‚¤‚©
	union {
		struct {
			BOOL	m_bTraceMarker,		// ÄÚ°½’†‚ÌŒ»ÝˆÊ’u•\Ž¦
					m_bDrawCircleCenter,// ‰~ŒÊ•âŠÔ‚Ì’†S‚ð•`‰æ
					m_bScale,			// TRUE:¶Þ²ÄÞ‚É–Ú·
					m_bGuide,			// TRUE:Šg‘å—¦‚É“¯Šú
					m_bSolidView,		// OpenGL¿Ø¯ÄÞ•\Ž¦
					m_bG00View,			// G00ˆÚ“®•\Ž¦
					m_bDragRender,		// ÄÞ×¯¸Þ’†‚àÚÝÀÞØÝ¸Þ
					m_bTexture;			// Ã¸½Á¬‚Ì“\‚è•t‚¯
		};
		BOOL		m_bNCFlag[8];
	};
	COLORREF	m_colView[2],			// ËÞ­°‚ÌŠeF
				m_colNCView[16],
				m_colNCInfoView[3],
				m_colDXFView[8],
				m_colCustom[16];
	int			m_nLineType[2],			// üŽí
				m_nNCLineType[9],
				m_nDXFLineType[5],
				m_nWheelType,			// 0->Žè‘O:Šg‘å,‰œ:k¬ 1->‹t
				m_nTraceSpeed[3],		// 0:‚‘¬, 1:’†‘¬, 2:’á‘¬
				m_nMillType;			// 0:½¸³ª±, 1:ÎÞ°Ù
	double		m_dGuide[NCXYZ],		// ¶Þ²ÄÞŽ²‚Ì’·‚³
				m_dDefaultEndmill;		// ÃÞÌ«ÙÄ´ÝÄÞÐÙŒa(”¼Œa)
	LOGFONT		m_lfFont[2];			// NC/DXF‚ÅŽg—p‚·‚éÌ«ÝÄî•ñ
	CString		m_strTexture;			// Ã¸½Á¬‰æ‘œÌ§²Ù

	void	AllDefaultSetting(void);

public:
	CViewOption();
	BOOL		SaveViewOption(void);	// Ú¼Þ½ÄØ‚Ö‚Ì•Û‘¶

	BOOL		IsMouseWheel(void) const {
		return m_bMouseWheel;
	}
	int			GetWheelType(void) const {
		return m_nWheelType;
	}
	BOOL		GetNCViewFlg(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_bNCFlag) );
		return m_bNCFlag[a];
	}
	int			GetTraceSpeed(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nTraceSpeed) );
		return m_nTraceSpeed[a];
	}
	COLORREF	GetDrawColor(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_colView) );
		return m_colView[a];
	}
	COLORREF	GetNcDrawColor(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_colNCView) );
		return m_colNCView[a];
	}
	COLORREF	GetNcInfoDrawColor(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_colNCInfoView) );
		return m_colNCInfoView[a];
	}
	COLORREF	GetDxfDrawColor(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_colDXFView) );
		return m_colDXFView[a];
	}
	COLORREF*	GetCustomColor(void) {
		return m_colCustom;
	}
	int			GetDrawType(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nLineType) );
		return m_nLineType[a];
	}
	int			GetNcDrawType(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nNCLineType) );
		return m_nNCLineType[a];
	}
	int			GetDxfDrawType(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nDXFLineType) );
		return m_nDXFLineType[a];
	}
	double	GetGuideLength(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_dGuide) );
		return m_dGuide[a];
	}
	double	GetDefaultEndmill(void) const {
		return m_dDefaultEndmill;
	}
	int		GetDefaultEndmillType(void) const {
		return m_nMillType;
	}
	const	LPLOGFONT	GetLogFont(DOCTYPE enType) {
		return &m_lfFont[enType];
	}
	CString	GetTextureFile(void) const {
		return m_strTexture;
	}

	// ´¸½Îß°ÄC²ÝÎß°Ä
	BOOL	Export(LPCTSTR);
	void	Inport(LPCTSTR);
};

COLORREF	ConvertSTRtoRGB(LPCTSTR);	// •¶Žš—ñ‚ðFî•ñ‚É•ÏŠ·
CString		ConvertRGBtoSTR(COLORREF);	// ‚»‚Ì‹t
