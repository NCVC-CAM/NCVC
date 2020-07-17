// CCADbindDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CCADbindDlg �_�C�A���O

class CCADbindDlg : public CDialog
{
	void	PathOptimizeAdd(const CString&);

public:
	CCADbindDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[

// �_�C�A���O �f�[�^
	enum { IDD = IDD_CADBIND };
	CListBox	m_ctBindList;
	CFloatEdit	m_dBindWork[2],
				m_dBindMargin;
	int			m_nOrgType;

	CStringArray	m_aryFile;	// BindList�ɓo�^��������߽

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnBindFileAdd();
	afx_msg void OnBindFileClr();
	afx_msg void OnBindFileAllClr();

	DECLARE_MESSAGE_MAP()
};
