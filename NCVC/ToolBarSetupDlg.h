// ToolBarSetupDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CToolBarSetupDlg �_�C�A���O

class CToolBarSetupDlg : public CDialog
{
// �R���X�g���N�V����
public:
	CToolBarSetupDlg(DWORD);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CToolBarSetupDlg)
	enum { IDD = IDD_TOOLBAR };
	//}}AFX_DATA

	// ������ MainFrm.cpp �� g_tbInfo �ɂ��
	BOOL	m_bToolBar[7];	// Main, Trace, MakeNCD, Shape, Addin, Exec, Machine

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CToolBarSetupDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CToolBarSetupDlg)
	afx_msg void OnToolBarCustomize();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
