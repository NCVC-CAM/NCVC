// C3dModelChild �t���[��
//

#pragma once

#include "ChildBase.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelChild �t���[��

class C3dModelChild : public CChildBase
{
protected:
	C3dModelChild();           // ���I�����Ŏg�p����� protected �R���X�g���N�^�[
	virtual ~C3dModelChild();
	DECLARE_DYNCREATE(C3dModelChild)

	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);

	DECLARE_MESSAGE_MAP()
};
