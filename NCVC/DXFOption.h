// DXFOption.h: DXFµÌß¼®ÝÌÇ
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

/*
	DXFOPT_VIEW		: Ï·ãËÞ­°N®
	DXFOPT_ORGTYPE	: ´_Ú²Ôª³¢Æ«Ì
							0:´×°, 1`4:Eã,Eº,¶ã,¶º, 5:
*/
#define	DXFOPT_VIEW		0
#define	DXFOPT_ORGTYPE	1
enum	enMAKETYPE	{NCMAKEMILL, NCMAKELATHE, NCMAKEWIRE};

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;

	CString	m_strReadLayer[DXFLAYERSIZE];	// ´_CØí(üÍ²Ò°¼ÞÛ¶p)C
											// ÁHJnÊuÚ²Ô¼, ­§Ú®w¦Ú²Ô¼, ºÒÝÄp
	boost::regex	m_regCutter;			// ØíÚ²Ô³K\»
	int			m_nDXF[2];			// View, OrgType
	CStringList	m_strMillList,			// ØíðÌ§²Ù¼Ìð
				m_strLatheList,			// ùÕpØíðÌ§²Ù¼Ìð
				m_strLayerToInitList;	// Ú²Ô¼ÆðÌ§²ÙÌÖWÌ§²ÙÌð
	enMAKETYPE	m_enMakeType;		// ¼OÌNC¶¬À²Ìß

	BOOL	AddListHistory(CStringList&, LPCTSTR);
	void	DelListHistory(CStringList&, LPCTSTR);
	BOOL	ReadInitHistory(enMAKETYPE);
	BOOL	SaveInitHistory(enMAKETYPE);
	BOOL	SaveLayerHistory(void);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);		// Ú¼Þ½ÄØÖÌÛ¶(ºLÜÞ)

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
	enMAKETYPE	GetNCMakeType(void) const {
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
