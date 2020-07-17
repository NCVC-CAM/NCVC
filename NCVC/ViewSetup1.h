// ViewSetup1.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup1 �_�C�A���O

class CViewSetup1 : public CPropertyPage
{
	COLORREF	m_colView[COMCOL_NUMS];
	CBrush		m_brColor[COMCOL_NUMS];

	void	EnableControl(void);

// �R���X�g���N�V����
public:
	CViewSetup1();
	~CViewSetup1();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CViewSetup1)
	enum { IDD = IDD_VIEW_SETUP1 };
	BOOL	m_bMouseWheel;
	int		m_nWheelType;
	CButton m_ctMouseWheel[COMCOL_NUMS];
	//}}AFX_DATA
	CStatic		m_ctColor[COMCOL_NUMS];
	CComboBox	m_cbLineType[COMCOL_NUMS];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CViewSetup1)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CViewSetup1)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnChange();
	afx_msg void OnColorButton();
	afx_msg void OnWheel();
	afx_msg void OnDefColor();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
