// CLayerDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CLayerDlg ダイアログ

class CLayerDlg : public CDialog
{
	HTREEITEM	m_hTree[4];	// 各ﾚｲﾔのﾂﾘｰﾊﾝﾄﾞﾙ

	void	EnableButton(BOOL bEnable) {
		m_ctLayerTree.EnableWindow(bEnable);
		m_ctOK.EnableWindow(bEnable);
	}
	void	SetChildCheck(HTREEITEM);
	void	SetParentCheck(HTREEITEM);

// コンストラクション
public:
	CLayerDlg();

// ダイアログ データ
	//{{AFX_DATA(CLayerDlg)
	enum { IDD = IDD_DXFVIEW_LAYER };
	CTreeCtrl	m_ctLayerTree;
	CButton	m_ctOK;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CLayerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CLayerDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnLayerTreeKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLayerTreeClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLayerTreeGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
