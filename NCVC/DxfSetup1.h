// DxfSetup1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup1 ダイアログ

class CDxfSetup1 : public CPropertyPage
{
	void	EnableReloadButton(void);

// コンストラクション
public:
	CDxfSetup1();

// ダイアログ データ
	//{{AFX_DATA(CDxfSetup1)
	enum { IDD = IDD_DXF_SETUP1 };
	CEdit		m_ctCamLayer;
	CEdit		m_ctOrgLayer;
	CButton		m_ctReload;
	CString		m_strCamLayer;
	CString		m_strOrgLayer;
	int			m_nOrgType;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CDxfSetup1)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CDxfSetup1)
	virtual BOOL OnInitDialog();
	afx_msg void OnReload();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
