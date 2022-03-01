// MakeNCSetup.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "MakeNCSetup1.h"
#include "MakeNCSetup2.h"
#include "MakeNCSetup3.h"
#include "MakeNCSetup4.h"
#include "MakeNCSetup5.h"
#include "MakeNCSetup6.h"
#include "MakeNCSetup8.h"
#include "NCMakeMillOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup

class CMakeNCSetup : public CPropertySheet
{
	CNCMakeMillOpt*		m_pNCMake;		// NC������߼��

// �R���X�g���N�V����
public:
	CMakeNCSetup(LPCTSTR, LPCTSTR);
	~CMakeNCSetup();
	DECLARE_DYNAMIC(CMakeNCSetup)

// �A�g���r���[�g
public:
	// �����߰���޲�۸�
	CMakeNCSetup1	m_dlg1;
	CMakeNCSetup2	m_dlg2;
	CMakeNCSetup3	m_dlg3;
	CMakeNCSetup4	m_dlg4;
	CMakeNCSetup5	m_dlg5;
	CMakeNCSetup6	m_dlg6;
	CMakeNCSetup8	m_dlg8;

	CNCMakeMillOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMakeNCSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CMakeNCSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
