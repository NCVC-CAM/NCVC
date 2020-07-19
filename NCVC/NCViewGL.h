// NCViewGL.h : CNCViewGL �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "FrameBuffer.h"
#include "NCdata.h"
//#define	USE_SHADER
//#include "GLSL.h"

// TrackingMode
enum ENTRACKINGMODE
{
	TM_NONE, TM_SPIN, TM_PAN
};

// Range Parameter
struct RANGEPARAM
{
	INT_PTR		s, e;
	RANGEPARAM(INT_PTR ss, INT_PTR ee) {
		s = ss;
		e = ee;
	}
};

// CreateBoxel_fromIGES() argument
typedef	boost::variant<CNCdata*, RANGEPARAM>	CREATEBOXEL_IGESPARAM;

// GetClipDepthMill() argument
enum ENCLIPDEPTH
{
	DP_NoStencil, DP_BottomStencil, DP_TopStencil
};

// GetClipDepthMill_hoge() argument
struct CLIPDEPTHMILL
{
	GLdouble	wx, wy, wz;
	size_t		tp, bm;
};
typedef void (CNCViewGL::*PFNGETCLIPDEPTHMILL)(const CLIPDEPTHMILL&);

// ��ʕ`����W�����گ�ޗp
struct CREATEBOTTOMVERTEXPARAM
{
#ifdef _DEBUG
	int		dbgThread;		// �گ��ID
#endif
	CEvent		evStart,
				evEnd;
	BOOL		bThread,
				bResult;
	CNCDoc*		pDoc;
	size_t		s, e;
	CVBtmDraw	vBD;		// from NCdata.h
	// CEvent ���蓮����Ăɂ��邽�߂̺ݽ�׸�
	CREATEBOTTOMVERTEXPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE),
		bThread(TRUE), bResult(TRUE)
	{}
};
typedef	CREATEBOTTOMVERTEXPARAM*	LPCREATEBOTTOMVERTEXPARAM;

// ���_�z�񐶐��گ�ޗp
struct CREATEELEMENTPARAM
{
#ifdef _DEBUG
	int		dbgThread;
#endif
	CEvent		evStart,
				evEnd;
	const GLfloat*	pfXYZ;
	GLfloat*		pfNOR;
	const GLubyte*	pbStl;
	BOOL	bThread,
			bResult;
	int		cx, cy,
			cs, ce;
	GLfloat	h, l;
	// ���_�z����ޯ��(�ϒ�2�����z��)
	std::vector<CVelement>	vvElementCut,	// from NCdata.h
							vvElementWrk;
	// CEvent ���蓮����Ăɂ��邽�߂̺ݽ�׸�
	CREATEELEMENTPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE),
		bThread(TRUE), bResult(TRUE)
	{}
};
typedef	CREATEELEMENTPARAM*		LPCREATEELEMENTPARAM;

/////////////////////////////////////////////////////////////////////////////

class CNCViewGL : public CView
{
	CString		m_strGuide;
	BOOL		m_bActive,
				m_bSizeChg,
				m_bWirePath,	// �ޭ����Ƃɓ��I�ɐ؂�ւ����׸�
				m_bSlitView;
	int			m_cx,  m_cy,	// ����޳����(��ذ�)
				m_icx, m_icy,
				m_nLe;			// �����ް�����(m_nLe<=m_icx)
	GLint		m_wx, m_wy;		// glReadPixels, glWindowPos
	float		m_dRate,		// ��g�嗦
				m_dRoundAngle,	// �����݂̉�]�p�x
				m_dRoundStep;	// �����݂̂P�񂠂���̉�]�p�x
	CRect3F		m_rcView,		// ���ً��
				m_rcDraw;		// ܰ���`(�د�ޕ\���p)
	CPointF		m_ptCenter,		// �`�撆�S
				m_ptCenterBk,
				m_ptLastMove;	// �ړ��O���W
	CPoint3F	m_ptLastRound,	// ��]�O���W
				m_ptRoundBase;	// �����ݘA����]�̊���W
	CPoint		m_ptDownClick;	// ��÷���ƭ��\���p��
	HGLRC		m_hRC;
	GLuint		m_glCode;		// �؍��߽���ި���ڲؽ�

