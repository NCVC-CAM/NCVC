#pragma once

// CNCFindDlg �_�C�A���O

class CNCFindDlg : public CDialog
{
public:
	CNCFindDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^

// �_�C�A���O �f�[�^
	enum { IDD = IDD_NCVIEW_FIND };
	CEdit m_ctStrFind;
	CButton m_ctFind,
			m_ctFindUp,
			m_ctFindDown;
	int m_nUpDown;
	CString m_strFind, m_strFindOK;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
