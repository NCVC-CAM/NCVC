// ExtensionDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CExtensionDlg �_�C�A���O

class CExtensionDlg : public CDialog
{
// �R���X�g���N�V����
public:
	CExtensionDlg();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CExtensionDlg)
	enum { IDD = IDD_EXTENSION };
	//}}AFX_DATA
	CString		m_strExtTxt[2];	// ncd or dxf
	CListBox	m_ctExtList[2];
	CEdit		m_ctExtTxt[2];
	CButton		m_ctExtDelBtn[2];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CExtensionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CExtensionDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnExtAdd();
	afx_msg void OnExtDel();
	afx_msg void OnExtSelchangeList();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
