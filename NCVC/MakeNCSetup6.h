// MakeNCSetup6.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup6 �_�C�A���O

class CMakeNCSetup6 : public CPropertyPage
{
// �R���X�g���N�V����
public:
	CMakeNCSetup6();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCSetup6)
	enum { IDD = IDD_MKNC_SETUP6 };
	CStatic m_ctCircleGroup;
	CButton m_ctCircleR;
	CButton m_ctCircleIJ;
	CButton	m_ctCircleHalf;
	CButton	m_ctZeroCutIJ;
	CFloatEdit	m_dEllipse;
	int		m_nDot;
	int		m_nFDot;
	int		m_nCircleCode;
	int		m_nIJ;
	BOOL	m_bZeroCut;
	BOOL	m_bCircleHalf;
	BOOL	m_bZeroCutIJ;
	BOOL	m_bEllipse;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMakeNCSetup6)
public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeNCSetup6)
	virtual BOOL OnInitDialog();
	afx_msg void OnCircleR();
	afx_msg void OnCircleIJ();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
