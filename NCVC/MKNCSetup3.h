// MKNCSetup3.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup3 �_�C�A���O

class CMKNCSetup3 : public CPropertyPage
{
	void	EnableControl_MakeEnd(void);
	void	EnableControl_Deep(void);
	void	EnableControl_Helical(void);
	void	EnableControl_Finish(void);

// �R���X�g���N�V����
public:
	CMKNCSetup3();
	~CMKNCSetup3();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMKNCSetup3)
	enum { IDD = IDD_MKNC_SETUP3 };
	CButton		m_ctHelical;
	CButton		m_ctFinish;
	CComboBox	m_ctZProcess;
	CComboBox	m_ctCProcess;
	CComboBox	m_ctAProcess;
	CStatic		m_ctMakeLabel1;
	CIntEdit	m_nSpindle;
	CFloatEdit	m_dFeed;
	CFloatEdit	m_dZCut;
	CFloatEdit	m_dZG0Stop;
	CFloatEdit	m_dMakeValue;
	CFloatEdit	m_dMakeFeed;
	CFloatEdit	m_dZStep;
	CFloatEdit	m_dDeep;
	int		m_nMakeEnd;
	int		m_nDeepAll;
	int		m_nDeepRound;
	int		m_nDeepReturn;
	BOOL	m_bDeep;
	BOOL	m_bHelical;
	BOOL	m_bFinish;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMKNCSetup3)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMKNCSetup3)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeMakeEnd();
	afx_msg void OnSelchangeProcess();
	afx_msg void OnDeep();
	afx_msg void OnDeepFinish();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
