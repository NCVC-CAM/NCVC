// MakeDXFDlg2.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg2 �_�C�A���O

class CMakeDXFDlg2 : public CPropertyPage
{
	void	EnableControl_CycleR(void);

// �R���X�g���N�V����
public:
	CMakeDXFDlg2();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeDXFDlg2)
	enum { IDD = IDD_MAKEDXF2 };
	CFloatEdit	m_dCycleR;
	CFloatEdit	m_dLength;
	BOOL	m_bOrgCircle;
	BOOL	m_bOrgCross;
	int		m_nCycle;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMakeDXFDlg2)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeDXFDlg2)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCycle();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
