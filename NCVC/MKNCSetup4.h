// MKNCSetup4.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup4 �_�C�A���O

class CMKNCSetup4 : public CPropertyPage
{
	void	EnableControl_Dwell(void);
	void	EnableControl_Circle(void);
	void	EnableControl_DrillQ(void);

// �R���X�g���N�V����
public:
	CMKNCSetup4();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMKNCSetup4)
	enum { IDD = IDD_MKNC_SETUP4 };
	CComboBox	m_ctCircleGroup;
	CComboBox	m_ctCircleProcess;
	CButton	m_ctCircleBreak;
	CStatic	m_ctDwellUnit;
	CFloatEdit	m_dFeed;
	CFloatEdit	m_dDrillZ;
	CFloatEdit	m_dDrillR;
	CFloatEdit	m_dCircleR;
	CFloatEdit	m_dDrillQ;
	CIntEdit	m_nDwell;
	CIntEdit	m_nSpindle;
	int		m_nProcess;
	int		m_nDwellFormat;
	int		m_nDrillReturn;
	int		m_nSort;
	int		m_nCircleProcess;
	BOOL	m_bDrillMatch;
	BOOL	m_bCircle;
	BOOL	m_bCircleBreak;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMKNCSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMKNCSetup4)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDwellFormat();
	afx_msg void OnSelchangeZProcess();
	afx_msg void OnCircle();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
