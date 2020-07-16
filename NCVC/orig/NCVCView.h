// NCVCView.h : CNCVCView クラスのインターフェイス
//


#pragma once


class CNCVCView : public CView
{
protected: // シリアル化からのみ作成します。
	CNCVCView();
	DECLARE_DYNCREATE(CNCVCView)

// 属性
public:
	CNCVCDoc* GetDocument() const;

// 操作
public:

// オーバーライド
public:
	virtual void OnDraw(CDC* pDC);  // このビューを描画するためにオーバーライドされます。
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// 実装
public:
	virtual ~CNCVCView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成された、メッセージ割り当て関数
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // NCVCView.cpp のデバッグ バージョン
inline CNCVCDoc* CNCVCView::GetDocument() const
   { return reinterpret_cast<CNCVCDoc*>(m_pDocument); }
#endif

