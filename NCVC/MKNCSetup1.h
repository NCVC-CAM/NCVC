// MKNCSetup1.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup1 �_�C�A���O

class CMKNCSetup1 : public CPropertyPage
{
// �R���X�g���N�V����
public:
	CMKNCSetup1();
	~CMKNCSetup1();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMKNCSetup1)
	enum { IDD = IDD_MKNC_SETUP1 };
	CButton	m_ctHeaderBt;
	CButton	m_ctFooterBt;
	CEdit	m_ctHeader;
	CEdit	m_ctFooter;
	CFloatEdit	m_dZCut;
	CFloatEdit	m_dZG0Stop;
	CFloatEdit	m_dZFeed;
	CFloatEdit	m_dFeed;
	CIntEdit	m_nSpindle;
	CString	m_strFooter;
	CString	m_strHeader;
	BOOL	m_bXrev;
	BOOL	m_bYrev;
	//}}AFX_DATA
	CFloatEdit	m_dG92[NCXYZ];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMKNCSetup1)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMKNCSetup1)
	virtual BOOL OnInitDialog();
	afx_msg void OnHeaderLoopup();
	afx_msg void OnFooterLoopup();
	afx_msg void OnHeaderEdit();
	afx_msg void OnFooterEdit();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
