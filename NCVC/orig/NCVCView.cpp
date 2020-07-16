// NCVCView.cpp : CNCVCView クラスの実装
//

#include "stdafx.h"
#include "NCVC.h"

#include "NCVCDoc.h"
#include "NCVCView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNCVCView

IMPLEMENT_DYNCREATE(CNCVCView, CView)

BEGIN_MESSAGE_MAP(CNCVCView, CView)
END_MESSAGE_MAP()

// CNCVCView コンストラクション/デストラクション

CNCVCView::CNCVCView()
{
	// TODO: 構築コードをここに追加します。

}

CNCVCView::~CNCVCView()
{
}

BOOL CNCVCView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: この位置で CREATESTRUCT cs を修正して Window クラスまたはスタイルを
	//  修正してください。

	return CView::PreCreateWindow(cs);
}

// CNCVCView 描画

void CNCVCView::OnDraw(CDC* /*pDC*/)
{
	CNCVCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: この場所にネイティブ データ用の描画コードを追加します。
}


// CNCVCView 診断

#ifdef _DEBUG
void CNCVCView::AssertValid() const
{
	CView::AssertValid();
}

void CNCVCView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCVCDoc* CNCVCView::GetDocument() const // デバッグ以外のバージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCVCDoc)));
	return (CNCVCDoc*)m_pDocument;
}
#endif //_DEBUG


// CNCVCView メッセージ ハンドラ
