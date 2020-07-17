// NCDoc.h : CNCDoc �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "DocBase.h"
#include "NCdata.h"
#include "DXFMakeOption.h"
#include "MCOption.h"

enum NCCOMMENT {		// g_szNCcomment[]
	ENDMILL = 0, DRILL, TAP, REAMER, 
	WORKRECT, WORKCYLINDER,
	LATHEVIEW, WIREVIEW,
	TOOLPOS
};
#define	ENDMILL_S		g_szNCcomment[ENDMILL]
#define	DRILL_S			g_szNCcomment[DRILL]
#define	TAP_S			g_szNCcomment[TAP]
#define	REAMER_S		g_szNCcomment[REAMER]
#define	WORKRECT_S		g_szNCcomment[WORKRECT]
#define	WORKCYLINDER_S	g_szNCcomment[WORKCYLINDER]
#define	LATHEVIEW_S		g_szNCcomment[LATHEVIEW]
#define	WIREVIEW_S		g_szNCcomment[WIREVIEW]
#define	TOOLPOS_S		g_szNCcomment[TOOLPOS]

// CNCDoc::DataOperation() �̑�����@
enum ENNCOPERATION {
	NCADD, NCINS, NCMOD
};

class CNCDoc : public CDocBase
{
	CWinThread*	m_pCutcalcThread;	// �؍펞�Ԍv�Z�گ�ނ������
	CString		m_strDXFFileName,	// DXF�o��̧�ٖ�
				m_strCurrentFile;	// ���ݏ�������NÇ�ٖ�(FileInsert etc.)
	CRecentViewInfo*	m_pRecentViewInfo;		// ̧�ق��Ƃ̕`����
	//
	int			m_nWorkOrg;						// �g�p����ܰ����W
	CPoint3D	m_ptNcWorkOrg[WORKOFFSET+1],	// ܰ����W�n(G54�`G59)��G92���_
				m_ptNcLocalOrg;					// ۰�ٍ��W�n(G52)���_
	CNCblockArray	m_obBlock;		// ̧�ٲҰ����ۯ��ް�
	CTypedPtrArrayEx<CPtrArray, CNCdata*>
					m_obGdata;		// G���ޕ`���޼ު��
	CStringArray	m_obMacroFile;	// ϸۓW�J�ꎞ̧��
	double		m_dMove[2],		// �ړ�����, �؍�ړ�����
				m_dCutTime;		// �؍펞��
	CRect3D		m_rcWork,		// ܰ���`(�ő�؍��`��OpenGLܰ���`�p)
				m_rcWorkCo;		// ���Ďw��
	//
	void	SetMaxRect(const CNCdata* pData) {
		// �ő��޼ު�ċ�`�ް����
		m_rcMax  |= pData->GetMaxRect();
		m_rcWork |= pData->GetMaxCutRect();
	}

	// �ڰ����̵�޼ު��
	CCriticalSection	m_csTraceDraw;
	size_t	m_nTraceDraw;	// ���̕`���߲��
	size_t	m_nTraceStart;	// �`��J�n�߲��(���وʒu�����ڰ����s)

	void	MakeDXF(const CDXFMakeOption*);

	// �ړ��E�؍풷�C���Ԍv�Z�گ��
	static	UINT CuttimeCalc_Thread(LPVOID);

	void	SerializeBlock(CArchive&, CNCblockArray&, DWORD);
	BOOL	SerializeAfterCheck(void);
	BOOL	ValidBlockCheck(void);
	BOOL	ValidDataCheck(void);

	void	ClearBlockData(void);
	void	DeleteMacroFile(void);

protected: // �V���A���C�Y�@�\�݂̂���쐬���܂��B
	CNCDoc();
	DECLARE_DYNCREATE(CNCDoc)

// �A�g���r���[�g
public:
	CString	GetDXFFileName(void) const {
		return m_strDXFFileName;
	}
	CString	GetCurrentFileName(void) const {
		return m_strCurrentFile;
	}
	CRecentViewInfo*	GetRecentViewInfo(void) const {
		return m_pRecentViewInfo;
	}
	INT_PTR		GetNCBlockSize(void) const {
		return m_obBlock.GetSize();
	}
	CNCblock*	GetNCblock(INT_PTR n) {
		ASSERT(0<=n && n<GetNCBlockSize());
		return m_obBlock[n];
	}
	INT_PTR		GetNCsize(void) const {
		return m_obGdata.GetSize();
	}
	CNCdata*	GetNCdata(INT_PTR n) const {
		ASSERT(0<=n && n<GetNCsize());
		return m_obGdata[n];
	}
	double	GetMoveData(size_t a) const {
		ASSERT(0<=a && a<SIZEOF(m_dMove));
		return m_dMove[a];
	}
	CPoint3D	GetOffsetOrig(void) const {
		ASSERT(0<=m_nWorkOrg && m_nWorkOrg<SIZEOF(m_ptNcWorkOrg));
		return m_ptNcWorkOrg[m_nWorkOrg] + m_ptNcLocalOrg;
	}
	double	GetCutTime(void) const {
		return m_dCutTime;
	}

	void	GetWorkRectPP(int a, double []);	// from NCInfoView.cpp

