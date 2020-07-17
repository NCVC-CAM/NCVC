// NCWorkDlg.h : ヘッダー ファイル
//

#pragma once

#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg ダイアログ

class CNCWorkDlg : public CDialog
{
	void	EnableButton(BOOL bEnable) {
		m_ctOK.EnableWindow(bEnable);
		m_ctHide.EnableWindow(bEnable);
	}
	void	SaveValue(void);

// コンストラクション
public:
	CNCWorkDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CNCWorkDlg)
	enum { IDD = IDD_NCVIEW_WORK };
	CButton	m_ctHide;
	CButton	m_ctOK;
	//}}AFX_DATA
	CIntEdit	m_WorkNum[2][NCXYZ];
	CSpinButtonCtrl	m_Spin[2][NCXYZ];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CNCWorkDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CNCWorkDlg)
	afx_msg void OnHide();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
