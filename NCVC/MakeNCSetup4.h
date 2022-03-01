// MakeNCSetup4.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup4 �_�C�A���O

class CMakeNCSetup4 : public CPropertyPage
{
	void	EnableControl_Dwell(void);
	void	EnableControl_Circle(void);
	void	EnableControl_DrillQ(void);

// �R���X�g���N�V����
public:
	CMakeNCSetup4();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCSetup4)
	enum { IDD = IDD_MKNC_SETUP4 };
	CComboBox	m_ctCircleGroup;
	CComboBox	m_ctCircleProcess;
	CButton	m_ctCircleBreak;
	CStatic	m_ctDwellUnit;
	CStatic	m_ctDwellWorning;
	CFloatEdit	m_dFeed;
	CFloatEdit	m_dDrillZ;
	CFloatEdit	m_dDrillR;
	CFloatEdit	m_dCircleR;
	CFloatEdit	m_dDrillQ;
	CFloatEdit	m_dDwell;
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

	//{{AFX_VIRTUAL(CMakeNCSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeNCSetup4)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDwellFormat();
	afx_msg void OnSelchangeZProcess();
	afx_msg void OnCircle();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
