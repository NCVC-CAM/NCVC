// C3dScanSetupDlg ダイアログ
//

#pragma once

class C3dScanSetupDlg : public CDialog
{
public:
	C3dScanSetupDlg(CWnd* pParent = nullptr);   // 標準コンストラクター

// ダイアログ データ
	enum { IDD = IDD_3DSCAN };
	CFloatEdit	m_dBallEndmill;
	CFloatEdit	m_dHeight;
	CFloatEdit	m_dZCut;
	CIntEdit	m_nLineSplit;
	BOOL		m_bOrigin;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
