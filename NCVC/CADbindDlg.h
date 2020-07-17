// CCADbindDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CCADbindDlg ダイアログ

class CCADbindDlg : public CDialog
{
	void	PathOptimizeAdd(const CString&);

public:
	CCADbindDlg(CWnd* pParent = NULL);   // 標準コンストラクター

// ダイアログ データ
	enum { IDD = IDD_CADBIND };
	CListBox	m_ctBindList;
	CFloatEdit	m_dBindWork[2],
				m_dBindMargin;
	int			m_nOrgType;

	CStringArray	m_aryFile;	// BindListに登録されるﾌﾙﾊﾟｽ

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnBindFileAdd();
	afx_msg void OnBindFileClr();
	afx_msg void OnBindFileAllClr();

	DECLARE_MESSAGE_MAP()
};
