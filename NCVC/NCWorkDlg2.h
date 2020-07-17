// NCWorkDlg1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg2 ダイアログ

class CNCWorkDlg2 : public CPropertyPage
{
	void	EnableButton(BOOL);
	void	SetValue(const CRect3D&);

public:
	CNCWorkDlg2();

	enum { IDD = IDD_NCVIEW_WORK2 };
	CButton		m_ctShow, m_ctHide,
				m_ctRecover, m_ctComment;
	CStatic		m_ctWorkLabel[2],
				m_ctOffsetLabel[NCXYZ];
	CFloatEdit	m_ctWork[2],
				m_ctOffset[NCXYZ];

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
