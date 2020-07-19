#pragma once
#include "afxwin.h"


// COBSdlg �_�C�A���O

class COBSdlg : public CDialog
{
	void	GetOBSdata(void);
	void	SetOBSdata(void);

public:
	COBSdlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[

// �_�C�A���O �f�[�^
	enum { IDD = IDD_OBS };
	CString m_strMCFile;
	CButton m_ctOK;
	BOOL	m_bOBS[10];

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	afx_msg void OnAllON();
	afx_msg void OnAllOFF();
	afx_msg void OnObsWrite();
	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);
	afx_msg LRESULT OnUserSwitchMachine(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
