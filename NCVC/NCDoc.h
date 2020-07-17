// NCDoc.h : CNCDoc �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "DocBase.h"
#include "NCdata.h"
#include "DXFMakeOption.h"
#include "MCOption.h"

// CNCDoc::DataOperation() �̑�����@
enum	ENNCOPERATION	{
	NCADD, NCINS, NCMOD
};

class CNCDoc : public CDocument, public CDocBase
{
	BOOL		m_fError;		// �޷���đS��
	HANDLE		m_hCutcalc;		// �؍펞�Ԍv�Z�گ�ނ������
	BOOL		m_bCutcalc,		// �@�@�V�@�@�p���׸�
				m_bCorrect;		// �␳�v�Z�s�����ǂ���
	CString		m_strDXFFileName,	// DXF�o��̧�ٖ�
				m_strCurrentFile;	// ���ݏ�������NÇ�ٖ�(FileInsert etc.)
	// NC�ް�
	int			m_nWorkOrg;						// �g�p����ܰ����W
	CPoint3D	m_ptNcWorkOrg[WORKOFFSET+1],	// ܰ����W�n(G54�`G59)��G92���_
				m_ptNcLocalOrg;					// ۰�ٍ��W�n(G52)���_
	CNCblockArray	m_obBlock;	// ̧�ٲҰ����ۯ��ް�
	CTypedPtrArrayEx<CPtrArray, CNCdata*>
				m_obGdata;		// G���ޕ`���޼ު��
	CStringArray	m_obMacroFile;	// ϸۓW�J�ꎞ̧��
	double		m_dMove[2],		// �ړ�����, �؍�ړ�����
				m_dCutTime;		// �؍펞��
	CRect3D		m_rcMax,		// �ő��޼ު�ċ�`
				m_rcWork;		// ܰ���`
	BOOL		m_bMaxRect,		// �ő�؍��`�̕`��
				m_bWorkRect;	// ܰ���`�̕`��
	void		SetMaxRect(const CNCdata* pData) {
		m_rcMax |= pData->GetMaxRect();	// �ő��`�ް����
	}

	// �ڰ����̵�޼ު��
	CCriticalSection	m_csTraceDraw;
	int		m_nTraceDraw;	// ���̕`���߲��
	int		m_nTraceStart;	// �`��J�n�߲��(���وʒu�����ڰ����s)

	void	MakeDXF(const CDXFMakeOption*);

	// �ړ��E�؍풷�C���Ԍv�Z�گ��
	static	UINT CuttimeCalc_Thread(LPVOID);

	void	SerializeBlock(CArchive&, CNCblockArray&, DWORD = 0, BOOL = TRUE);
	BOOL	SerializeAfterCheck(void);

protected: // �V���A���C�Y�@�\�݂̂���쐬���܂��B
	CNCDoc();
	DECLARE_DYNCREATE(CNCDoc)

// �A�g���r���[�g
public:
	BOOL	GetDocError(void) const {
		return m_fError;
	}
	CString	GetDXFFileName(void) const {
		return m_strDXFFileName;
	}
	CString	GetCurrentFileName(void) const {
		return m_strCurrentFile;
	}
	int		GetNCBlockSize(void) const {
		return m_obBlock.GetSize();
	}
	CNCblock*	GetNCblock(int n) {
		ASSERT(0<=n && n<GetNCBlockSize());
		return m_obBlock[n];
	}
	int		GetNCsize(void) const {
		return m_obGdata.GetSize();
	}
	CNCdata*	GetNCdata(int n) const {
		ASSERT(0<=n && n<GetNCsize());
		return m_obGdata[n];
	}
	double	GetMoveData(size_t a) const {
		ASSERT(0<=a && a<SIZEOF(m_dMove));
		return m_dMove[a];
	}
	double	GetCutTime(void) const {
		return m_dCutTime;
	}
	BOOL	IsCalcContinue(void) const {
		return m_bCutcalc;
	}

	void	GetWorkRectPP(int a, double dTmp[]);	// from NCInfoView.cpp

	int		SearchBlockRegex(boost::regex&, int = 0, BOOL = FALSE);
	void	CheckBreakPoint(int a) {	// ��ڰ��߲�Ă̐ݒ�
		CNCblock*	pBlock = GetNCblock(a);
		pBlock->SetBlockFlag(pBlock->GetBlockFlag() ^ NCF_BREAK, FALSE);
	}
	BOOL	IsBreakPoint(int a) {		// ��ڰ��߲�Ă̏��
		return GetNCblock(a)->GetBlockFlag() & NCF_BREAK;
	}
	void	ClearBreakPoint(void);		// ��ڰ��߲�đS����
	int		GetTraceDraw(void) {
		m_csTraceDraw.Lock();
		int	nTraceDraw = m_nTraceDraw;
		m_csTraceDraw.Unlock();
		return nTraceDraw;
	}
	int		GetTraceStart(void) const {
		return m_nTraceStart;
	}

	BOOL	IsMaxRect(void) const {
		return m_bMaxRect;
	}
	BOOL	IsWorkRect(void) const {
		return m_bWorkRect;
	}
	CRect3D	GetMaxRect(void) const {
		return m_rcMax;
	}
	CRect3D	GetWorkRect(void) const {
		return m_rcWork;
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
	void	StrOperation(LPCSTR, int = -1, ENNCOPERATION = NCADD);
	void	RemoveAt(int, int);
	void	RemoveStr(int, int);

	void	AllChangeFactor(double) const;	// �g�嗦�̍X�V
	void	AllChangeFactorXY(double) const;
	void	AllChangeFactorXZ(double) const;
	void	AllChangeFactorYZ(double) const;

	void	CreateCutcalcThread(void);		// �؍펞�Ԍv�Z�گ�ނ̐���
	void	WaitCalcThread(void);			// �گ�ނ̏I��

	// from TH_NCRead.cpp
	BOOL	SerializeInsertBlock(LPCTSTR, int, DWORD = 0, BOOL = TRUE);	// �����ہCϸۂ̑}��
	void	AddMacroFile(const CString&);	// �޷���Ĕj����ɏ�������ꎞ̧��

	// from NCChild.cpp <- NCWorkDlg.cpp
	void	SetWorkRect(BOOL bHide, CRect3D& rc) {
		if ( bHide )
			m_rcWork = rc;
		UpdateAllViews(NULL, UAV_DRAWWORKRECT, (CObject *)bHide);
		m_bWorkRect = bHide;
	}

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

//�I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CNCDoc)
	public:
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
	//}}AFX_VIRTUAL
	// �X�Vϰ��t�^
	virtual void SetModifiedFlag(BOOL bModified = TRUE);

// �C���v�������e�[�V����
public:
	virtual ~CNCDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

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
