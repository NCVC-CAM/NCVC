// MachineSetup.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "MachineSetup1.h"
#include "MachineSetup2.h"
#include "MachineSetup3.h"
#include "MachineSetup4.h"
#include "MachineSetup5.h"

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup

class CMachineSetup : public CPropertySheet
{
// �R���X�g���N�V����
public:
	CMachineSetup(LPCTSTR, LPCTSTR);

// �A�g���r���[�g
public:
	CString		m_strFileName;	// ���ݏ������̋@�B���̧�ٖ�
	BOOL		m_bReload,		// �ēǍ����K�v
				m_bCalcThread;	// �Čv�Z���K�v

	// �����߰���޲�۸�
	CMachineSetup1	m_dlg1;
	CMachineSetup2	m_dlg2;
	CMachineSetup3	m_dlg3;
	CMachineSetup4	m_dlg4;
	CMachineSetup5	m_dlg5;

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMachineSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CMachineSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
