#if !defined(___CMAKENCDWIZ2___)
#define ___CMAKENCDWIZ2___

#pragma once

// CMakeNCDWiz2 ダイアログ

class CMakeNCDWiz2 : public CPropertyPage
{
	CMCOption	m_optMC;		// MCｵﾌﾟｼｮﾝ(工具径参照用)
	int			m_nCurSelMC;	// ﾌｫｰｶｽ前に選択されていた機械ｲﾝﾃﾞｯｸｽ
	double		m_dMachine;		// 機械情報の工具径

	CString	GetMCListFile(int);
	void	SetRefTool(void);

public:
	CMakeNCDWiz2();
	virtual ~CMakeNCDWiz2();

// ダイアログ データ
	enum { IDD = IDD_MAKENCD_WIZ2 };
	CIntEdit m_nTool;
	CComboBox m_ctMachine;
	CString m_strMachineRef;
	CFloatEdit m_dTool;
	int m_nCorrect;
	int m_nPreference;
	CString m_strTool;
	BOOL m_bG10;

	CString	m_strMachine;	// 機械情報ﾌﾙﾊﾟｽ

	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	afx_msg void OnBnClickedG10();
	afx_msg void OnEnKillfocusTool();
	afx_msg void OnSelchangeMC();

	DECLARE_MESSAGE_MAP()
};

#endif