// MakeDXFDlg.h : ヘッダー ファイル
//

#pragma once

#include "MakeDXFDlg1.h"
#include "MakeDXFDlg2.h"
#include "DXFMakeOption.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg

class CMakeDXFDlg : public CPropertySheet
{
	CNCDoc*			m_pDoc;
	CDXFMakeOption	m_dxfMake;		// DXF出力ｵﾌﾟｼｮﾝ

// コンストラクション
public:
	CMakeDXFDlg(CNCDoc*);

// アトリビュート
public:
	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMakeDXFDlg1	m_dlg1;
	CMakeDXFDlg2	m_dlg2;

	CNCDoc*	GetDoc(void) {
		return m_pDoc;
	}
	CDXFMakeOption*	GetDXFMakeOption(void) {
		return &m_dxfMake;
	}

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeDXFDlg)
	public:
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CMakeDXFDlg();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CMakeDXFDlg)
		// メモ - ClassWizard はこの位置にメンバ関数を追加または削除します。
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
