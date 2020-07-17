// ViewBase.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewBase

class CViewBase : public CView
{
	int			m_nBoth;		// 両ﾎﾞﾀﾝ処理ｶｳﾝﾀ
								// 最後に離したﾎﾞﾀﾝでReleaseCapture()するため
	CPoint		m_ptBeforeOrg,	// 直前の原点
				m_ptMouse,		// ﾏｳｽが押されたﾃﾞﾊﾞｲｽ座標
				m_ptMovOrg;		// Rﾏｳｽが押された論理座標(移動基点)
	CRect		m_rcMagnify;	// 拡大矩形(純粋な論理座標)
	double		m_dBeforeFactor;// 直前の拡大率

	int		OnMouseButtonUp(int, const CPoint&);	// ﾎﾞﾀﾝを離したときの共通処理
	CSize	OnViewLens(CClientDC&);					// 拡大縮小共通処理

protected:
	CViewBase();
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	BOOL		m_bMagRect;		// 拡大矩形表示中
	double		m_dFactor;		// 拡大率
	int			m_nLState,		// -1:Up, 0:Down, 1:拡大矩形表示中
				m_nRState;		// -1:Up, 0:Down, 1:右ﾄﾞﾗｯｸﾞ移動中

	// 係数による数値の補正(描画座標)
	int		DrawConvert(double d) {
		return (int)(d * m_dFactor * LOMETRICFACTOR);
	}
	CPoint	DrawConvert(const CPointD& pt) {
		return CPoint( pt * m_dFactor * LOMETRICFACTOR );
	}
	CRect	DrawConvert(const CRectD& rc) {
		return CRect( DrawConvert(rc.TopLeft()), DrawConvert(rc.BottomRight()) );
	}

	// 拡大矩形の描画
	void	DrawMagnifyRect(CDC* pDC) {
		pDC->SetROP2(R2_XORPEN);
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenCom(COMPEN_RECT));
		pDC->Rectangle(&m_rcMagnify);
		pDC->SelectObject(pOldPen);
		pDC->SetROP2(R2_COPYPEN);
	}

	// ﾒﾀﾌｧｲﾙをｸﾘｯﾌﾟﾎﾞｰﾄﾞにｺﾋﾟｰ
	void	CopyNCDrawForClipboard(HENHMETAFILE);

	// ﾒｯｾｰｼﾞﾏｯﾌﾟの共通部分
	void	OnViewFit(const CRectD&, BOOL = TRUE);	// 図形ﾌｨｯﾄ
	void	OnViewLensP(void);						// 拡大
	void	OnViewLensN(void);						// 縮小
	void	OnMoveKey(UINT);						// 移動
	void	OnBeforeMagnify(void);					// 前回の拡大率

	int		OnCreate(LPCREATESTRUCT, BOOL bDC = TRUE);
	void	OnLButtonDown(const CPoint&);
	int		OnLButtonUp(const CPoint&);
	void	OnRButtonDown(const CPoint&);
	int		OnRButtonUp(const CPoint&);
	BOOL	OnMouseMove(UINT, const CPoint&);
	BOOL	OnMouseWheel(UINT, short, const CPoint&);
	void	OnContextMenu(CPoint pt, UINT nID);
	BOOL	OnEraseBkgnd(CDC*, COLORREF, COLORREF);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};
