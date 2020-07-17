// MakeNCDlgEx3.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx3 ダイアログ

class CMakeNCDlgEx3 : public CPropertyPage
{
	int		m_nSortColumn;	// ｿｰﾄ中の列番号(0:任意)
	void	SwapObject(int, int);
	void	SetFocusListCtrl(int);
	void	SetHeaderMark(int);

public:
	CMakeNCDlgEx3();
	virtual ~CMakeNCDlgEx3();

// ダイアログ データ
	enum { IDD = IDD_MAKENCD_EX3 };
	CListCtrl	m_ctLayerList;

public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	afx_msg void OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndScrollLayerList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnUp();
	afx_msg void OnDown();
	afx_msg LRESULT OnQuerySiblings(WPARAM, LPARAM);

	static UINT	m_nParentID;
	// ｿｰﾄ用ｺｰﾙﾊﾞｯｸ関数
	static int CALLBACK CompareFunc(LPARAM, LPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
