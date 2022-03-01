// MakeNCSetup5.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup5 �_�C�A���O

class CMakeNCSetup5 : public CPropertyPage
{
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strScriptPath;	// �{�����߽��

	void	EnableControl_Drill(void);

// �R���X�g���N�V����
public:
	CMakeNCSetup5();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCSetup5)
	enum { IDD = IDD_MKNC_SETUP5 };
	CFloatEdit	m_dTolerance;
	CFloatEdit	m_dDrillMargin;
	CFloatEdit	m_dZApproach;
	CIntEdit	m_nZAppDwell;
	CEdit	m_ctScript;
	int		m_nOptimaizeDrill;
	int		m_nTolerance;
	CString	m_strScript;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMakeNCSetup5)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeNCSetup5)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDrill();
	afx_msg void OnScriptLookup();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
