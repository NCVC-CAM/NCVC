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
	CDxfAutoWorkingDlg(double, BOOL);

// ダイアログ データ
	//{{AFX_DATA(CDxfAutoWorkingDlg)
	enum { IDD = IDD_DXFEDIT_AUTOWORKING };
	int		m_nSelect;
	int		m_nDetail;
	CFloatEdit	m_ctOffset;
	BOOL	m_bAcuteRound;
	CButton m_ctDetail[4];
	CButton m_ctAcuteRound;
	//}}AFX_DATA
	double	m_dOffset;	// ｵﾌｾｯﾄ値取得用

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
