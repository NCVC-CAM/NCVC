// C3dRoughScanSetupDlg ダイアログ
//

#pragma once

class C3dModelDoc;

class C3dRoughScanSetupDlg : public CDialog
{
	C3dModelDoc*	m_pDoc;

public:
	C3dRoughScanSetupDlg(C3dModelDoc*, CWnd* pParent = nullptr);   // 標準コンストラクター

// ダイアログ データ
	enum { IDD = IDD_3DROUGH };
	CFloatEdit	m_dBallEndmill;
	CFloatEdit	m_dHeight;
	CFloatEdit	m_dZCut;
	CIntEdit	m_nLineSplit;
	BOOL		m_bZOrigin;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
