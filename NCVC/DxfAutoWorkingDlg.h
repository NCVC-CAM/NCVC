// DxfAutoWorkingDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoWorkingDlg ダイアログ

class CDxfAutoWorkingDlg : public CDialog
{
	void	SetDetailCtrl(void);

// コンストラクション
public:
	CDxfAutoWorkingDlg(AUTOWORKINGDATA*);

// ダイアログ データ
	//{{AFX_DATA(CDxfAutoWorkingDlg)
	enum { IDD = IDD_DXFEDIT_AUTOWORKING };
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
	double	m_dOffset;	// ｵﾌｾｯﾄ値取得用
	int		m_nLoopCnt;	// 繰り返し数取得用

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDxfAutoWorkingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CDxfAutoWorkingDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedSelect();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
