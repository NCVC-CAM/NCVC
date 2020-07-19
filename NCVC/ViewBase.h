// ViewBase.h : ヘッダー ファイル
//

#pragma once

// ﾏｳｽ操作の遷移
enum ENMOUSESTATE
{
	MS_UP = 0,
	MS_DOWN,
	MS_DRAG
};

// ﾏｳｽﾄﾞﾗｯｸﾞ時の最低移動量
#define	MAGNIFY_RANGE	10		// ﾋﾟｸｾﾙ

/////////////////////////////////////////////////////////////////////////////
// CViewBase

class CViewBase : public CView
{
	int		m_nBoth;		// 両ﾎﾞﾀﾝ処理ｶｳﾝﾀ
							// 最後に離したﾎﾞﾀﾝでReleaseCapture()するため
	int		OnMouseButtonUp(ENMOUSESTATE, CPoint&);	// ﾎﾞﾀﾝを離したときの共通処理
	CSize	OnViewLens(CClientDC&);		// 拡大縮小共通処理

protected:
	CViewBase();
	DECLARE_DYNAMIC(CViewBase)

	CPoint		m_ptMouse,		// ﾏｳｽが押されたﾃﾞﾊﾞｲｽ座標
				m_ptMovOrg;		// Rﾏｳｽが押された論理座標(移動基点)
	CRect		m_rcMagnify;	// 拡大矩形(純粋な論理座標)
	BOOL		m_bMagRect;		// 拡大矩形表示中
	float		m_dFactor;		// 拡大率(*LOMETRICFACTOR)
	ENMOUSESTATE	m_enLstate,
					m_enRstate;

	// 係数による数値の補正(描画座標)
	int		DrawConvert(float d) {
		return (int)(d * m_dFactor);
	}
	CPoint	DrawConvert(const CPointF& pt) {
		return CPoint(pt * m_dFactor);
	}
	CRect	DrawConvert(const CRectF& rc) {
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
	void	OnViewFit(const CRectF&, BOOL = TRUE);	// 図形ﾌｨｯﾄ
	void	OnViewLensP(void);						// 拡大
	void	OnViewLensN(void);						// 縮小
	void	OnMoveKey(UINT);						// 移動
	//
	int		_OnLButtonUp(CPoint&);
	int		_OnRButtonUp(CPoint&);
	BOOL	_OnMouseMove(UINT, const CPoint&);
	void	_OnContextMenu(CPoint pt, UINT nID);
	BOOL	_OnEraseBkgnd(CDC*, COLORREF, COLORREF);
	//
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//
	virtual	void	OnViewLensComm(void) {}

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	DECLARE_MESSAGE_MAP()
};
