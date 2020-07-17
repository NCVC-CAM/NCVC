// MakeNCDlgEx.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "MakeNCDlgEx2.h"
#include "MakeNCDlgEx3.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx

class CMakeNCDlgEx : public CPropertySheet
{
	UINT		m_nID;		// ����ID
	CDXFDoc*	m_pDoc;		// ڲԏ��擾
	
// �R���X�g���N�V����
public:
	CMakeNCDlgEx(UINT, CDXFDoc*);
	~CMakeNCDlgEx();

// �A�g���r���[�g
public:
	UINT		GetNCMakeID(void) {
		return m_nID;
	}
	CDXFDoc*	GetDocument(void) {
		return m_pDoc;
	}

	// �����߰���޲�۸�
	CMakeNCDlgEx2	m_dlg1;
	CMakeNCDlgEx3	m_dlg2;

	// �޲�۸ދ��ʍ�
	CString		m_strNCFileName,
				m_strInitFileName,
				m_strLayerToInitFileName;

	DECLARE_MESSAGE_MAP()
};
