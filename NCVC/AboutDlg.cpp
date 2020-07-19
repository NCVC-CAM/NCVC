// About.cpp
// ﾊﾞｰｼﾞｮﾝ情報ﾀﾞｲｱﾛｸﾞ(NCVC.cppから分離)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "AboutDlg.h"

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg ダイアログ

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_ABOUT_URL, m_ctURL);
	DDX_Control(pDX, IDC_ABOUT_VER, m_ctVersion);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg メッセージ ハンドラ

BOOL CAboutDlg::OnInitDialog() 
{
	__super::OnInitDialog();

#ifdef _WIN64
	CString	strVer;
	m_ctVersion.GetWindowText(strVer);
	strVer += _T(" 64bit version");
	m_ctVersion.SetWindowText(strVer);
#endif

	CString	strURL;
	m_ctURL.GetWindowText(strURL);
	m_ctURL.SetURL(strURL);
	m_ctURL.SetTooltip(_T("Visit NCVC WebPage"));
/*
	// ﾎｯﾄｽﾎﾟｯﾄに下線付きﾌｫﾝﾄを指示
	LOGFONT	logFont;
	if ( GetFont()->GetLogFont(&logFont) ) {
		logFont.lfUnderline = TRUE;
		if ( m_fontURL.CreateFontIndirect(&logFont) )
			m_ctURL.SetFont(&m_fontURL);
	}
*/
	m_ctURL.SizeToContent();

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = __super::OnCtlColor(pDC, pWnd, nCtlColor);

	// ﾎｯﾄｽﾎﾟｯﾄに青色
	if ( m_ctURL.m_hWnd == pWnd->m_hWnd )
		pDC->SetTextColor(RGB(0,0,255));

//	OnInitDialog() で
//	m_ctURL.SetTextColor(RGB(0,0,255));
//	は効果なかった

	return hbr;
}
