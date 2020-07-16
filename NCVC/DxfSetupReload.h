// DxfSetupReload.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfSetupReload ダイアログ

class CDxfSetupReload : public CDialog
{
// コンストラクション
public:
	CDxfSetupReload(CWnd* pParent);

// ダイアログ データ
	//{{AFX_DATA(CDxfSetupReload)
	enum { IDD = IDD_DXF_SETUP_RELOAD };
		// メモ: ClassWizard はこの位置にデータ メンバを追加します。
	//}}AFX_DATA
	CCheckListBox	m_ctReloadList;	// 標準ﾘｽﾄﾎﾞｯｸｽを置換(ｻﾌﾞｸﾗｽ化)

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDxfSetupReload)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CDxfSetupReload)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
