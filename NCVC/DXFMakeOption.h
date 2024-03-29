// DXFMakeOption.h: DXF出力ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

enum {
	MKDX_NUM_LTYPE_O = 0,
	MKDX_NUM_LTYPE_C,
	MKDX_NUM_LTYPE_M,
	MKDX_NUM_LTYPE_H,
	MKDX_NUM_LCOL_O,
	MKDX_NUM_LCOL_C,
	MKDX_NUM_LCOL_M,
	MKDX_NUM_LCOL_H,
	MKDX_NUM_PLANE,
	MKDX_NUM_CYCLE,
		MKDX_NUM_NUMS		// [10]
};
enum {
	MKDX_DBL_ORGLENGTH = 0,
	MKDX_DBL_CYCLER,
		MKDX_DBL_NUMS		// [2]
};
enum {
	MKDX_FLG_OUT_O = 0,
	MKDX_FLG_OUT_C,
	MKDX_FLG_OUT_M,
	MKDX_FLG_OUT_H,
	MKDX_FLG_ORGCIRCLE,
	MKDX_FLG_ORGCROSS,
		MKDX_FLG_NUMS		// [6]
};
enum {
	MKDX_STR_ORIGIN = 0,
	MKDX_STR_CAMLINE,
	MKDX_STR_MOVE,
	MKDX_STR_CORRECT,
		MKDX_STR_NUMS		// [4]
};

class CDXFMakeOption  
{
friend class CMakeDXFDlg1;
friend class CMakeDXFDlg2;

	// int型ｵﾌﾟｼｮﾝ
	union {
		struct {
			int		m_nLType[4],	// 各ﾚｲﾔの線種
					m_nLColor[4],	// 　〃 の色
					m_nPlane,		// 平面指定
					m_nCycle;		// 固定ｻｲｸﾙ出力ﾀｲﾌﾟ
		};
		int			m_unNums[MKDX_NUM_NUMS];
	};
	// float型ｵﾌﾟｼｮﾝ
	union {
		struct {
			float	m_dOrgLength,	// 原点長さ(径)
					m_dCycleR;		// 固定ｻｲｸﾙ円出力の径
		};
		float		m_udNums[MKDX_DBL_NUMS];
	};
	// BOOL型ｵﾌﾟｼｮﾝ
	union {
		struct {
			BOOL	m_bOut[4],		// 各ﾚｲﾔの出力ﾌﾗｸﾞ
					m_bOrgCircle,	// 原点円出力
					m_bOrgCross;	// 原点十字出力
		};
		BOOL		m_ubFlags[MKDX_FLG_NUMS];
	};
	// CString型ｵﾌﾟｼｮﾝ
	CString		m_strOption[MKDX_STR_NUMS];	// 各種ﾚｲﾔ

	//
	void	Initialize_Registry(void);
	void	Initialize_Default(void);

public:
	CDXFMakeOption(BOOL bRegist = TRUE);
	BOOL	SaveDXFMakeOption(void);		// ﾚｼﾞｽﾄﾘへの保存

	int		GetNum(size_t n) const {		// 数字ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	float	GetDbl(size_t n) const {		// 数字ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<SIZEOF(m_udNums) );
		return m_udNums[n];
	}
	BOOL	GetFlag(size_t n) const {		// ﾌﾗｸﾞｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<SIZEOF(m_ubFlags) );
		return m_ubFlags[n];
	}
	CString	GetStr(size_t n) const {		// 文字列ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<SIZEOF(m_strOption) );
		return m_strOption[n];
	}
};
