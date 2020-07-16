// MKNCSetup6.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup6 �_�C�A���O

class CMKNCSetup6 : public CPropertyPage
{
// �R���X�g���N�V����
public:
	CMKNCSetup6();
	~CMKNCSetup6();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMKNCSetup6)
	enum { IDD = IDD_MKNC_SETUP6 };
	CButton	m_ctCircleHalf;
	int		m_nDot;
	int		m_nFDot;
	BOOL	m_bZeroCut;
	int		m_nCircleCode;
	int		m_nIJ;
	BOOL	m_bCircleHalf;
	CFloatEdit	m_dEllipse;
	BOOL	m_bEllipse;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMKNCSetup6)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMKNCSetup6)
	virtual BOOL OnInitDialog();
	afx_msg void OnCircleR();
	afx_msg void OnCircleIJ();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
