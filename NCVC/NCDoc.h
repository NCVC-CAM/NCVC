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
	CPoint3F	m_ptNcWorkOrg[WORKOFFSET+1],	// ܰ����W�n(G54�`G59)��G92���_
				m_ptNcLocalOrg;					// ۰�ٍ��W�n(G52)���_
	CNCblockArray	m_obBlock;		// ̧�ٲҰ����ۯ��ް�
	CNCarray		m_obGdata;		// G���ޕ`���޼ު��
	CStringArray	m_obMacroFile;	// ϸۓW�J�ꎞ̧��
	float		m_dMove[2],		// �ړ�����, �؍�ړ�����
				m_dCutTime;		// �؍펞��
	CRect3F		m_rcWork,		// ܰ���`(�ő�؍��`��OpenGLܰ���`�p)
				m_rcWorkCo;		// ���Ďw��
	//
	void	SetMaxRect(const CNCdata* pData) {
		// �ő��޼ު�ċ�`�ް����
		m_rcMax  |= pData->GetMaxRect();
		m_rcWork |= pData->GetMaxCutRect();
	}

	// �ڰ�
	CCriticalSection	m_csTraceDraw;
	UINT	m_nTrace;		// �ڰ����s���
	INT_PTR	m_nTraceDraw;	// ���̕`���߲��
	INT_PTR	m_nTraceStart;	// �`��J�n�߲��(���وʒu�����ڰ����s)

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
	BOOL	IsDocMill(void) const {
		return !(IsDocFlag(NCDOC_WIRE)||IsDocFlag(NCDOC_LATHE));
	}
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
	float	GetMoveData(size_t a) const {
		ASSERT(0<=a && a<SIZEOF(m_dMove));
		return m_dMove[a];
	}
	CPoint3F	GetOffsetOrig(void) const {
		ASSERT(0<=m_nWorkOrg && m_nWorkOrg<SIZEOF(m_ptNcWorkOrg));
		return m_ptNcWorkOrg[m_nWorkOrg] + m_ptNcLocalOrg;
	}
	float	GetCutTime(void) const {
		return m_dCutTime;
	}

	void	GetWorkRectPP(int a, float []);	// from NCInfoView.cpp

	INT_PTR	SearchBlockRegex(boost::regex&, INT_PTR = 0, BOOL = FALSE);
	void	CheckBreakPoint(INT_PTR a) {	// ��ڰ��߲�Ă̐ݒ�
		CNCblock*	pBlock = GetNCblock(a);
		pBlock->SetBlockFlag(pBlock->GetBlockFlag() ^ NCF_BREAK, FALSE);
	}
	BOOL	IsBreakPoint(INT_PTR a) {		// ��ڰ��߲�Ă̏��
		return GetNCblock(a)->GetBlockFlag() & NCF_BREAK;
	}
	void	ClearBreakPoint(void);		// ��ڰ��߲�đS����
	INT_PTR	GetTraceDraw(void) {
		m_csTraceDraw.Lock();
		INT_PTR	nTraceDraw = m_nTraceDraw;
		m_csTraceDraw.Unlock();
		return nTraceDraw;
	}
	UINT	GetTraceMode(void) const {
		return m_nTrace;
	}
	INT_PTR	GetTraceStart(void) const {
		return m_nTraceStart;
	}

	CRect3F	GetWorkRect(void) const {
		return m_rcWork;
	}
	CRect3F	GetWorkRectOrg(void) const {
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
	CNCdata*	DataOperation(const CNCdata*, LPNCARGV, INT_PTR = -1, ENNCOPERATION = NCADD);
	void	StrOperation(LPCTSTR, INT_PTR = -1, ENNCOPERATION = NCADD);
	void	RemoveAt(INT_PTR, INT_PTR);
	void	RemoveStr(INT_PTR, INT_PTR);

	void	AllChangeFactor(ENNCDRAWVIEW, float) const;	// �g�嗦�̍X�V

	void	CreateCutcalcThread(void);		// �؍펞�Ԍv�Z�گ�ނ̐���
	void	WaitCalcThread(BOOL = FALSE);	// �گ�ނ̏I��

	// from TH_NCRead.cpp
	BOOL	SerializeInsertBlock(LPCTSTR, INT_PTR, DWORD = 0);	// �����ہCϸۂ̑}��
	void	AddMacroFile(const CString&);	// �޷���Ĕj����ɏ�������ꎞ̧��
	void	SetWorkRectComment(const CRect3F& rc, BOOL bUpdate = TRUE) {
		m_rcWorkCo = rc;	// ���ĂŎw�肳�ꂽܰ���`
		if ( bUpdate ) {
			m_rcWorkCo.NormalizeRect();
			m_bDocFlg.set(NCDOC_COMMENTWORK);
		}
	}
	void	SetWorkCylinderComment(float d, float h, const CPoint3F& ptOffset) {
		m_bDocFlg.set(NCDOC_CYLINDER);
		// �O�ڎl�p�` -> m_rcWorkCo
		d /= 2.0;
		CRect3F	rc(-d, -d, d, d, h, 0);
		rc.OffsetRect(ptOffset);
		SetWorkRectComment(rc);
	}
	void	SetWorkLatheR(float r) {
		m_rcWorkCo.high = r;
		m_rcWorkCo.low  = 0;
		m_bDocFlg.set(NCDOC_COMMENTWORK_R);
	}
	void	SetWorkLatheZ(float z1, float z2) {
		m_rcWorkCo.left  = z1;
		m_rcWorkCo.right = z2;
		m_rcWorkCo.NormalizeRect();
		m_bDocFlg.set(NCDOC_COMMENTWORK_Z);
	}
	void	SetLatheViewMode(void);

	// from NCWorkDlg.cpp
	void	SetWorkRect(BOOL, const CRect3F&);
	void	SetWorkCylinder(BOOL, float, float, const CPoint3F&);
	void	SetCommentStr(const CString&);

	// from NCViewTab.cpp
	void	SetTraceMode(UINT id) {
		m_nTrace = id;
	}
	BOOL	IncrementTrace(INT_PTR&);	// �ڰ����s�̎��̺��ތ���
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

	// from NCListView.cpp
	void	InsertBlock(int, const CString&);

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
	afx_msg void OnFileNCD2DXF();
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWorkRect(CCmdUI* pCmdUI);
	afx_msg void OnWorkRect();
	afx_msg void OnUpdateMaxRect(CCmdUI* pCmdUI);
	afx_msg void OnMaxRect();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
