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
enum	eMAKETYPE	{NCMAKEMILL, NCMAKELATHE};

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;

	CString	m_strReadLayer[DXFLAYERSIZE];	// 原点，切削(入力ｲﾒｰｼﾞ保存用)，
											// 加工開始位置ﾚｲﾔ名, 強制移動指示ﾚｲﾔ名, ｺﾒﾝﾄ用
	boost::regex	m_regCutter;			// 切削ﾚｲﾔ正規表現
	int			m_nDXF[2];			// View, OrgType
	CStringList	m_strMillList,			// 切削条件ﾌｧｲﾙ名の履歴
				m_strLatheList,			// 旋盤用切削条件ﾌｧｲﾙ名の履歴
				m_strLayerToInitList;	// ﾚｲﾔ名と条件ﾌｧｲﾙの関係ﾌｧｲﾙの履歴
	eMAKETYPE	m_enMakeType;		// 直前のNC生成ﾀｲﾌﾟ

	BOOL	AddListHistory(CStringList&, LPCTSTR);
	void	DelListHistory(CStringList&, LPCTSTR);
	BOOL	ReadInitHistory(eMAKETYPE);
	BOOL	SaveInitHistory(eMAKETYPE);
	BOOL	SaveLayerHistory(void);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);		// ﾚｼﾞｽﾄﾘへの保存(下記含む)

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
	eMAKETYPE	GetNCMakeType(void) const {
		return m_enMakeType;
	}

	const	CStringList*	GetMillInitList(void) const {
		return &m_strMillList;
	}
	const	CStringList*	GetLatheInitList(void) const {
		return &m_strLatheList;
	}
	const	CStringList*	GetLayerToInitList(void) const {
		return &m_strLayerToInitList;
	}
	BOOL	AddMillInitHistory(LPCTSTR lpszSearch) {
		if ( AddListHistory(m_strMillList, lpszSearch) )
			return SaveInitHistory(NCMAKEMILL);
		return FALSE;
	}
	BOOL	AddLatheInitHistory(LPCTSTR lpszSearch) {
		if ( AddListHistory(m_strLatheList, lpszSearch) )
			return SaveInitHistory(NCMAKELATHE);
		return FALSE;
	}
	BOOL	AddLayerHistory(LPCTSTR lpszSearch) {
		if ( AddListHistory(m_strLayerToInitList, lpszSearch) )
			return SaveLayerHistory();
		return FALSE;
	}
	void	DelMillInitHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strMillList, lpszSearch);
	}
	void	DelLatheInitHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strLatheList, lpszSearch);
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
