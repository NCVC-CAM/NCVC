// AddinDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CAddinDlg �_�C�A���O

class CAddinDlg : public CDialog
{
	void	SetDetailData(CNCVCaddinIF*);

// �R���X�g���N�V����
public:
	CAddinDlg(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CAddinDlg)
	enum { IDD = IDD_ADDININFO };
	CEdit	m_ctAddnInfo;
	CEdit	m_ctReadMe;
	CListCtrl	m_ctList;
	CString	m_strAddinInfo;
	CString	m_strReadMe;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CAddinDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CAddinDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemChangedAddinList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
