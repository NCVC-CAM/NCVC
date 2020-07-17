// MCSetup1.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// CMCSetup1 �_�C�A���O

class CMCSetup1 : public CPropertyPage
{
// �R���X�g���N�V����
public:
	CMCSetup1();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMCSetup1)
	enum { IDD = IDD_MC_SETUP1 };
	CFloatEdit	m_dFspeed;
	CFloatEdit	m_dBlock;
	CString	m_strName;
	int		m_nFselect;
	//}}AFX_DATA
	int			m_nModalGroup[MODALGROUP];
	CIntEdit	m_nMoveSpeed[NCXYZ];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMCSetup1)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMCSetup1)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
