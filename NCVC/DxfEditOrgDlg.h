// DxfEditOrgDlg.h : ヘッダー ファイル
//

#pragma once

//	ｺﾝﾄﾛｰﾙの非ｱｸﾃｨﾌﾞﾌﾗｸﾞ
#define	EDITORG_NUMERIC		0x0001
#define	EDITORG_ORIGINAL	0x0002

/////////////////////////////////////////////////////////////////////////////
// CDxfEditOrgDlg ダイアログ

class CDxfEditOrgDlg : public CDialog
{
	DWORD	m_dwControl;	// ｺﾝﾄﾛｰﾙの非ｱｸﾃｨﾌﾞﾌﾗｸﾞ
	void	SelectControl(int);

// コンストラクション
public:
	CDxfEditOrgDlg(DWORD dwControl);

// ダイアログ データ
	//{{AFX_DATA(CDxfEditOrgDlg)
	enum { IDD = IDD_DXFEDIT_ORG };
	CButton	m_ctSelNumeric;
	CButton	m_ctSelOriginal;
	CComboBox	m_ctRectType;
	CEdit	m_ctNumeric;
	int		m_nSelect;
	CString	m_strNumeric;
	int		m_nRectType;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDxfEditOrgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CDxfEditOrgDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelect();
	virtual void OnOK();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
