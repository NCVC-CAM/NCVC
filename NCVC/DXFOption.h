// DXFOption.h: DXFｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

enum {
	DXFOPT_VIEW = 0,
	DXFOPT_FILECOMMENT,
		DXFOPT_FLGS			// [2]
};
enum {
	DXFOPT_ORGTYPE = 0,
	DXFOPT_BINDORG,
	DXFOPT_BINDSORT,
		DXFOPT_NUMS			// [3]
};
enum {
	DXFOPT_BINDWORKX = 0,
	DXFOPT_BINDWORKY,
	DXFOPT_BINDMARGIN,
		DXFOPT_DBL_NUMS		// [3]
};
enum	enMAKETYPE	{
	NCMAKEMILL = 0,		// MC
	NCMAKELATHE,		// 旋盤
	NCMAKEWIRE,			// ﾜｲﾔ放電加工機
	NCMAKELAYER,		// ﾚｲﾔ名と条件ﾌｧｲﾙの関係ﾌｧｲﾙの履歴
		NCMAKENUM			// [4]
};

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;
friend	class	CCADbindDlg;
friend	class	CMakeBindOptDlg;

	union {
		struct {
			BOOL	m_bView,		// 変換後ﾋﾞｭｰ起動
					m_bFileComment;	// 生成時ﾌｧｲﾙごとにｺﾒﾝﾄを埋め込む
		};
		BOOL		m_ubNums[DXFOPT_FLGS];
	};
	union {
		struct {
			int		m_nOrgType,		// 原点ﾚｲﾔが無いときの処理
									//    0:ｴﾗｰ,  1～4:右上,右下,左上,左下, 5:中央
					m_nBindOrg,		// CADﾃﾞｰﾀ統合時の加工原点
									//    0～3:右上,右下,左上,左下, 4:中央
					m_nBindSort;	// 生成時の並べ替え
		};
		int			m_unNums[DXFOPT_NUMS];
	};
	union {
		struct {
			float	m_dBindWork[2],	// CADﾃﾞｰﾀの統合ﾜｰｸｻｲｽﾞ
					m_dBindMargin;	// 配置ﾏｰｼﾞﾝ
		};
		float		m_udNums[DXFOPT_DBL_NUMS];
	};
	CString	m_strReadLayer[DXFLAYERSIZE];	// 原点，切削(入力ｲﾒｰｼﾞ保存用)，
											// 加工開始位置ﾚｲﾔ名, 強制移動指示ﾚｲﾔ名, ｺﾒﾝﾄ用
	CStringList	m_strInitList[NCMAKENUM];	// 切削条件ﾌｧｲﾙ名の履歴
	enMAKETYPE	m_enMakeType;				// 直前のNC生成ﾀｲﾌﾟ
	boost::regex	m_regCutter;			// 切削ﾚｲﾔ正規表現

	BOOL	AddListHistory(enMAKETYPE, LPCTSTR);
	BOOL	ReadInitHistory(enMAKETYPE);
	BOOL	SaveInitHistory(enMAKETYPE);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);		// ﾚｼﾞｽﾄﾘへの保存
	BOOL	SaveBindOption(void);

	BOOL	IsOriginLayer(LPCTSTR lpszLayer) const {
		return (m_strReadLayer[DXFORGLAYER] == lpszLayer);
	}
	BOOL	IsCutterLayer(LPCTSTR lpszLayer) const {
		return boost::regex_search(lpszLayer, m_regCutter);
	}
	BOOL	IsStartLayer(LPCTSTR lpszLayer) const {
		return m_strReadLayer[DXFSTRLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFSTRLAYER] == lpszLayer);
	}
	BOOL	IsMoveLayer(LPCTSTR lpszLayer) const {
		return m_strReadLayer[DXFMOVLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFMOVLAYER] == lpszLayer);
	}
	BOOL	IsCommentLayer(LPCTSTR lpszLayer) const {
		return m_strReadLayer[DXFCOMLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFCOMLAYER] == lpszLayer);
	}
	//
	BOOL	GetDxfOptFlg(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_ubNums) );
		return m_ubNums[n];
	}
	int		GetDxfOptNum(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	void	SetViewFlag(BOOL bView) {
		m_bView = bView;
	}
	enMAKETYPE	GetNCMakeType(void) const {
		return m_enMakeType;
	}
	const	CStringList*	GetInitList(enMAKETYPE enType) const {
		return &m_strInitList[enType];
	}

	BOOL	AddInitHistory(enMAKETYPE enType, LPCTSTR lpszSearch) {
		if ( AddListHistory(enType, lpszSearch) )
			return SaveInitHistory(enType);
		return FALSE;
	}
	void	DelInitHistory(enMAKETYPE, LPCTSTR);
	//
	float	GetBindSize(size_t n) const {
		return m_dBindWork[n];
	}
	float	GetBindMargin(void) const {
		return m_dBindMargin;
	}
	//
	// from DXFMakeOption.cpp, NCVCaddinDXF.cpp
	CString	GetReadLayer(size_t n) const {
		ASSERT( n>=0 && n<DXFLAYERSIZE );
		return m_strReadLayer[n];
	}
};
