// ViewBase.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewBase

class CViewBase : public CView
{
	int			m_nBoth;		// �����ݏ�������
								// �Ō�ɗ��������݂�ReleaseCapture()���邽��
	CPoint		m_ptBeforeOrg,	// ���O�̌��_
				m_ptMouse,		// ϳ��������ꂽ���޲����W
				m_ptMovOrg;		// Rϳ��������ꂽ�_�����W(�ړ���_)
	CRect		m_rcMagnify;	// �g���`(�����Ș_�����W)
	double		m_dBeforeFactor;// ���O�̊g�嗦

	int		OnMouseButtonUp(int, const CPoint&);	// ���݂𗣂����Ƃ��̋��ʏ���
	CSize	OnViewLens(CClientDC&);					// �g��k�����ʏ���

protected:
	CViewBase();
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	BOOL		m_bMagRect;		// �g���`�\����
	double		m_dFactor;		// �g�嗦
	int			m_nLState,		// -1:Up, 0:Down, 1:�g���`�\����
				m_nRState;		// -1:Up, 0:Down, 1:�E��ׯ�ވړ���

	// �W���ɂ�鐔�l�̕␳(�`����W)
	int		DrawConvert(double d) {
		return (int)(d * m_dFactor * LOMETRICFACTOR);
	}
	CPoint	DrawConvert(const CPointD& pt) {
		return CPoint( pt * m_dFactor * LOMETRICFACTOR );
	}
	CRect	DrawConvert(const CRectD& rc) {
		return CRect( DrawConvert(rc.TopLeft()), DrawConvert(rc.BottomRight()) );
	}

	// �g���`�̕`��
	void	DrawMagnifyRect(CDC* pDC) {
		pDC->SetROP2(R2_XORPEN);
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenCom(COMPEN_RECT));
		pDC->Rectangle(&m_rcMagnify);
		pDC->SelectObject(pOldPen);
		pDC->SetROP2(R2_COPYPEN);
	}

	// ��̧�ق�د���ް�ނɺ�߰
	void	CopyNCDrawForClipboard(HENHMETAFILE);

	// ү����ϯ�߂̋��ʕ���
	void	OnViewFit(const CRectD&, BOOL = TRUE);	// �}�`̨��
	void	OnViewLensP(void);						// �g��
	void	OnViewLensN(void);						// �k��
	void	OnMoveKey(UINT);						// �ړ�
	void	OnBeforeMagnify(void);					// �O��̊g�嗦

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
