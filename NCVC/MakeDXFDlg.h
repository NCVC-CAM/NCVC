// MakeDXFDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "MakeDXFDlg1.h"
#include "MakeDXFDlg2.h"
#include "DXFMakeOption.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg

class CMakeDXFDlg : public CPropertySheet
{
	CNCDoc*			m_pDoc;
	CDXFMakeOption	m_dxfMake;		// DXF�o�͵�߼��

// �R���X�g���N�V����
public:
	CMakeDXFDlg(CNCDoc*);

// �A�g���r���[�g
public:
	// �����߰���޲�۸�
	CMakeDXFDlg1	m_dlg1;
	CMakeDXFDlg2	m_dlg2;

	CNCDoc*	GetDoc(void) {
		return m_pDoc;
	}
	CDXFMakeOption*	GetDXFMakeOption(void) {
		return &m_dxfMake;
	}

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMakeDXFDlg)
	public:
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CMakeDXFDlg();

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CMakeDXFDlg)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ����o�֐���ǉ��܂��͍폜���܂��B
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
