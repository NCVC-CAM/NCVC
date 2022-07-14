// 3dModelView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBaseGL.h"
#include "Kodatuno/BODY.h"
#include "Kodatuno/Describe_BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)

/////////////////////////////////////////////////////////////////////////////
// C3dModelView ビュー

class C3dModelView : public CViewBaseGL
{
	int			m_icx, m_icy;		// FBOを作った時のサイズ
	NURBSC*		m_pSelCurve;		// 選択した曲線
	NURBSS*		m_pSelFace;			// 選択した曲面

	void	DrawBody(RENDERMODE);
	void	DrawRoughPath(void);
	void	DrawContourPath(void);
	void	DoSelect(const CPoint&);
	NURBSC*	DoSelectCurve(const CPoint&);
	NURBSS*	DoSelectFace (const CPoint&);
	void	SetKodatunoColor(DispStat&, COLORREF);

protected:
	C3dModelView();
	DECLARE_DYNCREATE(C3dModelView)

public:
	virtual ~C3dModelView();
	C3dModelDoc*	GetDocument();
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg	void OnLensKey(UINT);
	afx_msg void OnUpdateFile3dRough(CCmdUI* pCmdUI);
	afx_msg void OnFile3dRough();
	afx_msg void OnUpdateFile3dSmooth(CCmdUI* pCmdUI);
	afx_msg void OnFile3dSmooth();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline C3dModelDoc* C3dModelView::GetDocument()
   { return static_cast<C3dModelDoc *>(m_pDocument); }
#endif
