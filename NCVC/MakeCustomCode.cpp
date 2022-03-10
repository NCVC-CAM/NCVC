// ¶½ÀÑÍ¯ÀŞ°, Ì¯ÀŞ°ˆ—‚ÌŠî’ê¸×½
//	from TH_MakeNCD.cpp TH_MakeLathe.cpp
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "3dModelDoc.h"
#include "NCMakeOption.h"
#include "MakeCustomCode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;

static	LPCTSTR	g_szCustomCode[] = {
	"MakeUser", "MakeDate", "MakeTime",
	"MakeNCD", "MakeDXF", "MakeCondition"
};

CMakeCustomCode::CMakeCustomCode
	(const CDXFDoc* pDoc, const CDXFdata* pData, const CNCMakeOption* pMakeOpt)
{
	m_pDXFDoc	= pDoc;
	m_p3DMDoc	= NULL;
	m_pData		= pData;
	m_pMakeOpt	= pMakeOpt;
	m_strOrderIndex.SetElement(SIZEOF(g_szCustomCode), g_szCustomCode);	// ‹¤’Ê·°Ü°ÄŞ‚ğ“o˜^
}

CMakeCustomCode::CMakeCustomCode
	(const C3dModelDoc* pDoc, const CDXFdata* pData, const CNCMakeOption* pMakeOpt)
{
	m_pDXFDoc	= NULL;
	m_p3DMDoc	= pDoc;
	m_pData		= pData;
	m_pMakeOpt	= pMakeOpt;
	m_strOrderIndex.SetElement(SIZEOF(g_szCustomCode), g_szCustomCode);
}

tuple<int, CString>	CMakeCustomCode::ReplaceCustomCode(const std::string& str) const
{
	static	LPCTSTR	szReplaceErr = "???";

	INT_PTR	nTestCode;
	CString	strResult, strPath, strFile;
	TCHAR	szUserName[_MAX_PATH];
	DWORD	dwResult;
	CTime	time;
	
	nTestCode = (str.length()>2 && str.front()=='{' && str.back()=='}') ?
		m_strOrderIndex.GetIndex(str.substr(1, str.length()-2).c_str()) : -1;

	// replace
	switch ( nTestCode ) {
	case 0:		// MakeUser
		dwResult = _MAX_PATH;
		// Õ°»Ş–¼‚ÉŠ¿š‚ªŠÜ‚Ü‚ê‚Ä‚¢‚é‚Æ¶¬‚µ‚È‚¢
		strResult = ::GetUserName(szUserName, &dwResult) ?
						szUserName : szReplaceErr;
		break;
	case 1:		// MakeDate
		time = CTime::GetCurrentTime();
		VERIFY(strPath.LoadString(ID_INDICATOR_DATE_F2));	// %y/%m/%d
		strResult = time.Format(strPath);
		break;
	case 2:		// MakeTime
		time = CTime::GetCurrentTime();
		VERIFY(strPath.LoadString(ID_INDICATOR_TIME_F));	// %H:%M
		strResult = time.Format(strPath);
		break;
	case 3:		// MakeNCD
		if ( m_pDXFDoc ) {
			if ( m_pDXFDoc->IsDocFlag(DXFDOC_BIND) ) {
				CDXFDoc* pDoc = m_pDXFDoc->GetBindParentDoc();
				ASSERT( pDoc );
				::Path_Name_From_FullPath(pDoc->GetNCFileName(), strPath, strFile);
			}
			else {
				::Path_Name_From_FullPath(m_pDXFDoc->GetNCFileName(), strPath, strFile);
			}
			strResult = strFile;
		}
		else if ( m_p3DMDoc ) {
			::Path_Name_From_FullPath(m_p3DMDoc->GetNCFileName(), strPath, strFile);
			strResult = strFile;
		}
		break;
	case 4:		// MakeDXF
		if ( m_pDXFDoc ) {
			if ( m_pDXFDoc->IsDocFlag(DXFDOC_BIND) ) {
				CDXFDoc* pDoc = m_pDXFDoc->GetBindParentDoc();
				ASSERT( pDoc );
				CString	strDocFile(pDoc->GetPathName());
				if ( strDocFile.IsEmpty() )
					strFile = szReplaceErr;
				else
					::Path_Name_From_FullPath(strDocFile, strPath, strFile);
			}
			else {
				::Path_Name_From_FullPath(m_pDXFDoc->GetPathName(), strPath, strFile);	// GetTitle()‚É‚Í"*"‚ª•t‚­‰Â”\«‚ ‚è
			}
			strResult = strFile;
		}
		else if ( m_p3DMDoc ) {
			::Path_Name_From_FullPath(m_p3DMDoc->GetPathName(), strPath, strFile);
			strResult = strFile;
		}
		break;
	case 5:		// MakeCondition
		::Path_Name_From_FullPath(m_pMakeOpt->GetInitFile(), strPath, strFile);
		strResult = strFile;
		break;
	}

	int	nResult = (int)(nTestCode - SIZEOF(g_szCustomCode));	// ‹¤’Ê·°Ü°ÄŞ•ª‚ğŒ¸Z

	return make_tuple(nResult, strResult);
}
