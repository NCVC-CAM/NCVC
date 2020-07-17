// ｶｽﾀﾑﾍｯﾀﾞｰ, ﾌｯﾀﾞｰ処理の基底ｸﾗｽ
//	from TH_MakeNCD.cpp TH_MakeLathe.cpp
//////////////////////////////////////////////////////////////////////

#pragma once

class	CDXFDoc;
class	CDXFdata;
class	CNCMakeOption;

class CMakeCustomCode
{
	const CDXFDoc*			m_pDoc;
	const CNCMakeOption*	m_pMakeOpt;

	BOOL	IsNCchar(LPCTSTR) const;

protected:
	CStringKeyIndex	m_strOrderIndex;	// ｵｰﾀﾞｰ文字列
	const CDXFdata*	m_pData;			// 派生ｸﾗｽから参照
	CString&		m_strResult;		// 置換結果(参照型)

public:
	CMakeCustomCode(CString&,
		const CDXFDoc*, const CDXFdata*, const CNCMakeOption*);

	int		ReplaceCustomCode(const char*, const char*) const;
};
