// CMakeBindOptDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeBindOptDlg ダイアログ

class CMakeBindOptDlg : public CDialog
{
public:
	CMakeBindOptDlg(CWnd* pParent = NULL);   // 標準コンストラクター

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAKEBINDOPT };
#endif
	int		m_nSort;
	BOOL	m_bFileComment;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
