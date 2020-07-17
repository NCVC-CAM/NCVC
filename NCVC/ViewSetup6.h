// ViewSetup6.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup6 ダイアログ

class CViewSetup6 : public CPropertyPage
{

// コンストラクション
public:
	CViewSetup6();

// ダイアログ データ
	enum { IDD = IDD_VIEW_SETUP6 };
	int		m_nForceView01[4],
			m_nForceView02[4];

// オーバーライド
public:
	virtual BOOL OnApply();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	afx_msg void OnChange();
	afx_msg void OnDefColor();

	DECLARE_MESSAGE_MAP()
};
