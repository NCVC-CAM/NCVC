// NCViewBase.h: CNCViewBase �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ViewBase.h"

class CNCViewBase : public CViewBase
{
protected:
	CNCViewBase() {}
	virtual ~CNCViewBase() {}

protected:
	CString			m_strGuide;		// ���ʈē�����
	PFNNCDRAWPROC	m_pfnDrawProc;	// �`��֐��߲��

	// XY, XZ, YZ ���ʗp (XYZ��CNCView���Ǝ��Ɏ���)
	CPoint	m_ptGuide[2][2];	// ���̶޲�ލ��W(�n�_�E�I�_)
	CRect	m_rcDrawWork;		// ܰ���`
	CRect	m_rcDrawMax;		// �ް���`
	void	DrawWorkRect(CDC* pDC) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_WORK));
		pDC->Rectangle(m_rcDrawWork);
	}
	void	DrawMaxRect(CDC* pDC) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_MAXCUT));
		pDC->Rectangle(m_rcDrawMax);
	}

	void	OnInitialUpdate(int);
};
