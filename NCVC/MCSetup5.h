// MCSetup5.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMCSetup4 ダイアログ

class CMCSetup5 : public CPropertyPage
{
public:
	CMCSetup5();
	virtual ~CMCSetup5();

// ダイアログ データ
	enum { IDD = IDD_MC_SETUP5 };
	BOOL	m_bLathe,
			m_bL0Cycle;
	CString	m_strAutoBreak;

public:
	virtual BOOL OnApply();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
