// NCMakeWireOpt.h: ﾜｲﾔ放電加工機用NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {
	MKWI_NUM_PROG = 0,			// ﾌﾟﾛｸﾞﾗﾑ番号
	MKWI_NUM_LINEADD,			// 行番号増加
	MKWI_NUM_G90,				// 位置指定(G90 or G91)
	MKWI_NUM_DOT,				// 数値表記(小数3位 or 小数4位 or 1/1000)
	MKWI_NUM_FDOT,				// Ｆﾊﾟﾗﾒｰﾀの数値表記
	MKWI_NUM_CIRCLECODE,		// 円切削(G2 or G3)
		MKWI_NUM_NUMS		// [6]
};
enum {
	MKWI_DBL_DEPTH = 0,			// ﾜｰｸ厚み
	MKWI_DBL_TAPER,				// ﾃｰﾊﾟ角度[deg]
	MKWI_DBL_FEED,				// 切削送り
	MKWI_DBL_G92X,				// G92
	MKWI_DBL_G92Y,
	MKWI_DBL_AWFCIRCLE_LO,		// AWF結線対象円
	MKWI_DBL_AWFCIRCLE_HI,
	MKWI_DBL_ELLIPSE,			// 楕円公差
		MKWI_DBL_NUMS		// [8]
};
enum {
	MKWI_FLG_PROG = 0,			// O番号付与
	MKWI_FLG_PROGAUTO,			// ﾗﾝﾀﾞﾑ割り当て
	MKWI_FLG_LINEADD,			// 行番号
	MKWI_FLG_ZEROCUT,			// 小数点以下のｾﾞﾛｶｯﾄ
	MKWI_FLG_GCLIP,				// Gｺｰﾄﾞ省略形
	MKWI_FLG_ELLIPSE,			// 長径と短径が等しい楕円は円とみなす
	MKWI_FLG_AWFSTART,			// 加工前結線
	MKWI_FLG_AWFEND,			// 加工後切断
		MKWI_FLG_NUMS		// [8]
};
enum {
	MKWI_STR_LINEFORM = 0,		// 行番号ﾌｫｰﾏｯﾄ
	MKWI_STR_EOB,				// EOB
	MKWI_STR_HEADER,			// ｶｽﾀﾑﾍｯﾀﾞｰ
	MKWI_STR_FOOTER,			// ｶｽﾀﾑﾌｯﾀｰ
	MKWI_STR_TAPERMODE,			// TaperMode
	MKWI_STR_AWFCNT,			// AWF結線ｺｰﾄﾞ
	MKWI_STR_AWFCUT,			// AWF切断ｺｰﾄﾞ
		MKWI_STR_NUMS		// [7]
};
//
#define	WIR_I_PROG			m_pIntOpt[MKWI_NUM_PROG]
#define	WIR_I_LINEADD		m_pIntOpt[MKWI_NUM_LINEADD]
#define	WIR_I_G90			m_pIntOpt[MKWI_NUM_G90]
#define	WIR_I_DOT			m_pIntOpt[MKWI_NUM_DOT]
#define	WIR_I_FDOT			m_pIntOpt[MKWI_NUM_FDOT]
#define	WIR_I_CIRCLECODE	m_pIntOpt[MKWI_NUM_CIRCLECODE]
//
#define	WIR_D_DEPTH			m_pDblOpt[MKWI_DBL_DEPTH]
#define	WIR_D_TAPER			m_pDblOpt[MKWI_DBL_TAPER]
#define	WIR_D_FEED			m_pDblOpt[MKWI_DBL_FEED]
#define	WIR_D_G92X			m_pDblOpt[MKWI_DBL_G92X]
#define	WIR_D_G92Y			m_pDblOpt[MKWI_DBL_G92Y]
#define	WIR_D_AWFCIRCLE_LO	m_pDblOpt[MKWI_DBL_AWFCIRCLE_LO]
#define	WIR_D_AWFCIRCLE_HI	m_pDblOpt[MKWI_DBL_AWFCIRCLE_HI]
#define	WIR_D_ELLIPSE		m_pDblOpt[MKWI_DBL_ELLIPSE]
//
#define	WIR_F_PROG			m_pFlgOpt[MKWI_FLG_PROG]
#define	WIR_F_PROGAUTO		m_pFlgOpt[MKWI_FLG_PROGAUTO]
#define	WIR_F_LINEADD		m_pFlgOpt[MKWI_FLG_LINEADD]
#define	WIR_F_ZEROCUT		m_pFlgOpt[MKWI_FLG_ZEROCUT]
#define	WIR_F_GCLIP			m_pFlgOpt[MKWI_FLG_GCLIP]
#define	WIR_F_ELLIPSE		m_pFlgOpt[MKWI_FLG_ELLIPSE]
#define	WIR_F_AWFSTART		m_pFlgOpt[MKWI_FLG_AWFSTART]
#define	WIR_F_AWFEND		m_pFlgOpt[MKWI_FLG_AWFEND]
//
#define	WIR_S_LINEFORM		m_strOption[MKWI_STR_LINEFORM]
#define	WIR_S_EOB			m_strOption[MKWI_STR_EOB]
#define	WIR_S_HEADER		m_strOption[MKWI_STR_HEADER]
#define	WIR_S_FOOTER		m_strOption[MKWI_STR_FOOTER]
#define	WIR_S_TAPERMODE		m_strOption[MKWI_STR_TAPERMODE]
#define	WIR_S_AWFCNT		m_strOption[MKWI_STR_AWFCNT]
#define	WIR_S_AWFCUT		m_strOption[MKWI_STR_AWFCUT]
//
class CNCMakeWireOpt : public CNCMakeOption
{
	friend class CMakeWireSetup1;
	friend class CMakeWireSetup2;
	friend class CMakeNCSetup2;
	friend class CMakeNCSetup6;

	// 親ｸﾗｽにﾃﾞｰﾀを持たせるので、
	// union/struct技は使えない
	// むしろC++らしくｺｰﾃﾞｨﾝｸﾞ

protected:
	virtual	void	InitialDefault(void);
	virtual	BOOL	IsPathID(int);

public:
	CNCMakeWireOpt(LPCTSTR);

	BOOL	IsAWFcircle(float r) {
		return WIR_D_AWFCIRCLE_LO<=r && r<=WIR_D_AWFCIRCLE_HI;
	}
	virtual	CString	GetLineNoForm(void) const;

#ifdef _DEBUG
	virtual	void	DbgDump(void) const;	// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
