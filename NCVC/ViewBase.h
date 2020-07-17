// ViewBase.h : ƒwƒbƒ_[ ƒtƒ@ƒCƒ‹
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewBase

class CViewBase
{
	int			m_nBoth;		// —¼ÎÞÀÝˆ—¶³ÝÀ
								// ÅŒã‚É—£‚µ‚½ÎÞÀÝ‚ÅReleaseCapture()‚·‚é‚½‚ß
	CPoint		m_ptBeforeOrg,	// ’¼‘O‚ÌŒ´“_
				m_ptMouse,		// Ï³½‚ª‰Ÿ‚³‚ê‚½ÃÞÊÞ²½À•W
				m_ptMovOrg;		// RÏ³½‚ª‰Ÿ‚³‚ê‚½˜_—À•W(ˆÚ“®Šî“_)
	CRect		m_rcMagnify;	// Šg‘å‹éŒ`(ƒˆ‚È˜_—À•W)
	double		m_dBeforeFactor;// ’¼‘O‚ÌŠg‘å—¦

	int		OnMouseButtonUp(int, CPoint&);	// ÎÞÀÝ‚ð—£‚µ‚½‚Æ‚«‚Ì‹¤’Êˆ—

protected:
	CViewBase();
	virtual	~CViewBase();

	CView*		m_pView;		// ”h¶ËÞ­°
	BOOL		m_bMagRect;		// Šg‘å‹éŒ`•\Ž¦’†
	double		m_dFactor;		// Šg‘å—¦
	int			m_nLState,		// -1:Up, 0:Down, 1:Šg‘å‹éŒ`•\Ž¦’†
				m_nRState;		// -1:Up, 0:Down, 1:‰EÄÞ×¯¸ÞˆÚ“®’†

	// ŒW”‚É‚æ‚é”’l‚Ì•â³(•`‰æÀ•W)
	int		DrawConvert(double d) {
		return (int)(d * m_dFactor * LOMETRICFACTOR);
	}
	CPoint	DrawConvert(const CPointD& ptd) {
		return CPoint( ptd * m_dFactor * LOMETRICFACTOR );
	}

	// Šg‘å‹éŒ`‚Ì•`‰æ
	void	DrawMagnifyRect(CDC* pDC) {
		pDC->SetROP2(R2_XORPEN);
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenCom(COMPEN_RECT));
		pDC->Rectangle(&m_rcMagnify);
		pDC->SelectObject(pOldPen);
		pDC->SetROP2(R2_COPYPEN);
	}

	// ÒÀÌ§²Ù‚ð¸Ø¯ÌßÎÞ°ÄÞ‚ÉºËß°
	void	CopyNCDrawForClipboard(HENHMETAFILE);

	// Ò¯¾°¼ÞÏ¯Ìß‚Ì‹¤’Ê•”•ª
	void	OnViewFit(const CRectD&);		// }Œ`Ì¨¯Ä
	void	OnViewLensP(void);				// Šg‘å
	void	OnViewLensN(void);				// k¬
	void	OnMoveKey(UINT);				// ˆÚ“®
	void	OnBeforeMagnify(void);			// ‘O‰ñ‚ÌŠg‘å—¦

	void	OnCreate(CView*, BOOL bDC = TRUE);
	void	OnLButtonDown(const CPoint&);
	int		OnLButtonUp(CPoint&);
	void	OnRButtonDown(const CPoint&);
	int		OnRButtonUp(CPoint&);
	BOOL	OnMouseMove(UINT, const CPoint&);
	BOOL	OnMouseWheel(UINT, short, const CPoint&);
	void	OnContextMenu(CPoint point, UINT nID) {
		CMenu	menu;
		menu.LoadMenu(nID);
		CMenu*	pMenu = menu.GetSubMenu(0);
		pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
			point.x, point.y, AfxGetMainWnd());
	}
};
