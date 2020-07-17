// ViewSetup.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ViewSetup1.h"
#include "ViewSetup2.h"
#include "ViewSetup3.h"
#include "ViewSetup4.h"
#include "ViewSetup5.h"

/////////////////////////////////////////////////////////////////////////////
// CViewSetup

class CViewSetup : public CPropertySheet
{
// �R���X�g���N�V����
public:
	CViewSetup(void);

// �A�g���r���[�g
public:
	// �����߰���޲�۸�
	CViewSetup1	m_dlg1;
	CViewSetup2	m_dlg2;
	CViewSetup3	m_dlg3;
	CViewSetup4	m_dlg4;
	CViewSetup5	m_dlg5;

// �I�y���[�V����
public:
	CString	GetChangeFontButtonText(LOGFONT*);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CViewSetup)
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CViewSetup();

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CViewSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
