#pragma once

// CNCFindDlg ダイアログ

class CNCFindDlg : public CDialog
{
public:
	CNCFindDlg(CWnd* pParent = NULL);   // 標準コンストラクタ

// ダイアログ データ
	enum { IDD = IDD_NCVIEW_FIND };
	CEdit m_ctStrFind;
	CButton m_ctFind;
	CButton m_ctFindUp;
	CButton m_ctFindDown;
	int m_nUpDown;
	CString m_strFind;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
