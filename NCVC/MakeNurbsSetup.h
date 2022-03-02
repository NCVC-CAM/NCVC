// MakeNurbsSetup.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "MakeNCSetup1.h"
#include "MakeNCSetup2.h"
#include "MakeNCSetup6.h"
#include "NCMakeMillOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeNurbsSetup

class CMakeNurbsSetup : public CPropertySheet
{
	CNCMakeMillOpt*		m_pNCMake;		// NC������߼��

// �R���X�g���N�V����
public:
	CMakeNurbsSetup(LPCTSTR, LPCTSTR);
	virtual ~CMakeNurbsSetup();
	DECLARE_DYNAMIC(CMakeNurbsSetup)	// RUNTIME_CLASS�}�N���p

// �A�g���r���[�g
public:
	// �����߰���޲�۸�
	CMakeNCSetup1	m_dlg1;
	CMakeNCSetup2	m_dlg2;
	CMakeNCSetup6	m_dlg6;

	CNCMakeMillOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMakeNurbsSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CMakeNurbsSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
