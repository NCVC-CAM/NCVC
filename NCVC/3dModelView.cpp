// 3dModelView.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "3dModelChild.h"
#include "3dModelDoc.h"
#include "3dModelView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// C3dModelView

IMPLEMENT_DYNCREATE(C3dModelView, CViewBase)

BEGIN_MESSAGE_MAP(C3dModelView, CViewBase)
END_MESSAGE_MAP()

C3dModelView::C3dModelView()
{
}

C3dModelView::~C3dModelView()
{
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelView クラスのオーバライド関数

#ifdef _DEBUG
C3dModelDoc* C3dModelView::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(C3dModelDoc)));
	return static_cast<C3dModelDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// C3dModelView 描画

void C3dModelView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: 描画コードをここに追加してください。
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelView メッセージ ハンドラー
