// MakeDXFDlg2.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg2 ダイアログ

class CMakeDXFDlg2 : public CPropertyPage
{
	void	EnableControl_CycleR(void);

// コンストラクション
public:
	CMakeDXFDlg2();

// ダイアログ データ
	//{{AFX_DATA(CMakeDXFDlg2)
	enum { IDD = IDD_MAKEDXF2 };
	CFloatEdit	m_dCycleR;
	CFloatEdit	m_dLength;
	BOOL	m_bOrgCircle;
	BOOL	m_bOrgCross;
	int		m_nCycle;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMakeDXFDlg2)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeDXFDlg2)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCycle();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
