// MachineSetup4.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup4 �_�C�A���O

class CMachineSetup4 : public CPropertyPage
{
// �R���X�g���N�V����
public:
	CMachineSetup4();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMachineSetup4)
	enum { IDD = IDD_MC_SETUP4 };
	//}}AFX_DATA
	CString	m_strMacro[MCMACROSTRING];	// �Ăяo�����ށC̫��ށCI/F�C�o��̫��ށC�o�͖�
	CEdit	m_ctMacro[MCMACROSTRING];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMachineSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMachineSetup4)
	afx_msg void OnFolder();
	afx_msg void OnMacroIF();
	afx_msg void OnKillFocusMacroCode();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