	int		SearchBlockRegex(boost::regex&, int = 0, BOOL = FALSE);
	void	CheckBreakPoint(int a) {	// ��ڰ��߲�Ă̐ݒ�
		CNCblock*	pBlock = GetNCblock(a);
		pBlock->SetBlockFlag(pBlock->GetBlockFlag() ^ NCF_BREAK, FALSE);
	}
	BOOL	IsBreakPoint(int a) {		// ��ڰ��߲�Ă̏��
		return GetNCblock(a)->GetBlockFlag() & NCF_BREAK;
	}
	void	ClearBreakPoint(void);		// ��ڰ��߲�đS����
	size_t	GetTraceDraw(void) {
		m_csTraceDraw.Lock();
		size_t	nTraceDraw = m_nTraceDraw;
		m_csTraceDraw.Unlock();
		return nTraceDraw;
	}
	size_t	GetTraceStart(void) const {
		return m_nTraceStart;
	}

	CRect3D	GetWorkRect(void) const {
		return m_rcWork;
	}
	CRect3D	GetWorkRectOrg(void) const {
		return m_rcWorkCo;
	}

// �I�y���[�V����
public:
	// ���Ѻ����ٰèݸ�
	BOOL	RouteCmdToAllViews(CView*, UINT, int, void*, AFX_CMDHANDLERINFO*);

	void	SelectWorkOffset(int nWork) {
		ASSERT(nWork>=0 && nWork<WORKOFFSET);
		m_nWorkOrg = nWork;
	}
	CNCdata*	DataOperation(const CNCdata*, LPNCARGV, int = -1, ENNCOPERATION = NCADD);
	void	StrOperation(LPCTSTR, int = -1, ENNCOPERATION = NCADD);
	void	RemoveAt(int, int);
	void	RemoveStr(int, int);

	void	AllChangeFactor(ENNCDRAWVIEW, double) const;	// �g�嗦�̍X�V

	void	CreateCutcalcThread(void);		// �؍펞�Ԍv�Z�گ�ނ̐���
	void	WaitCalcThread(BOOL = FALSE);	// �گ�ނ̏I��

	// from TH_NCRead.cpp
	BOOL	SerializeInsertBlock(LPCTSTR, int, DWORD = 0);	// �����ہCϸۂ̑}��
	void	AddMacroFile(const CString&);	// �޷���Ĕj����ɏ�������ꎞ̧��
	void	SetWorkRectComment(const CRect3D& rc, BOOL bUpdate = TRUE) {
		m_rcWorkCo = rc;	// ���ĂŎw�肳�ꂽܰ���`
		if ( bUpdate ) {
			m_rcWorkCo.NormalizeRect();
			m_bDocFlg.set(NCDOC_COMMENTWORK);
		}
	}
	void	SetWorkCylinderComment(double d, double h, const CPoint3D& ptOffset) {
		m_bDocFlg.set(NCDOC_CYLINDER);
		// �O�ڎl�p�` -> m_rcWorkCo
		d /= 2.0;
		CRect3D	rc(-d, -d, d, d, h, 0);
		rc.OffsetRect(ptOffset);
		SetWorkRectComment(rc);
	}
	void	SetWorkLatheR(double r) {
		m_rcWorkCo.high = r;
		m_rcWorkCo.low  = 0;
		m_bDocFlg.set(NCDOC_COMMENTWORK_R);
	}
	void	SetWorkLatheZ(double z1, double z2) {
		m_rcWorkCo.left  = z1;
		m_rcWorkCo.right = z2;
		m_rcWorkCo.NormalizeRect();
		m_bDocFlg.set(NCDOC_COMMENTWORK_Z);
	}
	void	SetLatheViewMode(void);

	// from NCWorkDlg.cpp
	void	SetWorkRect(BOOL, const CRect3D&);
	void	SetWorkCylinder(BOOL, double, double, const CPoint3D&);
	void	SetCommentStr(const CString&);

	// from NCViewTab.cpp
	BOOL	IncrementTrace(int&);	// �ڰ����s�̎��̺��ތ���
	BOOL	SetLineToTrace(BOOL, int);	// �s�ԍ������ڰ��s��ݒ�
	void	StartTrace(void) {
		m_csTraceDraw.Lock();
		m_nTraceDraw = 0;
		m_csTraceDraw.Unlock();
	}
	void	StopTrace(void) {
		m_csTraceDraw.Lock();
		m_nTraceDraw = GetNCsize();
		m_csTraceDraw.Unlock();
	}
	void	ResetTraceStart(void) {
		m_nTraceStart = 0;
	}

	// from ThumbnailDlg.cpp
	void	ReadThumbnail(LPCTSTR);

//�I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CNCDoc)
	public:
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
	virtual void OnChangedViewList();

// �C���v�������e�[�V����
public:
	virtual ~CNCDoc();

// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCDoc)
	afx_msg void OnUpdateFileInsert(CCmdUI* pCmdUI);
	afx_msg void OnFileInsert();
	afx_msg void OnFileNCD2DXF();
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWorkRect(CCmdUI* pCmdUI);
	afx_msg void OnWorkRect();
	afx_msg void OnUpdateMaxRect(CCmdUI* pCmdUI);
	afx_msg void OnMaxRect();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
