// DxfAutoPocketDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoPocketgDlg ダイアログ

class CDxfAutoPocketgDlg : public CDialog
{
	void	SetDetailCtrl(void);

// コンストラクション
public:
	CDxfAutoPocketgDlg(LPAUTOWORKINGDATA);

// ダイアログ データ
	//{{AFX_DATA(CDxfAutoPocketgDlg)
	enum { IDD = IDD_DXFEDIT_AUTOPOCKET };
	int		m_nSelect;
	BOOL	m_bAcuteRound;
	int		m_nScan;
	BOOL	m_bCircle;
	CFloatEdit	m_ctOffset;
	CIntEdit m_ctLoop;
	CComboBox m_ctScan;
	CButton m_ctAcuteRound;
	CButton m_ctCircle;
	//}}AFX_DATA
	float	m_dOffset;	// ｵﾌｾｯﾄ値取得用
	int		m_nLoopCnt;	// 繰り返し数取得用

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
	afx_msg void OnBnClickedSelect();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
