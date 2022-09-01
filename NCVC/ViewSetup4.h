// ViewSetup4.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup4 �_�C�A���O

class CViewSetup4 : public CPropertyPage
{
	LOGFONT		m_lfFont;
	COLORREF	m_colView[NCINFOCOL_NUMS];
	CBrush		m_brColor[NCINFOCOL_NUMS];

// �R���X�g���N�V����
public:
	CViewSetup4();
	~CViewSetup4();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CViewSetup4)
	enum { IDD = IDD_VIEW_SETUP4 };
	CButton	m_ctFontButton;
	//}}AFX_DATA
	CStatic		m_ctColor[NCINFOCOL_NUMS];
	CIntEdit	m_nTraceSpeed[3];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CViewSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CViewSetup4)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnChange();
	afx_msg void OnColorButton();
	afx_msg void OnFontChange();
	afx_msg void OnDefColor();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
