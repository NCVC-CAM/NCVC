// CustomControl.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "CustomControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;
using std::string;
extern	LPCTSTR	gg_szDelimiter;	// ":"

BEGIN_MESSAGE_MAP(CIntEdit, CEdit)
	//{{AFX_MSG_MAP(CIntEdit)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CFloatEdit, CEdit)
	//{{AFX_MSG_MAP(CFloatEdit)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CColComboBox, CComboBox)
	//{{AFX_MSG_MAP(CColComboBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIntEdit

CIntEdit::CIntEdit()
{
}

CIntEdit::~CIntEdit()
{
}

CIntEdit& CIntEdit::operator =(int n)
{
	ASSERT(::IsWindow(m_hWnd));
	SetWindowText(lexical_cast<string>(n).c_str());
	return (*this);
}

CIntEdit::operator int()
{
	ASSERT(::IsWindow(m_hWnd));
	CString	strNumber;
	GetWindowText(strNumber);
	return atoi(LPCTSTR(strNumber.Trim()));
}

///////////////////////////////////
// CIntEdit メッセージ ハンドラ

void CIntEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	static	CString	strLeave("+-");
	TCHAR	tChar = (TCHAR)nChar;

	if ( (IsCharAlphaNumeric(tChar) && !IsCharAlpha(tChar)) ||
				strLeave.Find(tChar) >= 0 || nChar == VK_BACK )
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	else
		::MessageBeep(MB_ICONASTERISK);
}

/////////////////////////////////////////////////////////////////////////////
// CFloatEdit

CFloatEdit::CFloatEdit(BOOL bIntFormat/*=FALSE*/)
{
	m_bIntFormat = bIntFormat;
}

CFloatEdit::~CFloatEdit()
{
}

CFloatEdit& CFloatEdit::operator =(float d)
{
	ASSERT(::IsWindow(m_hWnd));
	CString	strBuf;
	if ( m_bIntFormat ) {
		float	integer;
		if ( fabs(modf(d, &integer)) == 0.0f )
			strBuf = lexical_cast<string>((int)integer).c_str();
		else
			strBuf.Format(IDS_MAKENCD_FORMAT, d);
	}
	else
		strBuf.Format(IDS_MAKENCD_FORMAT, d);
	SetWindowText(strBuf);
	return (*this);
}

CFloatEdit::operator float()
{
	ASSERT(::IsWindow(m_hWnd));
	CString	strNumber;
	GetWindowText(strNumber);
	return (float)atof(LPCTSTR(strNumber.Trim()));
}

///////////////////////////////////
// CFloatEdit メッセージ ハンドラ

void CFloatEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	static	CString	strLeave("+-.");
	TCHAR	tChar = (TCHAR)nChar;

	if ( (IsCharAlphaNumeric(tChar) && !IsCharAlpha(tChar)) ||
				strLeave.Find(tChar) >= 0 || nChar == VK_BACK )
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	else
		::MessageBeep(MB_ICONASTERISK);
}

/////////////////////////////////////////////////////////////////////////////
// CColComboBox

CColComboBox::CColComboBox()
{
}

CColComboBox::~CColComboBox()
{
}

///////////////////////////////////
// CColComboBox メッセージ ハンドラ

void CColComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	// ﾘｽﾄ項目をｴﾃﾞｨｯﾄ(ｽﾀﾃｨｯｸ)ｺﾝﾄﾛｰﾙと同じ高さにする
	lpMeasureItemStruct->itemHeight = GetItemHeight(-1);
}

void CColComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	// ｺﾝﾄﾛｰﾙ全体の描画(ｱｲﾃﾑに登録された32ﾋﾞｯﾄ値をｶﾗｰｺｰﾄﾞと認識)
	COLORREF	rgb = ConvertSTRtoRGB((LPCTSTR)(lpDrawItemStruct->itemData));
	HBRUSH		hBrush = ::CreateSolidBrush(rgb);
	::FillRect(lpDrawItemStruct->hDC, &(lpDrawItemStruct->rcItem), hBrush);
	::DeleteObject(hBrush);
	// 選択状態のみが変更
	if ( lpDrawItemStruct->itemState & (ODS_FOCUS|ODS_SELECTED) )
		::DrawFocusRect(lpDrawItemStruct->hDC, &(lpDrawItemStruct->rcItem));
}

/////////////////////////////////////////////////////////////////////////////

COLORREF ConvertSTRtoRGB(LPCTSTR lpszCol)
{
	LPTSTR	lpsztok, lpszcontext, lpszBuf = NULL;
	BYTE	col[3] = {0, 0, 0};

	try {
		lpszBuf = new TCHAR[lstrlen(lpszCol)+1];
		lpsztok = strtok_s(lstrcpy(lpszBuf, lpszCol), gg_szDelimiter, &lpszcontext);
		// Get Color
		for ( int i=0; i<SIZEOF(col) && lpsztok; i++ ) {
			col[i] = atoi(lpsztok);
			lpsztok = strtok_s(NULL, gg_szDelimiter, &lpszcontext);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
	if ( lpszBuf )
		delete[]	lpszBuf;

	return RGB(col[0], col[1], col[2]);
}

CString ConvertRGBtoSTR(COLORREF col)
{
	CString	strRGB = (
		lexical_cast<string>((int)GetRValue(col)) + gg_szDelimiter +
		lexical_cast<string>((int)GetGValue(col)) + gg_szDelimiter +
		lexical_cast<string>((int)GetBValue(col)) ).c_str();
	return strRGB;
}
