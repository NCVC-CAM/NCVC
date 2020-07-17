// ¶½ÀÑÍ¯ÀŞ°, Ì¯ÀŞ°ˆ—‚ÌŠî’ê¸×½
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
	CStringKeyIndex	m_strOrderIndex;	// µ°ÀŞ°•¶š—ñ
	const CDXFdata*	m_pData;			// ”h¶¸×½‚©‚çQÆ
	CString&		m_strResult;		// ’uŠ·Œ‹‰Ê(QÆŒ^)

public:
	CMakeCustomCode(CString&,
		const CDXFDoc*, const CDXFdata*, const CNCMakeOption*);

	int		ReplaceCustomCode(const char*, const char*) const;
};
