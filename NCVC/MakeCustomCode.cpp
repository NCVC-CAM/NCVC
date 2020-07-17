// ¶½ÀÑÍ¯ÀÞ°, Ì¯ÀÞ°ˆ—‚ÌŠî’ê¸×½
//	from TH_MakeNCD.cpp TH_MakeLathe.cpp
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "NCMakeOption.h"
#include "MakeCustomCode.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace std;

static	LPCTSTR	g_szCustomCode[] = {
	"MakeUser", "MakeDate", "MakeTime", "MakeNCD", "MakeDXF", "MakeCondition"
};

CMakeCustomCode::CMakeCustomCode(CString& str,
	const CDXFDoc* pDoc, const CDXFdata* pData,
	const CNCMakeOption* pMakeOpt) : m_strResult(str)
{
	m_pDoc		= pDoc;
	m_pData		= pData;
	m_pMakeOpt	= pMakeOpt;
	// ‹¤’Ê·°Ü°ÄÞ‚ð“o˜^
	m_strOrderIndex.SetElement(SIZEOF(g_szCustomCode), g_szCustomCode);
}

int CMakeCustomCode::ReplaceCustomCode(const char* s, const char* e) const
{
	static	LPCTSTR	szReplaceErr = "???";

	if ( *s != '{' ) {
		m_strResult += string(s, e).c_str();
		return -1;
	}

	// ‘OŒã‚Ì "{}" œ‹Ž‚µ‚Ä‰ðÍ
	int		nTestCode = m_strOrderIndex.GetIndex(string(s+1, e-1).c_str());
	CString	strPath, strFile;
	TCHAR	szUserName[_MAX_PATH];
	DWORD	dwResult;
	CTime	time;

	// replace
	switch ( nTestCode ) {
	case 0:		// MakeUser
		dwResult = _MAX_PATH;
		// Õ°»Þ–¼‚ÉŠ¿Žš‚ªŠÜ‚Ü‚ê‚Ä‚¢‚é‚Æ¶¬‚µ‚È‚¢
		m_strResult += ::GetUserName(szUserName, &dwResult) && IsNCchar(szUserName) ?
			szUserName : szReplaceErr;
		break;
	case 1:		// MakeDate
		time = CTime::GetCurrentTime();
		VERIFY(strPath.LoadString(ID_INDICATOR_DATE_F2));	// %y/%m/%d
		m_strResult += time.Format(strPath);
		break;
	case 2:		// MakeTime
		time = CTime::GetCurrentTime();
		VERIFY(strPath.LoadString(ID_INDICATOR_TIME_F));	// %H:%M
		m_strResult += time.Format(strPath);
		break;
	case 3:		// MakeNCD
		::Path_Name_From_FullPath(m_pDoc->GetNCFileName(), strPath, strFile);
		m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
		break;
	case 4:		// MakeDXF
		::Path_Name_From_FullPath(m_pDoc->GetPathName(), strPath, strFile);	// GetTitle()‚É‚Í"*"‚ª•t‚­‰Â”\«‚ ‚è
		m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
		break;
	case 5:		// MakeCondition
		::Path_Name_From_FullPath(m_pMakeOpt->GetInitFile(), strPath, strFile);
		m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
		break;
	}

	return nTestCode - SIZEOF(g_szCustomCode);	// ‹¤’Ê·°Ü°ÄÞ•ª‚ðŒ¸ŽZ
}

BOOL CMakeCustomCode::IsNCchar(LPCTSTR lpsz) const
{
	for ( int i=0; i<lstrlen(lpsz); i++ ) {
//		if ( isprint(lpsz[i]) == 0 )
		if ( ::IsDBCSLeadByte(lpsz[i]) )
			return FALSE;
	}
	return TRUE;
}
