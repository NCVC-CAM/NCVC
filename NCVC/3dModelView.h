// 3dModelView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBase.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelView ビュー

class C3dModelView : public CViewBase
{

protected:
	C3dModelView();           // 動的生成で使用される protected コンストラクター
	virtual ~C3dModelView();

public:
	C3dModelDoc*	GetDocument();
	virtual void OnDraw(CDC* pDC);      // このビューを描画するためにオーバーライドされます。

protected:
	DECLARE_DYNCREATE(C3dModelView)
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline C3dModelDoc* C3dModelView::GetDocument()
   { return static_cast<C3dModelDoc *>(m_pDocument); }
#endif
