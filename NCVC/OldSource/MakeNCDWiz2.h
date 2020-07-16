#if !defined(___CMAKENCDWIZ2___)
#define ___CMAKENCDWIZ2___

#pragma once

// CMakeNCDWiz2 �_�C�A���O

class CMakeNCDWiz2 : public CPropertyPage
{
	CMCOption	m_optMC;		// MC��߼��(�H��a�Q�Ɨp)
	int			m_nCurSelMC;	// ̫����O�ɑI������Ă����@�B���ޯ��
	double		m_dMachine;		// �@�B���̍H��a

	CString	GetMCListFile(int);
	void	SetRefTool(void);

public:
	CMakeNCDWiz2();
	virtual ~CMakeNCDWiz2();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MAKENCD_WIZ2 };
	CIntEdit m_nTool;
	CComboBox m_ctMachine;
	CString m_strMachineRef;
	CFloatEdit m_dTool;
	int m_nCorrect;
	int m_nPreference;
	CString m_strTool;
	BOOL m_bG10;

	CString	m_strMachine;	// �@�B������߽

	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	afx_msg void OnBnClickedG10();
	afx_msg void OnEnKillfocusTool();
	afx_msg void OnSelchangeMC();

	DECLARE_MESSAGE_MAP()
};

#endif