// ViewSetup3.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup3 �_�C�A���O

class CViewSetup3 : public CPropertyPage
{
	LOGFONT		m_lfFont;
	COLORREF	m_colView[8];
	CBrush		m_brColor[8];

// �R���X�g���N�V����
public:
	CViewSetup3();
	~CViewSetup3();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CViewSetup3)
	enum { IDD = IDD_VIEW_SETUP3 };
	CButton	m_ctFontButton;
	//}}AFX_DATA
	CStatic		m_ctColor[8];
	CComboBox	m_cbLineType[5];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CViewSetup3)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CViewSetup3)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnChange();
	afx_msg void OnColorButton();
	afx_msg void OnFontChange();
	afx_msg void OnDefColor();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
