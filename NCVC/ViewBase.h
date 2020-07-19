// ViewBase.h : �w�b�_�[ �t�@�C��
//

#pragma once

// ϳ�����̑J��
enum ENMOUSESTATE
{
	MS_UP = 0,
	MS_DOWN,
	MS_DRAG
};

// ϳ���ׯ�ގ��̍Œ�ړ���
#define	MAGNIFY_RANGE	10		// �߸��

/////////////////////////////////////////////////////////////////////////////
// CViewBase

class CViewBase : public CView
{
	int		m_nBoth;		// �����ݏ�������
							// �Ō�ɗ��������݂�ReleaseCapture()���邽��
	int		OnMouseButtonUp(ENMOUSESTATE, CPoint&);	// ���݂𗣂����Ƃ��̋��ʏ���
	CSize	OnViewLens(CClientDC&);		// �g��k�����ʏ���

protected:
	CViewBase();
	DECLARE_DYNAMIC(CViewBase)

	CPoint		m_ptMouse,		// ϳ��������ꂽ���޲����W
				m_ptMovOrg;		// Rϳ��������ꂽ�_�����W(�ړ���_)
	CRect		m_rcMagnify;	// �g���`(�����Ș_�����W)
	BOOL		m_bMagRect;		// �g���`�\����
	float		m_dFactor;		// �g�嗦(*LOMETRICFACTOR)
	ENMOUSESTATE	m_enLstate,
					m_enRstate;

	// �W���ɂ�鐔�l�̕␳(�`����W)
	int		DrawConvert(float d) {
		return (int)(d * m_dFactor);
	}
	CPoint	DrawConvert(const CPointF& pt) {
		return CPoint(pt * m_dFactor);
	}
	CRect	DrawConvert(const CRectF& rc) {
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
	void	OnViewFit(const CRectF&, BOOL = TRUE);	// �}�`̨��
	void	OnViewLensP(void);						// �g��
	void	OnViewLensN(void);						// �k��
	void	OnMoveKey(UINT);						// �ړ�
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
