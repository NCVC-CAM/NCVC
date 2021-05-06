
// TestProjectView.h : CTestProjectView クラスのインターフェイス
//

#pragma once


class CTestProjectView : public CView
{
protected: // シリアル化からのみ作成します。
	CTestProjectView();
	DECLARE_DYNCREATE(CTestProjectView)

// 属性
public:
	CTestProjectDoc* GetDocument() const;

// 操作
public:

// オーバーライド
public:
	virtual void OnDraw(CDC* pDC);  // このビューを描画するためにオーバーライドされます。
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// 実装
public:
	virtual ~CTestProjectView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成された、メッセージ割り当て関数
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // TestProjectView.cpp のデバッグ バージョン
inline CTestProjectDoc* CTestProjectView::GetDocument() const
   { return reinterpret_cast<CTestProjectDoc*>(m_pDocument); }
#endif

