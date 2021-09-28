// DxfSetup.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DxfSetup1.h"
#include "DxfSetup2.h"
#include "DxfSetup3.h"

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup

class CDxfSetup : public CPropertySheet
{
// �R���X�g���N�V����
public:
	CDxfSetup(UINT);

// �A�g���r���[�g
public:
	// �����߰���޲�۸�
	CDxfSetup1	m_dlg1;
	CDxfSetup2	m_dlg2;
	CDxfSetup3	m_dlg3;

// �I�y���[�V����
public:
	BOOL	OnReload(CPropertyPage*);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDxfSetup)
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CDxfSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
