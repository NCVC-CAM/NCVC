// DXFOption.h: DXFｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#if !defined(_DXFOPTION__INCLUDED_)
#define _DXFOPTION__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NCVCdefine.h"

/*
	DXFOPT_REGEX	: 従来通り(0)か，正規表現で(1)
	DXFOPT_MATCH	: 完全一致(0)か，部分一致(1)
	DXFOPT_ACCEPT	: 対象ﾚｲﾔを認識する(0)か，除外するか(1)
	DXFOPT_VIEW		: 変換後ﾋﾞｭｰ起動
	DXFOPT_ORGTYPE	: 原点ﾚｲﾔが無いときの処理
							0:ｴﾗｰ, 1～4:右上,右下,左上,左下, 5:中央
*/
#define	DXFOPT_REGEX	0
#define	DXFOPT_MATCH	1
#define	DXFOPT_ACCEPT	2
#define	DXFOPT_VIEW		3
#define	DXFOPT_ORGTYPE	4

class	CDXFOption;
// 切削ﾚｲﾔ名の認識ﾊﾟﾀｰﾝ関数のﾌﾟﾛﾄﾀｲﾌﾟ
typedef BOOL (*PFNISCUTTERLAYER)(const CDXFOption*, LPCTSTR);

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;

	CString	m_strReadLayer[DXFLAYERSIZE],	// 原点，切削(入力ｲﾒｰｼﾞ保存用)，
											// 加工開始位置ﾚｲﾔ名, 強制移動指示ﾚｲﾔ名,
											// ｺﾒﾝﾄ用
			m_strCutterFirst;	// 複数ﾚｲﾔの１つ目(CMapStringToObはSeqｱｸｾｽ不可)
	CMapStringToOb	m_strCutterMap;	// 複数ﾚｲﾔ管理用
	int			m_nDXF[5];		// Regex, Match, Accept, View, OrgType
	CStringList	m_strInitList;	// 切削条件ﾌｧｲﾙ名の履歴
	CStringList	m_strLayerToInitList;	// ﾚｲﾔ名と条件ﾌｧｲﾙの関係ﾌｧｲﾙの履歴

	void	SetCutterList(void);
	BOOL	AddListHistory(CStringList&, LPCTSTR);
	void	DelListHistory(CStringList&, LPCTSTR);

	// 切削ﾚｲﾔ名の認識ﾊﾟﾀｰﾝ
	PFNISCUTTERLAYER	m_pfnIsCutterLayer;
	void	SetCallingLayerFunction(void);
	static	BOOL	IsAllMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsPerfectMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsPartMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsPerfectNotMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsPartNotMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsRegex(const CDXFOption*, LPCTSTR);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);	// ﾚｼﾞｽﾄﾘへの保存(下記含む)
	BOOL	SaveInitHistory(void);	// 条件ﾌｧｲﾙの履歴だけ保存

	BOOL	IsOriginLayer(LPCTSTR lpszOrigin) const {
		return (m_strReadLayer[DXFORGLAYER] == lpszOrigin);
	}
	BOOL	IsCutterLayer(LPCTSTR lpszCutter) const {
		ASSERT( m_pfnIsCutterLayer );
		return (*m_pfnIsCutterLayer)(this, lpszCutter);
	}
	BOOL	IsStartLayer(LPCTSTR lpszStart) const {
		return m_strReadLayer[DXFSTRLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFSTRLAYER] == lpszStart);
	}
	BOOL	IsMoveLayer(LPCTSTR lpszMove) const {
		return m_strReadLayer[DXFMOVLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFMOVLAYER] == lpszMove);
	}
	BOOL	IsCommentLayer(LPCTSTR lpszComment) const {
		return m_strReadLayer[DXFCOMLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFCOMLAYER] == lpszComment);
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
	CString	GetCutterFirst(void) const {
		return m_strCutterFirst;
	}
};

#endif
