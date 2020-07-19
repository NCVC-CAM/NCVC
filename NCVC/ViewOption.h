// ViewOption.h: •\Ž¦ŒnµÌß¼®Ý‚ÌŠÇ—
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

struct	PENSTYLE
{
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
};
#define	MAXPENSTYLE		5

#define	VIEWUPDATE_REDRAW			0x0001
#define	VIEWUPDATE_DISPLAYLIST		0x0002
#define	VIEWUPDATE_LIGHT			0x0004
#define	VIEWUPDATE_BOXEL			0x0008
#define	VIEWUPDATE_TEXTURE			0x0010
#define	VIEWUPDATE_ALL				0x001f

enum {
	NCVIEWFLG_TRACEMARKER = 0,
	NCVIEWFLG_DRAWREVISE,
	NCVIEWFLG_DRAWCIRCLECENTER,
	NCVIEWFLG_GUIDESCALE,
	NCVIEWFLG_GUIDELENGTH,
	NCVIEWFLG_SOLIDVIEW,
	NCVIEWFLG_WIREPATH,
	NCVIEWFLG_DRAGRENDER,
	NCVIEWFLG_TEXTURE,
	NCVIEWFLG_LATHESLIT,
	NCVIEWFLG_NOACTIVETRACEGL,
	NCVIEWFLG_TOOLTRACE,
		NCVIEWFLG_NUMS		// [12]
};
enum {
	COMCOL_RECT = 0,
	COMCOL_SELECT,
		COMCOL_NUMS			// [2]
};
enum {
	NCCOL_BACKGROUND1 = 0,
	NCCOL_BACKGROUND2,
	NCCOL_PANE,
	NCCOL_GUIDEX, NCCOL_GUIDEY, NCCOL_GUIDEZ,
	NCCOL_G0, NCCOL_G1, NCCOL_G1Z,
	NCCOL_CYCLE,
	NCCOL_CENTERCIRCLE,
	NCCOL_WORK,
	NCCOL_MAXCUT,
	NCCOL_CORRECT,
	NCCOL_GL_WRK,
	NCCOL_GL_CUT,
	NCCOL_GL_MILL,
		NCCOL_NUMS			// [17]
};
enum {
	NCINFOCOL_BACKGROUND1 = 0,
	NCINFOCOL_BACKGROUND2,
	NCINFOCOL_TEXT,
		NCINFOCOL_NUMS		// [3]
};
enum {
	DXFCOL_BACKGROUND1 = 0,
	DXFCOL_BACKGROUND2,
	DXFCOL_ORIGIN,
	DXFCOL_CUTTER,
	DXFCOL_START,
	DXFCOL_MOVE,
	DXFCOL_TEXT,
	DXFCOL_OUTLINE,
	DXFCOL_WORK,
		DXFCOL_NUMS			// [9]
};
enum {
	// NCCOLLINE_GUIDE_[X|Y|Z] ‚Í NCA_[X|Y|Z] ‚Å‘ã—p
	NCCOLLINE_G0 = NCXYZ,
	NCCOLLINE_G1,
	NCCOLLINE_G1Z,
	NCCOLLINE_CYCLE,
	NCCOLLINE_WORK,
	NCCOLLINE_MAXCUT,
		NCCOLLINE_NUMS		// [9]
};
const size_t	DXFCOLLINE_NUMS = 6;

class CViewOption
{
friend	class	CViewSetup1;
friend	class	CViewSetup2;
friend	class	CViewSetup3;
friend	class	CViewSetup4;
friend	class	CViewSetup5;
friend	class	CViewSetup6;
friend	class	CNCViewGL;		// OpenGL»Îß°Äó‹µ‚É‚æ‚Á‚ÄÌ×¸Þ‚ð‹­§OFF

	DWORD	m_dwUpdateFlg;		// ViewSetup‚É‚æ‚é’¼‘O‚ÌXVó‹µ

	BOOL	m_bMouseWheel;				// Šg‘åk¬‚ÉÏ³½Î²°Ù‚ðŽg‚¤‚©
	union {
		struct {
			BOOL	m_bTraceMarker,		// ÄÚ°½’†‚ÌŒ»ÝˆÊ’u•\Ž¦
					m_bDrawRevise,		// •â³‘OÃÞ°À‚Ì•`‰æ
					m_bDrawCircleCenter,// ‰~ŒÊ•âŠÔ‚Ì’†S‚ð•`‰æ
					m_bScale,			// TRUE:¶Þ²ÄÞ‚É–Ú·
					m_bGuide,			// TRUE:Šg‘å—¦‚É“¯Šú
					m_bSolidView,		// OpenGL¿Ø¯ÄÞ•\Ž¦
					m_bWirePath,		// Ü²ÔÊß½•\Ž¦
					m_bDragRender,		// ÄÞ×¯¸Þ’†‚àÚÝÀÞØÝ¸Þ
					m_bTexture,			// Ã¸½Á¬‚Ì“\‚è•t‚¯
					m_bLatheSlit,		// ù”Õ‚Ì’f–Ê•\Ž¦
					m_bNoActiveTraceGL,	// ”ñ±¸Ã¨ÌÞ‚Å‚àÄÚ°½
					m_bToolTrace;		// ÄÚ°½’†‚ÉH‹ï‚ð•\Ž¦
		};
		BOOL		m_bNCFlag[NCVIEWFLG_NUMS];
	};
	COLORREF	m_colView[COMCOL_NUMS],	// ËÞ­°‚ÌŠeF
				m_colNCView[NCCOL_NUMS],
				m_colNCInfoView[NCINFOCOL_NUMS],
				m_colDXFView[DXFCOL_NUMS],
				m_colCustom[16];		// F‚ÌÝ’èÀÞ²±Û¸Þ(CColorDialog)
	int			m_nLineType[2],			// üŽí
				m_nNCLineType[NCCOLLINE_NUMS],
				m_nDXFLineType[DXFCOLLINE_NUMS],
				m_nWheelType,			// 0->Žè‘O:Šg‘å,‰œ:k¬ 1->‹t
				m_nTraceSpeed[3],		// 0:‚‘¬, 1:’†‘¬, 2:’á‘¬
				m_nMillType,			// 0:½¸³ª±, 1:ÎÞ°Ù, 2:–ÊŽæ‚è
				m_nForceView01[4],		// 4–Ê-1:¶ã,‰Eã,¶‰º,‰E‰º
				m_nForceView02[4];		// 4–Ê-2:¶ã,¶’†,¶‰º,‰E
	float		m_dGuide[NCXYZ],		// ¶Þ²ÄÞŽ²‚Ì’·‚³
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
	float	GetGuideLength(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_dGuide) );
		return m_dGuide[a];
	}
	float	GetDefaultEndmill(void) const {
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
	int		GetForceView01(int n) const {
		return m_nForceView01[n]>=0 && m_nForceView01[n]<SIZEOF(m_nForceView01) ?
			m_nForceView01[n] : 0;
	}
	int		GetForceView02(int n) const {
		return m_nForceView02[n]>=0 && m_nForceView02[n]<SIZEOF(m_nForceView02) ?
			m_nForceView02[n] : 0;
	}

	// ´¸½Îß°ÄC²ÝÎß°Ä
	BOOL	Export(LPCTSTR);
	void	Inport(LPCTSTR);
};
