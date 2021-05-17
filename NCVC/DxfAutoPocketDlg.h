// DxfAutoPocketDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoPocketgDlg ダイアログ

class CDxfAutoPocketgDlg : public CDialogEx
{
	LPAUTOWORKINGDATA	m_pAuto;

// コンストラクション
public:
	CDxfAutoPocketgDlg(LPAUTOWORKINGDATA);

// ダイアログ データ
	//{{AFX_DATA(CDxfAutoPocketgDlg)
	enum { IDD = IDD_DXFEDIT_AUTOPOCKET };
	CFloatEdit	m_dOffset;
	CIntEdit	m_nLoop;
	BOOL	m_bAcuteRound;
	int		m_nScan;
	BOOL	m_bCircle;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDxfAutoPocketgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CDxfAutoPocketgDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
