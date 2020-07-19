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

protected:
	DECLARE_DYNCREATE(C3dModelChild)
	DECLARE_MESSAGE_MAP()
};


