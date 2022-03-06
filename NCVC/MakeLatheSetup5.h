// MakeLatheSetup5.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup5 �_�C�A���O

class CMakeLatheSetup5 : public CPropertyPage
{
public:
	CMakeLatheSetup5();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MKLA_SETUP5 };
	CString m_strCustom;
	CIntEdit m_nSpindle;
	CFloatEdit m_dFeed;
	CFloatEdit m_dXFeed;
	CFloatEdit m_dPullX;
	CFloatEdit m_dWidth;
	CIntEdit   m_nDwell;
	int		m_nTool;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