	GLfloat*	m_pfDepth;		// ���߽�l�擾�z��
#ifndef NO_TRACE_WORKFILE			// from NCViewTab.h
	GLfloat*	m_pfDepthBottom;// WorkFile 2Pass
#endif
	GLubyte*	m_pbStencil;	// ��ݼ�
	CFrameBuffer*	m_pFBO;		// FrameBufferObject
#ifdef USE_SHADER
	CGLSL			m_glsl;		// Shader Manager
#endif
	GLfloat*	m_pfXYZ;		// -- �ϊ����ꂽܰ��ލ��W(temp area)
	GLfloat*	m_pfNOR;		// -- �@���޸��
	GLfloat*	m_pLatheX;		// -- ���Ղ�X�l
	GLfloat*	m_pLatheZ;		// -- ���Ղ�Z�l
	GLsizeiptr	m_nVBOsize;		// ���_�z�񻲽�
	GLuint		m_nVertexID[2],	// ���_�z��Ɩ@���޸�ٗp
				m_nTextureID,	// ø������W�p
				m_nPictureID;	// ø����摜�p
	GLuint*		m_pSolidElement;// ���_���ޯ���p
	GLuint*		m_pLocusElement;// �O�ղ��ޯ���p
	CVelement	m_vElementWrk,	// ܰ���`�pglDrawElements���_��
				m_vElementCut,	// �؍�ʗp
				m_vElementEdg,	// ���Ւ[�ʗp
				m_vElementSlt;	// ���Ւf�ʗp
	GLsizei		GetElementSize(void) const {
		return (GLsizei)(m_vElementWrk.size()+m_vElementCut.size()+m_vElementEdg.size()+m_vElementSlt.size());
	}
	WIREDRAW	m_WireDraw;		// ܲԉ��H�@�p

	struct {	// CreateElementThread() from CreateVBOMill()
		DWORD	m_nCeProc;
		HANDLE*	m_pCeHandle;
		LPCREATEELEMENTPARAM	m_pCeParam;
	};

	ENTRACKINGMODE	m_enTrackingMode;
	GLdouble		m_objXform[4][4],
					m_objXformBk[4][4];	// �޸�ُ��������ޯ�����

	void	ClearObjectForm(BOOL = FALSE);
	BOOL	SetupPixelFormat(CDC*);
	void	UpdateViewOption(void);
	void	CreateDisplayList(void);
	BOOL	CreateBoxel(BOOL = FALSE);
	BOOL	CreateBoxel_fromIGES(CREATEBOXEL_IGESPARAM* = NULL);
	BOOL	CreateLathe(BOOL = FALSE);
	BOOL	CreateWire(void);
	BOOL	CreateBottomFaceThread(BOOL, int);
	BOOL	GetClipDepthMill(ENCLIPDEPTH = DP_NoStencil);
	void	GetClipDepthMill_All(const CLIPDEPTHMILL&);
	void	GetClipDepthMill_Zonly(const CLIPDEPTHMILL&);
	void	GetClipDepthMill_BottomStencil(const CLIPDEPTHMILL&);
	void	GetClipDepthMill_TopStencil(const CLIPDEPTHMILL&);
	BOOL	GetClipDepthCylinder(void);
	BOOL	GetClipDepthLathe(void);
	BOOL	CreateVBOMill(void);
	BOOL	CreateVBOLathe(void);
	BOOL	ReadTexture(LPCTSTR);
	void	CreateTextureMill(void);
	void	CreateTextureLathe(void);
	void	CreateTextureWire(void);
	void	CreateTexture(GLsizeiptr, const GLfloat*);
	void	ClearVBO(void);
	void	ClearTexture(void);
	void	CreateFBO(void);
	void	InitialBoxel(void);
	void	FinalBoxel(void);
	void	EndOfCreateElementThread(void);

	void	RenderBack(void);
	void	RenderAxis(void);
	void	RenderCode(void);
	void	RenderMill(const CNCdata*);

	CPoint3F	PtoR(const CPoint& pt);
	void	BeginTracking(const CPoint&, ENTRACKINGMODE);
	void	EndTracking(void);
	void	DoTracking(const CPoint&);
	void	DoScale(int);
	void	DoRotation(float);
	void	SetupViewingTransform(void);
#ifdef _DEBUG
	void	DumpDepth(void) const;
	void	DumpStencil(void) const;
	void	DumpLatheZ(void) const;
#endif

protected:
	CNCViewGL();
	DECLARE_DYNCREATE(CNCViewGL)

public:
	virtual ~CNCViewGL();
	CNCDoc*	GetDocument();
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC*);
	virtual BOOL OnCmdMsg(UINT, int, void*, AFX_CMDHANDLERINFO*);
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView*, LPARAM, CObject*);
	virtual void OnActivateView(BOOL, CView*, CView*);

public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	// CNCViewTab::OnActivatePage() ���� SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// �ƭ������
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveRoundKey(CCmdUI* pCmdUI);
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnRoundKey(UINT);
	afx_msg	void OnLensKey(UINT);
	afx_msg void OnDefViewInfo();
	//
	afx_msg LRESULT OnSelectTrace(WPARAM, LPARAM);	// from NCViewTab.cpp

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

void	OutputGLErrorMessage(GLenum, UINT);
void	InitialMillNormal(void);	// from CNCVCApp::CNCVCApp()

#ifndef _DEBUG
inline CNCDoc* CNCViewGL::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
