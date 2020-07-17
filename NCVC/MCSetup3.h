// MCSetup3.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMCSetup3 ダイアログ

class CMCSetup3 : public CPropertyPage
{
	int		m_nSortColumn;	// ｿｰﾄ中の列番号(0:任意)
	BOOL	m_bChange;

	void	SetDetailData(const CMCTOOLINFO*);
	BOOL	CheckData(void);
	void	SetHeaderMark(int);

// コンストラクション
public:
	CMCSetup3();

// ダイアログ データ
	//{{AFX_DATA(CMCSetup3)
	enum { IDD = IDD_MC_SETUP3 };
	CFloatEdit	m_dToolH;
	CFloatEdit	m_dToolD;
	CIntEdit	m_nToolNo;
	CListCtrl	m_ctToolList;
	CString	m_strToolName;
	int		m_nType;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMCSetup3)
	public:
	virtual BOOL OnApply();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMCSetup3)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemChangedToolList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetDispInfoToolList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickToolList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDownToolList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAdd();
	afx_msg void OnMod();
	afx_msg void OnDel();
	//}}AFX_MSG

	// ｿｰﾄ用ｺｰﾙﾊﾞｯｸ関数
	static	int	CALLBACK	CompareToolListFunc(LPARAM, LPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
