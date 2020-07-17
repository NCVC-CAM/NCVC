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

using namespace boost;
using std::string;

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
	return lexical_cast<int>((LPCTSTR)strNumber);
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
			strBuf = lexical_cast<string>((int)integer).c_str();
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
	return lexical_cast<double>((LPCTSTR)strNumber);
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
