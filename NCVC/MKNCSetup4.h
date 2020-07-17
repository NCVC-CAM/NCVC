// MKNCSetup4.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup4 ダイアログ

class CMKNCSetup4 : public CPropertyPage
{
	void	EnableControl_Dwell(void);
	void	EnableControl_Circle(void);
	void	EnableControl_DrillQ(void);

// コンストラクション
public:
	CMKNCSetup4();

// ダイアログ データ
	//{{AFX_DATA(CMKNCSetup4)
	enum { IDD = IDD_MKNC_SETUP4 };
	CComboBox	m_ctCircleGroup;
	CComboBox	m_ctCircleProcess;
	CButton	m_ctCircleBreak;
	CStatic	m_ctDwellUnit;
	CFloatEdit	m_dFeed;
	CFloatEdit	m_dDrillZ;
	CFloatEdit	m_dDrillR;
	CFloatEdit	m_dCircleR;
	CFloatEdit	m_dDrillQ;
	CIntEdit	m_nDwell;
	CIntEdit	m_nSpindle;
	int		m_nProcess;
	int		m_nDwellFormat;
	int		m_nDrillReturn;
	int		m_nSort;
	int		m_nCircleProcess;
	BOOL	m_bDrillMatch;
	BOOL	m_bCircle;
	BOOL	m_bCircleBreak;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMKNCSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMKNCSetup4)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDwellFormat();
	afx_msg void OnSelchangeZProcess();
	afx_msg void OnCircle();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
