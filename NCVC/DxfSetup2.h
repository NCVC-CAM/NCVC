// DxfSetup2.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup2 ダイアログ

class CDxfSetup2 : public CPropertyPage
{
	void	EnableReloadButton(void);

// コンストラクション
public:
	CDxfSetup2();
	~CDxfSetup2();

// ダイアログ データ
	//{{AFX_DATA(CDxfSetup2)
	enum { IDD = IDD_DXF_SETUP2 };
	CButton	m_ctReload;
	CEdit	m_ctStartLayer;
	CString	m_strStartLayer;
	CString	m_strMoveLayer;
	CString	m_strCommentLayer;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CDxfSetup2)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CDxfSetup2)
	virtual BOOL OnInitDialog();
	afx_msg void OnReload();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
