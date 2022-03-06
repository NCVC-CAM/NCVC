// C3dModelChild フレーム
//

#pragma once

#include "ChildBase.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelChild フレーム

class C3dModelChild : public CChildBase
{
protected:
	C3dModelChild();           // 動的生成で使用される protected コンストラクター
	virtual ~C3dModelChild();
	DECLARE_DYNCREATE(C3dModelChild)

	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);

	DECLARE_MESSAGE_MAP()
};
