// MakeLatheSetup3.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup3 �_�C�A���O

class CMakeLatheSetup3 : public CPropertyPage
{
public:
	CMakeLatheSetup3();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MKLA_SETUP3 };
	CString m_strCustom;
	CIntEdit m_nSpindle;
	CIntEdit m_nMargin;
	CFloatEdit m_dFeed;
	CFloatEdit m_dXFeed;
	CFloatEdit m_dCut;
	CFloatEdit m_dPullZ;
	CFloatEdit m_dPullX;
	CFloatEdit m_dMargin;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	afx_msg void OnCopyFromOut();

	DECLARE_MESSAGE_MAP()
};
