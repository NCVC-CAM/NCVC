// NCViewGL.h : CNCViewGL �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ViewBaseGL.h"
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
	RANGEPARAM(INT_PTR ss, INT_PTR ee) : s(ss), e(ee) {}
};

#ifdef USE_KODATUNO
// CreateBoxel_fromIGES() argument
class	CNCdata;
typedef	boost::variant<CNCdata*, RANGEPARAM>	CREATEBOXEL_IGESPARAM;
#endif

// GetClipDepthMill() argument
enum ENCLIPDEPTH
{
	DP_NoStencil, DP_BottomStencil, DP_TopStencil
};

// GetClipDepthXXX() argument
struct ARGVCLIPDEPTH
{
	GLdouble	wx, wy, wz;
	size_t		tp, bm;
};
typedef void (CNCViewGL::*PFNGETCLIPDEPTHMILL)(const ARGVCLIPDEPTH&);

// ��ʕ`����W�����گ�ޗp
struct CREATEBOTTOMVERTEXPARAM
{
	// CEvent���g����Win7�ňُ�I���B�Q�ƶ��Ă��������H
	// �d���Ȃ��̂Ō��n�I�ȕ��@���̗p
	size_t		nID;		// �گ��ID
	BOOL		bThread,
				bResult;
	CNCDoc*		pDoc;
	size_t		s, e;
	CVBtmDraw	vBD;		// from NCdata.h
	CREATEBOTTOMVERTEXPARAM(size_t nID, CNCDoc* pDoc) : nID(nID), pDoc(pDoc), bThread(TRUE), bResult(TRUE) {}
};
typedef	CREATEBOTTOMVERTEXPARAM*	LPCREATEBOTTOMVERTEXPARAM;

// ���_�z�񐶐��گ�ޗp
struct CREATEELEMENTPARAM
{
#ifdef _DEBUG
	int		dbgID;
#endif
	CEvent		evStart;	// ����
	CEvent		evEnd;		// �蓮
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
	CREATEELEMENTPARAM() : evEnd(FALSE, TRUE), bThread(TRUE), bResult(TRUE) {}
};
typedef	CREATEELEMENTPARAM*		LPCREATEELEMENTPARAM;

/////////////////////////////////////////////////////////////////////////////

class CNCViewGL : public CViewBaseGL
{
	CString		m_strGuide;
	BOOL		m_bActive,
				m_bSizeChg,
				m_bWirePath,	// �ޭ����Ƃɓ��I�ɐ؂�ւ����׸�
				m_bSlitView;
	int			m_icx, m_icy;	// glDrawPixels
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
	GLfloat*	m_pLatheZo;		// -- ���Ղ�Z�l�O�a
	GLfloat*	m_pLatheZi;		// -- ���Ղ�Z�l���a
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
	void	UpdateViewOption(void);
	void	CreateDisplayList(void);
	BOOL	CreateBoxel(BOOL = FALSE);
#ifdef USE_KODATUNO
	BOOL	CreateBoxel_fromIGES(CREATEBOXEL_IGESPARAM* = NULL);
#endif
	BOOL	CreateLathe(BOOL = FALSE);
	BOOL	CreateWire(void);
	BOOL	CreateBottomFaceThread(BOOL, int);
	BOOL	GetClipDepthMill(ENCLIPDEPTH = DP_NoStencil);
	void	GetClipDepthMill_All(const ARGVCLIPDEPTH&);
	void	GetClipDepthMill_Zonly(const ARGVCLIPDEPTH&);
	void	GetClipDepthMill_BottomStencil(const ARGVCLIPDEPTH&);
	void	GetClipDepthMill_TopStencil(const ARGVCLIPDEPTH&);
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
	void	DeleteDepthMemory(void);
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
	virtual void OnUpdate(CView*, LPARAM, CObject*);
	virtual void OnActivateView(BOOL, CView*, CView*);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
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
