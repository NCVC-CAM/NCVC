// MakeLatheSetup4.h : �w�b�_�[ �t�@�C��
//

#pragma once
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup1 �_�C�A���O

class CMakeLatheSetup4 : public CPropertyPage
{
	void	EnableControl(void);

public:
	CMakeLatheSetup4();

	// �_�C�A���O �f�[�^
	enum { IDD = IDD_MKLA_SETUP4 };
	BOOL	m_bEndFace;
	CString m_strCustom;
	CEdit m_ctCustom;
	CIntEdit m_nSpindle;
	CFloatEdit m_dXFeed;
	CFloatEdit m_dCut;
	CFloatEdit m_dStep;
	CFloatEdit m_dPullZ;
	CFloatEdit m_dPullX;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	afx_msg void OnCheck();

	DECLARE_MESSAGE_MAP()
};
