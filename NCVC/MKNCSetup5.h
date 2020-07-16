// MKNCSetup5.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup5 �_�C�A���O

class CMKNCSetup5 : public CPropertyPage
{
	void	EnableControl_Drill(void);

// �R���X�g���N�V����
public:
	CMKNCSetup5();
	~CMKNCSetup5();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMKNCSetup5)
	enum { IDD = IDD_MKNC_SETUP5 };
	CFloatEdit	m_dTolerance;
	CFloatEdit	m_dDrillMargin;
	int		m_nOptimaizeDrill;
	int		m_nTolerance;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMKNCSetup5)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMKNCSetup5)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDrill();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
