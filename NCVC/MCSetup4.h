// MCSetup4.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMCSetup4 �_�C�A���O

class CMCSetup4 : public CPropertyPage
{
// �R���X�g���N�V����
public:
	CMCSetup4();
	~CMCSetup4();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMCSetup4)
	enum { IDD = IDD_MC_SETUP4 };
	//}}AFX_DATA
	CString	m_strMacro[MCMACROSTRING];	// �Ăяo�����ށC̫��ށCI/F�C�o��̫��ށC�o�͖�
	CEdit	m_ctMacro[MCMACROSTRING];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMCSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMCSetup4)
	afx_msg void OnFolder();
	afx_msg void OnMacroIF();
	afx_msg void OnKillFocusMacroCode();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
