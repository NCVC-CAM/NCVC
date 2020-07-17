// NCWorkDlg1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg1 ダイアログ

class CNCWorkDlg1 : public CPropertyPage
{
	void	EnableButton(BOOL, BOOL);
	void	SetValue(const CNCDoc*, const CRect3F&);

public:
	CNCWorkDlg1();

	enum { IDD = IDD_NCVIEW_WORK1 };
	CButton		m_ctShow, m_ctHide,
				m_ctRecover, m_ctComment;
	CStatic		m_ctLabel[2][NCXYZ];
	CFloatEdit	m_ctWork[2][NCXYZ];

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnShow();
	afx_msg void OnHide();
	afx_msg void OnRecover();
	afx_msg void OnComment();
	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
