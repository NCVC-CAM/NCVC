
// TestProjectView.cpp : CTestProjectView クラスの実装
//

#include "stdafx.h"
// SHARED_HANDLERS は、プレビュー、縮小版、および検索フィルター ハンドラーを実装している ATL プロジェクトで定義でき、
// そのプロジェクトとのドキュメント コードの共有を可能にします。
#ifndef SHARED_HANDLERS
#include "TestProject.h"
#endif

#include "TestProjectDoc.h"
#include "TestProjectView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestProjectView

IMPLEMENT_DYNCREATE(CTestProjectView, CView)

BEGIN_MESSAGE_MAP(CTestProjectView, CView)
END_MESSAGE_MAP()

// CTestProjectView コンストラクション/デストラクション

CTestProjectView::CTestProjectView()
{
	// TODO: 構築コードをここに追加します。

}

CTestProjectView::~CTestProjectView()
{
}

BOOL CTestProjectView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: この位置で CREATESTRUCT cs を修正して Window クラスまたはスタイルを
	//  修正してください。

	return CView::PreCreateWindow(cs);
}

// CTestProjectView 描画

void CTestProjectView::OnDraw(CDC* /*pDC*/)
{
	CTestProjectDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: この場所にネイティブ データ用の描画コードを追加します。
}


// CTestProjectView 診断

#ifdef _DEBUG
void CTestProjectView::AssertValid() const
{
	CView::AssertValid();
}

void CTestProjectView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTestProjectDoc* CTestProjectView::GetDocument() const // デバッグ以外のバージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTestProjectDoc)));
	return (CTestProjectDoc*)m_pDocument;
}
#endif //_DEBUG


// CTestProjectView メッセージ ハンドラー
