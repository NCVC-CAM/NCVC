// About.cpp
// �ް�ޮݏ���޲�۸�(NCVC.cpp���番��)
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
// CAboutDlg �_�C�A���O

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_ABOUT_URL, m_ctURL);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg ���b�Z�[�W �n���h��

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// ίĽ�߯Ăɉ����t��̫�Ă��w��
	LOGFONT	logFont;
	if ( GetFont()->GetLogFont(&logFont) ) {
		logFont.lfUnderline = TRUE;
		if ( m_fontURL.CreateFontIndirect(&logFont) )
			m_ctURL.SetFont(&m_fontURL);
	}

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// ίĽ�߯ĂɐF
	if ( m_ctURL.m_hWnd == pWnd->m_hWnd )
		pDC->SetTextColor(RGB(0,0,255));

	return hbr;
}
