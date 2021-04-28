// CCADbindDlg.h : ヘッダー ファイル
//

#pragma once
#include "afxwin.h"

struct BINDFILE {
	int		num;
	CString	strFile;
};

/////////////////////////////////////////////////////////////////////////////
// CCADbindDlg ダイアログ

class CCADbindDlg : public CDialog
{
	void	PathOptimizeAdd(const CString&);

public:
	CCADbindDlg(CWnd* pParent = NULL);   // 標準コンストラクター

// ダイアログ データ
	enum { IDD = IDD_CADBIND };
	CButton		m_ctMarge;
	CListCtrl	m_ctBindList;
	CFloatEdit	m_dBindWork[2],
				m_dBindMargin;
	CEdit*		m_ctBindEdit;	// ﾘｽﾄｺﾝﾄﾛｰﾙ編集
	int			m_nOrgType;
	BOOL		m_bMarge;

	//	CTypedPtrArrayだと要素のdeleteﾀｲﾐﾝｸﾞが難しい
	std::vector<BINDFILE>	m_arBind;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnBindFileAdd();
	afx_msg void OnBindFileClr();
	afx_msg void OnBindFileAllClr();
	afx_msg void OnNMDblclkBindList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()
};
