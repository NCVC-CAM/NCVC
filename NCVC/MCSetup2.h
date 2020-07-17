// MCSetup2.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMCSetup2 ダイアログ

class CMCSetup2 : public CPropertyPage
{
// コンストラクション
public:
	CMCSetup2();

// ダイアログ データ
	//{{AFX_DATA(CMCSetup2)
	enum { IDD = IDD_MC_SETUP2 };
		// メモ - ClassWizard はこの位置にデータ メンバを追加します。
		//    この位置に生成されるコードを編集しないでください。
	//}}AFX_DATA
	CFloatEdit	m_dInitialXYZ[NCXYZ];
	CFloatEdit	m_dWorkOffset[WORKOFFSET][NCXYZ];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMCSetup2)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMCSetup2)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
