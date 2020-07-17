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
} PENSTYLE;
#define	MAXPENSTYLE		5

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
#define	NCCOLLINE_G0		3
#define	NCCOLLINE_G1		4
#define	NCCOLLINE_G1Z		5
#define	NCCOLLINE_CYCLE		6
#define	NCCOLLINE_WORK		7
#define	NCCOLLINE_MAXCUT	8
#define	NCINFOCOL_BACKGROUND1	0
#define	NCINFOCOL_BACKGROUND2	1
#define	NCINFOCOL_TEXT			2
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

	BOOL		m_bMouseWheel,			// Šg‘åk¬‚ÉÏ³½Î²°Ù‚ðŽg‚¤‚©
				m_bTraceMarker,			// ÄÚ°½’†‚ÌŒ»ÝˆÊ’u•\Ž¦
				m_bDrawCircleCenter,	// ‰~ŒÊ•âŠÔ‚Ì’†S‚ð•`‰æ
				m_bGuide;				// TRUE:Šg‘å—¦‚É“¯Šú
	COLORREF	m_colView[2],			// ËÞ­°‚ÌŠeF
				m_colNCView[14],
				m_colNCInfoView[3],
				m_colDXFView[8],
				m_colCustom[16];
	int			m_nLineType[2],			// üŽí
				m_nNCLineType[9],
				m_nDXFLineType[5];
	int			m_nWheelType;			// 0->Žè‘O:Šg‘å,‰œ:k¬ 1->‹t
	int			m_nTraceSpeed[3];		// 0:‚‘¬, 1:’†‘¬, 2:’á‘¬
	double		m_dGuide[NCXYZ];		// ¶Þ²ÄÞŽ²‚Ì’·‚³
	LOGFONT		m_lfFont[2];			// NC/DXF‚ÅŽg—p‚·‚éÌ«ÝÄî•ñ

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
	BOOL		IsTraceMarker(void) const {
		return m_bTraceMarker;
	}
	int			GetTraceSpeed(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nTraceSpeed) );
		return m_nTraceSpeed[a];
	}
	BOOL		IsDrawCircleCenter(void) const {
		return m_bDrawCircleCenter;
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
	BOOL	IsGuideSync(void) const {
		return m_bGuide;
	}
	double	GetGuideLength(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_dGuide) );
		return m_dGuide[a];
	}
	const	LPLOGFONT	GetLogFont(DOCTYPE enType) {
		return &m_lfFont[enType];
	}

	// ´¸½Îß°ÄC²ÝÎß°Ä
	BOOL	Export(LPCTSTR);
	void	Inport(LPCTSTR);
};

COLORREF	ConvertSTRtoRGB(LPCTSTR);	// •¶Žš—ñ‚ðFî•ñ‚É•ÏŠ·
CString		ConvertRGBtoSTR(COLORREF);	// ‚»‚Ì‹t
