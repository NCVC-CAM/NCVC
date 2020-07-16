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
	// XY, XZ, YZ ���ʗp (XYZ��CNCView���Ǝ��Ɏ���)
	CPoint	m_ptGuid[2][2];		// ���̶޲�ލ��W(�n�_�E�I�_)
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

public:
/*
	�������z�֐��ɂ��� CTraceThread::InitInstance() ����
	���I�Ɋe��޼ު�Ă� DrawData() ���Ăяo���������C
	CView ����̔h���N���X�� "���I����(DECLARE_DYNCREATE)" �̂���
	CNCViewBase �����܂������Ȃ�??
*/
//	virtual	void	DrawData(CDC*, CNCdata*) = 0;
};
