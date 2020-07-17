// MCSetup.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "MCSetup1.h"
#include "MCSetup2.h"
#include "MCSetup3.h"
#include "MCSetup4.h"
#include "MCSetup5.h"

/////////////////////////////////////////////////////////////////////////////
// CMCSetup

class CMCSetup : public CPropertySheet
{
// �R���X�g���N�V����
public:
	CMCSetup(LPCTSTR, LPCTSTR);

// �A�g���r���[�g
public:
	CString		m_strFileName;	// ���ݏ������̋@�B���̧�ٖ�
	BOOL		m_bReload,		// �ēǍ����K�v
				m_bCalcThread;	// �Čv�Z���K�v

	// �����߰���޲�۸�
	CMCSetup1	m_dlg1;
	CMCSetup2	m_dlg2;
	CMCSetup3	m_dlg3;
	CMCSetup4	m_dlg4;
	CMCSetup5	m_dlg5;

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMCSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CMCSetup();

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CMCSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
