// DXFOption.h: DXFｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

/*
	DXFOPT_VIEW		: 変換後ﾋﾞｭｰ起動
	DXFOPT_ORGTYPE	: 原点ﾚｲﾔが無いときの処理
							0:ｴﾗｰ, 1～4:右上,右下,左上,左下, 5:中央
*/
#define	DXFOPT_VIEW		0
#define	DXFOPT_ORGTYPE	1

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;

	CString	m_strReadLayer[DXFLAYERSIZE];	// 原点，切削(入力ｲﾒｰｼﾞ保存用)，
											// 加工開始位置ﾚｲﾔ名, 強制移動指示ﾚｲﾔ名, ｺﾒﾝﾄ用
	boost::regex	m_regCutter;			// 切削ﾚｲﾔ正規表現
	int			m_nDXF[2];		// View, OrgType
	CStringList	m_strInitList;	// 切削条件ﾌｧｲﾙ名の履歴
	CStringList	m_strLayerToInitList;	// ﾚｲﾔ名と条件ﾌｧｲﾙの関係ﾌｧｲﾙの履歴

	BOOL	AddListHistory(CStringList&, LPCTSTR);
	void	DelListHistory(CStringList&, LPCTSTR);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);	// ﾚｼﾞｽﾄﾘへの保存(下記含む)
	BOOL	SaveInitHistory(void);	// 条件ﾌｧｲﾙの履歴だけ保存

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

	int		GetDxfFlag(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_nDXF) );
		return m_nDXF[n];
	}
	void	SetViewFlag(BOOL bView) {
		m_nDXF[DXFOPT_VIEW] = bView;
	}

	const	CStringList*	GetInitList(void) const {
		return &m_strInitList;
	}
	const	CStringList*	GetLayerToInitList(void) const {
		return &m_strLayerToInitList;
	}
	BOOL	AddInitHistory(LPCTSTR lpszSearch) {
		return AddListHistory(m_strInitList, lpszSearch);
	}
	BOOL	AddLayerHistory(LPCTSTR lpszSearch) {
		return AddListHistory(m_strLayerToInitList, lpszSearch);
	}
	void	DelInitHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strInitList, lpszSearch);
	}
	void	DelLayerHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strLayerToInitList, lpszSearch);
	}

	// from DXFMakeOption.cpp, NCVCaddinDXF.cpp
	CString	GetReadLayer(size_t n) const {
		ASSERT( n>=0 && n<DXFLAYERSIZE );
		return m_strReadLayer[n];
	}
};
