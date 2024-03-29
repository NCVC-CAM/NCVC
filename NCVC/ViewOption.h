// ViewOption.h: 表示系ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

struct	PENSTYLE
{
	LPCTSTR	lpszPenName;	// ﾀﾞｲｱﾛｸﾞ(ｺﾝﾎﾞﾎﾞｯｸｽ)登録用名称
	int		nPenType;		// CPenｸﾗｽへのﾍﾟﾝｽﾀｲﾙ
	// --- 以下DXF用
	LPCTSTR	lpszDXFname;	// 線種名
	LPCTSTR	lpszDXFpattern;	// 線種ﾊﾟﾀｰﾝ(ｺｰﾄﾞ3)
	int		nDXFdash;		// ﾀﾞｯｼｭの長さ項目数(ｺｰﾄﾞ73==ｺｰﾄﾞ49の数)
	float	dDXFpattern;	// ﾊﾟﾀｰﾝ全体長さ(ｺｰﾄﾞ40)
	float	dDXFdash[6];	// ﾀﾞｯｼｭ長さ(ｺｰﾄﾞ49)
	// --- 以下OpenGL用
	GLushort	nGLpattern;	// for glLineStipple
};
#define	MAXPENSTYLE		5

#define	VIEWUPDATE_REDRAW			0x0001
#define	VIEWUPDATE_LIGHT			0x0002
#define	VIEWUPDATE_BOXEL			0x0004
#define	VIEWUPDATE_TEXTURE			0x0008
#define	VIEWUPDATE_ALL				0x000f

enum {
	GLOPTFLG_DRAWREVISE = 0,
	GLOPTFLG_DRAWCIRCLECENTER,
	GLOPTFLG_GUIDESCALE,
	GLOPTFLG_GUIDELENGTH,
	GLOPTFLG_SOLIDVIEW,
	GLOPTFLG_USEFBO,
	GLOPTFLG_WIREPATH,
	GLOPTFLG_DRAGRENDER,
	GLOPTFLG_TEXTURE,
	GLOPTFLG_LATHESLIT,
	GLOPTFLG_NOACTIVETRACEGL,
	GLOPTFLG_TOOLTRACE,
		GLOPTFLG_NUMS		// [12]
};
enum {
	VIEWINT_MILLTYPE = 0,
	VIEWINT_FOURVIEW01,
	VIEWINT_FOURVIEW02
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
	// NCCOLLINE_GUIDE_[X|Y|Z] は NCA_[X|Y|Z] で代用
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
friend	class	CNCViewGL;		// OpenGLｻﾎﾟｰﾄ状況によってﾌﾗｸﾞを強制OFF

	DWORD	m_dwUpdateFlg;		// ViewSetupによる直前の更新状況

	BOOL	m_bMouseWheel;				// 拡大縮小にﾏｳｽﾎｲｰﾙを使うか
	union {
		struct {
			BOOL	m_bDrawRevise,		// 補正前ﾃﾞｰﾀの描画
					m_bDrawCircleCenter,// 円弧補間の中心を描画
					m_bScale,			// TRUE:ｶﾞｲﾄﾞに目盛
					m_bGuide,			// TRUE:拡大率に同期
					m_bSolidView,		// OpenGLｿﾘｯﾄﾞ表示
					m_bUseFBO,			// FBOを使用
					m_bWirePath,		// ﾜｲﾔﾊﾟｽ表示
					m_bDragRender,		// ﾄﾞﾗｯｸﾞ中もﾚﾝﾀﾞﾘﾝｸﾞ
					m_bTexture,			// ﾃｸｽﾁｬの貼り付け
					m_bLatheSlit,		// 旋盤の断面表示
					m_bNoActiveTraceGL,	// 非ｱｸﾃｨﾌﾞでもﾄﾚｰｽ
					m_bToolTrace;		// ﾄﾚｰｽ中に工具を表示
		};
		BOOL		m_bNCFlag[GLOPTFLG_NUMS];
	};
	union {
		struct {
			int		m_nLineType[COMCOL_NUMS],
					m_nNCLineType[NCCOLLINE_NUMS],
					m_nDXFLineType[DXFCOLLINE_NUMS],
					m_nWheelType,			// 0->手前:拡大,奥:縮小 1->逆
					m_nTraceSpeed[3],		// 0:高速, 1:中速, 2:低速
					m_nMillType,			// 0:ｽｸｳｪｱ, 1:ﾎﾞｰﾙ, 2:面取り
					m_nForceView01[4],		// 4面-1:左上,右上,左下,右下
					m_nForceView02[4];		// 4面-2:左上,左中,左下,右
		};
		int		m_nViewOpt[30];		// 修正中途半端...
	};
	COLORREF	m_colView[COMCOL_NUMS],	// ﾋﾞｭｰの各色
				m_colNCView[NCCOL_NUMS],
				m_colNCInfoView[NCINFOCOL_NUMS],
				m_colDXFView[DXFCOL_NUMS],
				m_colCustom[16];		// 色の設定ﾀﾞｲｱﾛｸﾞ(CColorDialog)
	float		m_dGuide[NCXYZ],		// ｶﾞｲﾄﾞ軸の長さ
				m_dDefaultEndmill;		// ﾃﾞﾌｫﾙﾄｴﾝﾄﾞﾐﾙ径(半径)
	LOGFONT		m_lfFont[2];			// NC/DXFで使用するﾌｫﾝﾄ情報
	CString		m_strTexture;			// ﾃｸｽﾁｬ画像ﾌｧｲﾙ

	void	AllDefaultSetting(void);

public:
	CViewOption();
	BOOL		SaveViewOption(void);	// ﾚｼﾞｽﾄﾘへの保存

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

	// ｴｸｽﾎﾟｰﾄ，ｲﾝﾎﾟｰﾄ
	BOOL	Export(LPCTSTR);
	void	Inport(LPCTSTR);
};
