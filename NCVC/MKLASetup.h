// MKNCSetup.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "MKLASetup0.h"
#include "MKLASetup1.h"
#include "MKLASetup2.h"
#include "MKLASetup3.h"
#include "MKLASetup4.h"
#include "MKNCSetup2.h"
#include "MKNCSetup6.h"
#include "NCMakeLatheOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup

class CMKLASetup : public CPropertySheet
{
	CNCMakeLatheOpt*	m_pNCMake;		// NC������߼��

public:
	CMKLASetup(LPCTSTR, LPCTSTR);
	virtual ~CMKLASetup();
	DECLARE_DYNAMIC(CMKLASetup)

	// �����߰���޲�۸�
	CMKLASetup0	m_dlg0;		// ��{
	CMKNCSetup2	m_dlg1;		// ����
	CMKNCSetup6	m_dlg2;		// �\�L
	CMKLASetup4	m_dlg3;		// �[��
	CMKLASetup2	m_dlg4;		// ����
	CMKLASetup3	m_dlg5;		// ���a
	CMKLASetup1	m_dlg6;		// �O�a

	CNCMakeLatheOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

	CMKLASetup3*	GetInsideDialog(void) {
		return &m_dlg5;
	}
	CMKLASetup1*	GetOutsideDialog(void) {
		return &m_dlg6;
	}

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
