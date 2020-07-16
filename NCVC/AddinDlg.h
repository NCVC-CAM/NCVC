// AddinDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CAddinDlg ダイアログ

class CAddinDlg : public CDialog
{
	void	SetDetailData(CNCVCaddinIF*);

// コンストラクション
public:
	CAddinDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CAddinDlg)
	enum { IDD = IDD_ADDININFO };
	CEdit	m_ctAddnInfo;
	CEdit	m_ctReadMe;
	CListCtrl	m_ctList;
	CString	m_strAddinInfo;
	CString	m_strReadMe;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CAddinDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CAddinDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemChangedAddinList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
