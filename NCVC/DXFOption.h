// DXFOption.h: DXFµÌß¼®ÝÌÇ
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

enum {
	DXFOPT_VIEW = 0,
	DXFOPT_ORGTYPE,
	DXFOPT_BINDORG,
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
	NCMAKELATHE,		// ùÕ
	NCMAKEWIRE,			// Ü²ÔúdÁH@
	NCMAKELAYER,		// Ú²Ô¼ÆðÌ§²ÙÌÖWÌ§²ÙÌð
		NCMAKENUM			// [4]
};

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;
friend	class	CCADbindDlg;

	union {
		struct {
			int		m_nView,		// Ï·ãËÞ­°N®
					m_nOrgType,		// ´_Ú²Ôª³¢Æ«Ì
									//    0:´×°,  1`4:Eã,Eº,¶ã,¶º, 5:
					m_nBindOrg;		// CADÃÞ°ÀÌÁH´_
									//    0:, 1`4:Eã,Eº,¶ã,¶º
		};
		int			m_unNums[DXFOPT_NUMS];
	};
	union {
		struct {
			double	m_dBindWork[2],	// CADÃÞ°ÀÌÜ°¸»²½Þ
					m_dBindMargin;	// zuÏ°¼ÞÝ
		};
		double		m_udNums[DXFOPT_DBL_NUMS];
	};
	CString	m_strReadLayer[DXFLAYERSIZE];	// ´_CØí(üÍ²Ò°¼ÞÛ¶p)C
											// ÁHJnÊuÚ²Ô¼, ­§Ú®w¦Ú²Ô¼, ºÒÝÄp
	CStringList	m_strInitList[NCMAKENUM];	// ØíðÌ§²Ù¼Ìð
	enMAKETYPE	m_enMakeType;				// ¼OÌNC¶¬À²Ìß
	boost::regex	m_regCutter;			// ØíÚ²Ô³K\»

	BOOL	AddListHistory(enMAKETYPE, LPCTSTR);
	BOOL	ReadInitHistory(enMAKETYPE);
	BOOL	SaveInitHistory(enMAKETYPE);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);		// Ú¼Þ½ÄØÖÌÛ¶
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
	int		GetDxfFlag(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	void	SetViewFlag(BOOL bView) {
		m_nView = bView;
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
	double	GetBindSize(size_t n) const {
		return m_dBindWork[n];
	}
	double	GetBindMargin(void) const {
		return m_dBindMargin;
	}
	//
	// from DXFMakeOption.cpp, NCVCaddinDXF.cpp
	CString	GetReadLayer(size_t n) const {
		ASSERT( n>=0 && n<DXFLAYERSIZE );
		return m_strReadLayer[n];
	}
};
