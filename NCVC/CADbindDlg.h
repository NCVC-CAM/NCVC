// CCADbindDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once
#include "afxwin.h"

struct BINDFILE {
	int		num;
	CString	strFile;
};

/////////////////////////////////////////////////////////////////////////////
// CCADbindDlg �_�C�A���O

class CCADbindDlg : public CDialog
{
	void	PathOptimizeAdd(const CString&);

public:
	CCADbindDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[

// �_�C�A���O �f�[�^
	enum { IDD = IDD_CADBIND };
	CButton		m_ctMarge;
	CListCtrl	m_ctBindList;
	CFloatEdit	m_dBindWork[2],
				m_dBindMargin;
	CEdit*		m_ctBindEdit;	// ؽĺ��۰ٕҏW
	int			m_nOrgType;
	BOOL		m_bMarge;

	//	CTypedPtrArray���Ɨv�f��delete���ݸނ����
	std::vector<BINDFILE>	m_arBind;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
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
