// MakeLatheSetup5.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup5 ダイアログ

class CMakeLatheSetup5 : public CPropertyPage
{
public:
	CMakeLatheSetup5();

// ダイアログ データ
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
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
