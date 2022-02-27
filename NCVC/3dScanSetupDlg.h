// C3dScanSetupDlg ダイアログ
//

#pragma once

struct SCANSETUP
{
	float	dBallEndmill;
	float	dHeight;
	float	dZCut;
	int		nLineSplit;
	BOOL	bOrigin;
};

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

	SCANSETUP	m;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
