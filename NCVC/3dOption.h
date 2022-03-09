// 3dOption.h: 3次元モデル切削のｵﾌﾟｼｮﾝ管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

enum {		// int型
	D3_INT_LINESPLIT,
		D3_INT_NUMS		// [1]
};
enum {		// float型
	D3_DBL_BALLENDMILL = 0,
	D3_DBL_HEIGHT,
	D3_DBL_ZCUT,
		D3_DBL_NUMS		// [3]
};

class C3dOption
{
friend	class	C3dScanSetupDlg;

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
			float	m_dBallEndmill,		// ボールエンドミル半径
					m_dWorkHeight,		// ワークの高さ
					m_dZCut;			// 1回の切り込み量
		};
		float		m_udNums[D3_DBL_NUMS];
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
};
