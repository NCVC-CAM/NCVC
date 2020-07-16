// CustomControl.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "resource.h"
#include "CustomControl.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

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

BEGIN_MESSAGE_MAP(CIEStatic, CStatic)
	//{{AFX_MSG_MAP(CIEStatic)
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
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
	CString	strBuf;
	strBuf.Format("%d", n);
	SetWindowText(strBuf);
	return (*this);
}

CIntEdit::operator int()
{
	ASSERT(::IsWindow(m_hWnd));
	CString	strNumber;
	GetWindowText(strNumber);
	return atoi(strNumber);
}

///////////////////////////////////
// CIntEdit ���b�Z�[�W �n���h��

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

CFloatEdit& CFloatEdit::operator =(double d)
{
	ASSERT(::IsWindow(m_hWnd));
	CString	strBuf;
	if ( m_bIntFormat ) {
		double	integer;
		if ( fabs(modf(d, &integer)) == 0.0 )
			strBuf.Format("%d", (int)integer);
		else
			strBuf.Format(IDS_MAKENCD_FORMAT, d);
	}
	else
		strBuf.Format(IDS_MAKENCD_FORMAT, d);
	SetWindowText(strBuf);
	return (*this);
}

CFloatEdit::operator double()
{
	ASSERT(::IsWindow(m_hWnd));
	CString	strNumber;
	GetWindowText(strNumber);
	return atof(strNumber);
}

///////////////////////////////////
// CFloatEdit ���b�Z�[�W �n���h��

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
// CIEStatic

CIEStatic::CIEStatic()
{
}

CIEStatic::~CIEStatic()
{
}

///////////////////////////////////
// CIEStatic ���b�Z�[�W �n���h��

void CIEStatic::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CString	strURL;
	GetWindowText(strURL);
	::ShellExecute(NULL, NULL, strURL, NULL, NULL, SW_SHOWNORMAL);

	CStatic::OnLButtonUp(nFlags, point);
}

BOOL CIEStatic::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if ( nHitTest == HTCLIENT ) {
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));	// WINVER>=0x0500
		return TRUE;
	}
	return CStatic::OnSetCursor(pWnd, nHitTest, message);
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
// CColComboBox ���b�Z�[�W �n���h��

void CColComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	// ؽč��ڂ��ި��(��è��)���۰قƓ��������ɂ���
	lpMeasureItemStruct->itemHeight = GetItemHeight(-1);
}

void CColComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	// ���۰ّS�̂̕`��(���тɓo�^���ꂽ32�ޯĒl��װ���ނƔF��)
	HBRUSH hBrush = ::CreateSolidBrush( (COLORREF)(lpDrawItemStruct->itemData) );
	::FillRect(lpDrawItemStruct->hDC, &(lpDrawItemStruct->rcItem), hBrush);
	::DeleteObject(hBrush);
	// �I����Ԃ݂̂��ύX
	if ( lpDrawItemStruct->itemState & (ODS_FOCUS|ODS_SELECTED) )
		::DrawFocusRect(lpDrawItemStruct->hDC, &(lpDrawItemStruct->rcItem));
}
