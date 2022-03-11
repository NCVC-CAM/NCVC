// 3dOption.h: 3次元モデル切削のｵﾌﾟｼｮﾝ管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

enum {		// int型
	D3_INT_LINESPLIT,
		D3_INT_NUMS		// [1]
};
enum {		// float型
	D3_DBL_ROUGH_BALLENDMILL = 0,
	D3_DBL_WORKHEIGHT,
	D3_DBL_ROUGH_ZCUT,
	D3_DBL_CONTOUR_BALLENDMILL,
	D3_DBL_CONTOUR_SPACE,
	D3_DBL_CONTOUR_ZMIN,
	D3_DBL_CONTOUR_ZMAX,
	D3_DBL_CONTOUR_SHIFT,
		D3_DBL_NUMS		// [8]
};
enum {		// BOOL型
	D3_FLG_ROUGH_ZORIGIN = 0,
	D3_FLG_CONTOUR_ZORIGIN,
		D3_FLG_NUMS		// [2]
};

class C3dOption
{
friend	class	C3dRoughScanSetupDlg;
friend	class	C3dContourScanSetupDlg;

	CString		m_str3dOptionFile;

	// int型ｵﾌﾟｼｮﾝ
	union {
		struct {
			int		m_nLineSplit;			// スキャニングライン分割数
		};
		int			m_unNums[D3_INT_NUMS];
	};
	// float型ｵﾌﾟｼｮﾝ
	union {
		struct {
			float	m_dRoughBallEndmill,	// 荒加工用ボールエンドミル半径
					m_dWorkHeight,			// ワークの高さ
					m_dRoughZCut,			// 荒加工の切り込み量
					m_dContourBallEndmill,	// 等高線用ボールエンドミル半径
					m_dContourSpace,		// 等高線の点間隔
					m_dContourZmin,			// 等高線をスキャンするZ値
					m_dContourZmax,
					m_dContourShift;		// 等高線の平面シフト量
		};
		float		m_udNums[D3_DBL_NUMS];
	};
	// BOOL型ｵﾌﾟｼｮﾝ
	union {
		struct {
			int		m_bRoughZOrigin,		// ワーク上面をZ軸原点にする
					m_bContourZOrigin;
		};
		int			m_ubFlgs[D3_FLG_NUMS];
	};

public:
	C3dOption();
	~C3dOption();
	BOOL	Read3dOption(LPCTSTR);
	BOOL	Save3dOption(void);

	// CNCMakeOption と関数名が被ると TH_MakeXXX系の #define でバッティングする
	int		Get3dInt(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	float	Get3dDbl(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_udNums) );
		return m_udNums[n];
	}
	BOOL	Get3dFlg(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_ubFlgs) );
		return m_ubFlgs[n];
	}
};
