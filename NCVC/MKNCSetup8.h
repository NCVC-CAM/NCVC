// MKNCSetup8.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup8 �_�C�A���O

class CMKNCSetup8 : public CPropertyPage
{
// �R���X�g���N�V����
public:
	CMKNCSetup8();
	~CMKNCSetup8();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMKNCSetup8)
	enum { IDD = IDD_MKNC_SETUP8 };
	BOOL	m_bLayerComment;
	BOOL	m_bL0Cycle;
	int		m_nMoveZ;
	CString	m_strCustMoveB;
	CString	m_strCustMoveA;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMKNCSetup8)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMKNCSetup8)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
