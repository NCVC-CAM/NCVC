// NCWorkDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg �_�C�A���O

class CNCWorkDlg : public CDialog
{
	void	EnableButton(BOOL, BOOL);
	void	SetValue(const CNCDoc*, const CRect3D&);

// �R���X�g���N�V����
public:
	CNCWorkDlg(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CNCWorkDlg)
	enum { IDD = IDD_NCVIEW_WORK };
	CButton	m_ctOK;
	CButton	m_ctHide;
	CButton m_ctRecover;
	//}}AFX_DATA
	CStatic		m_ctLabel[2][NCXYZ];
	CFloatEdit	m_ctWork[2][NCXYZ];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CNCWorkDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CNCWorkDlg)
	afx_msg void OnHide();
	afx_msg void OnRecover();
	//}}AFX_MSG
	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
