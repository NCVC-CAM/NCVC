// MKNCSetup2.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup2 �_�C�A���O

class CMKNCSetup2 : public CPropertyPage
{
	void	EnableControl_LineAdd(void);

// �R���X�g���N�V����
public:
	CMKNCSetup2();
	~CMKNCSetup2();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMKNCSetup2)
	enum { IDD = IDD_MKNC_SETUP2 };
	CComboBox	m_ctLineAdd;
	CEdit	m_ctLineForm;
	BOOL	m_bXrev;
	BOOL	m_bYrev;
	BOOL	m_bLineAdd;
	BOOL	m_bGclip;
	BOOL	m_bDisableSpindle;
	int		m_nLineAdd;
	int		m_nG90;
	int		m_nZReturn;
	CString	m_strLineForm;
	CString	m_strEOB;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMKNCSetup2)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMKNCSetup2)
	virtual BOOL OnInitDialog();
	afx_msg void OnLineAdd();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
