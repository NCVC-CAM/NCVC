// ExtensionDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CExtensionDlg ダイアログ

class CExtensionDlg : public CDialog
{
// コンストラクション
public:
	CExtensionDlg();

// ダイアログ データ
	//{{AFX_DATA(CExtensionDlg)
	enum { IDD = IDD_EXTENSION };
	//}}AFX_DATA
	CString		m_strExtTxt[2];	// ncd or dxf
	CListBox	m_ctExtList[2];
	CEdit		m_ctExtTxt[2];
	CButton		m_ctExtDelBtn[2];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CExtensionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CExtensionDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnExtAdd();
	afx_msg void OnExtDel();
	afx_msg void OnExtSelchangeList();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
