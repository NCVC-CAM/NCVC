// NCViewBase.h: CNCViewBase クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ViewBase.h"

class CNCViewBase : public CViewBase
{
protected:
	CNCViewBase() {}
	virtual ~CNCViewBase() {}

protected:
	CString			m_strGuide;		// 平面案内文字
	PFNNCDRAWPROC	m_pfnDrawProc;	// 描画関数ﾎﾟｲﾝﾀ

	// XY, XZ, YZ 平面用 (XYZはCNCViewが独自に持つ)
	CPoint	m_ptGuide[2][2];	// 軸のｶﾞｲﾄﾞ座標(始点・終点)
	CRect	m_rcDrawWork;		// ﾜｰｸ矩形
	CRect	m_rcDrawMax;		// ﾃﾞｰﾀ矩形
	void	DrawWorkRect(CDC* pDC) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_WORK));
		pDC->Rectangle(m_rcDrawWork);
	}
	void	DrawMaxRect(CDC* pDC) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_MAXCUT));
		pDC->Rectangle(m_rcDrawMax);
	}

	void	OnInitialUpdate(int);
};
