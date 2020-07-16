// TH_MakeNCD.cpp
// DXF->NC ����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCdata.h"
#include "NCMakeClass.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"

#include <math.h>
#include <ctype.h>

/*
!!!ATTENTION!!!
�������Ԃ̕\���F�ذ��ް�ޮ݂ł͊O���̂�Y�ꂸ��
#define	_DBG_NCMAKE_TIME
*/

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

/*
	CDXFdata �� GetType() �� GetMakeType() �̎g�������ɒ��ӁI�I
*/
// �悭�g���ϐ���Ăяo���̊ȗ��u��
using namespace boost;
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)

// ��۰��ٕϐ���`
static	CThreadDlg*	g_pParent;
static	CDXFDoc*	g_pDoc;
static	CNCMakeOption*	g_pMakeOpt;

static	LPCTSTR	g_szWait = "�ް�������!!!";
static	LPCTSTR	g_szFinal = "NC�ް��o�͒�!!!";

// NC�����ɕK�v���ް��Q
static	CDXFarray	g_obDXFdata;	// �؍�Ώ��ް�(ڲԖ��ɒ��o)
static	CDXFmap		g_mpDXFdata,	// ���W�𷰂�CDXFdata���i�[
			g_mpDXFstarttext,				// �J�nڲ�
			g_mpDXFmove, g_mpDXFmovetext,	// �ړ�ڲ�ϯ��
			g_mpDXFtext, g_mpDXFcomment;	// ���H÷�āC���Đ�p
static	CDXFarray	g_obPoint;		// �����H�ް�
static	CDXFsort	g_obCircle;		// �~�ް��������H����Ƃ��̉��o�^
static	CDXFsort	g_obDrillGroup;	// ��ٰ�ߕ������������H�ް�
static	CDXFsort	g_obDrillAxis;	// -> �����W�ŕ��בւ�
static	CDXFsort	g_obStartData;	// ���H�J�n�ʒu�w���ް�
static	CDXFmap		g_mpEuler;		// ��M������޼ު�Ă��i�[
static	CDXFlist	g_ltDeepGlist(1024);// �[���؍�p�̉��o�^
static	CTypedPtrArrayEx<CPtrArray, CNCMake*>	g_obMakeGdata;	// ���H�ް�

static	BOOL		g_bData;		// �e���������Ő������ꂽ��
static	double		g_dZCut;		// Z���̐؍���W == RoundUp(GetDbl(MKNC_DBL_ZCUT))
static	double		g_dDeep;		// �[���̐؍���W == RoundUp(GetDbl(MKNC_DBL_DEEP))
static	double		g_dZInitial;	// Z���̲Ƽ�ٓ_ == RoundUp(GetDbl(MKNC_DBL_G92Z))
static	double		g_dZG0Stop;		// Z����R�_ == RoundUp(GetDbl(MKNC_DBL_ZG0STOP))
static	double		g_dZReturn;		// Z���̕��A���W

// ��ފ֐�
static	void	InitialVariable(void);		// �ϐ�������
static	void	InitialCycleBaseVariable(void);	// �Œ軲�ق��ް��l������
static	BOOL	MakeNCD_MainThread(CString&);	// NC������Ҳ�ٰ��
static	BOOL	SingleLayer(void);			// �P��ڲԏ���
static	BOOL	MultiLayer(int);			// ����ڲԏ���(2����݌��p)
static	void	SetStaticOption(void);		// �ÓI�ϐ��̏�����
static	BOOL	SetStartData(void);			// ���H�J�n�ʒu�w��ڲԂ̉��o�^, �ړ��w��ڲԂ̏���
static	void	SetGlobalArray_Single(void);
static	void	SetGlobalArray_Multi(CLayerData*);
static	void	SetGlobalArray_Sub(CDXFdata*);
static	void	SetGlobalMap_Other(void);
static	BOOL	MakeNCD_FinalFunc(LPCTSTR = NULL);	// �I�����ށÇ�ُo�͂Ȃ�
static	BOOL	OutputNCcode(LPCTSTR);		// NC���ނ̏o��

// �e���ߕʂ̏����Ăяo��
static	enum	ENMAKETYPE {MAKECUTTER, MAKEDRILLPOINT, MAKEDRILLCIRCLE};
static	BOOL	CallMakeDrill_Sub(CString&);
static	BOOL	CallMakeLoop(ENMAKETYPE, CString&);

// ���_����(TRUE:IsMatch)
static	tuple<CDXFdata*, BOOL>	OrgTuningCutter(void);
static	tuple<CDXFdata*, BOOL>	OrgTuningDrillPoint(void);
static	CDXFdata*	OrgTuningDrillCircle(void);

// �ް���͊֐�
static	BOOL		MakeLoopEuler(BOOL, CDXFdata*);
static	BOOL		MakeLoopEulerSearch(CPointD&);
static	int			MakeLoopEulerAdd(void);
static	BOOL		MakeLoopEulerAdd_with_one_stroke(CPointD&, CDXFarray*, CDXFlist&, BOOL);
static	BOOL		MakeLoopDeepAdd(void);
typedef	CDXFdata*	(*PFNDEEPPROC)(BOOL);
static	PFNDEEPPROC	g_pfnDeepHead, g_pfnDeepTail;
static	CDXFdata*	MakeLoopAddDeepHead(BOOL);
static	CDXFdata*	MakeLoopAddDeepTail(BOOL);
static	CDXFdata*	MakeLoopAddDeepHeadAll(BOOL);
static	CDXFdata*	MakeLoopAddDeepTailAll(BOOL);
static	void		MakeLoopAddDeepZProc(BOOL, BOOL, CDXFdata*);
static	void		MakeLoopDeepZDown(void);
static	void		MakeLoopDeepZUp(void);
static	BOOL		MakeLoopDrillPoint(BOOL, CDXFdata*);
static	BOOL		MakeLoopDrillPointSeqChk(BOOL, CDXFsort&);
static	BOOL		MakeLoopDrillPointXY(BOOL);
static	BOOL		MakeLoopDrillPointXYRevers(BOOL);
static	BOOL		MakeLoopDrillCircle(void);
static	BOOL		MakeLoopAddDrill(BOOL, CDXFdata*);
static	BOOL		MakeLoopAddDrillSeq(int);
static	CDXFdata*	MakeLoopAddFirstMove(ENMAKETYPE);
static	BOOL		MakeLoopAddLastMove(void);

// �����Ŏw�肵����޼ު�ĂɈ�ԋ߂���޼ު�Ă�Ԃ�(�߂�l:Z���̈ړ����K�v��)
static	tuple<CDXFdata*, BOOL>	GetNearPointCutter(CDXFdata*);
static	tuple<CDXFdata*, BOOL>	GetNearPointDrill(CDXFdata*);
typedef	BOOL	(*PFNGETMATCHPOINTMOVE)(CDXFdata*&);
static	PFNGETMATCHPOINTMOVE	g_pfnGetMatchPointMove;
static	BOOL	GetMatchPointMove_Target(CDXFdata*&);
static	BOOL	GetMatchPointMove_Exclude(CDXFdata*&);

// ͯ�ް,̯�ް���̽�߼�ٺ��ސ���
static	void	AddCustomCode(const CString&, const CDXFdata*);
static	BOOL	IsNCchar(LPCTSTR);

// ÷�ď��
typedef	void	(*PFNADDTEXT)(CPointD&);
static	PFNADDTEXT	g_pfnAddCutterText, g_pfnAddCommentText,
					g_pfnAddStartText, g_pfnAddMoveText;
static	void	AddCutterText_Target(CPointD&);
static	void	AddStartText_Target(CPointD&);
static	void	AddMoveText_Target(CPointD&);
static	void	AddCommentText_Target(CPointD&);
static	void	AddText_Exclude(CPointD&);

// NC�ް��o�^�֐�
static	void	AddMakeStart(void);		// ���H�J�n�w���ް��̐���
typedef void	(*PFNADDMOVE)(void);	// �ړ��w��ڲԂ̈ړ����ɂ����铮��֐�
static	PFNADDMOVE	g_pfnAddMoveZ, g_pfnAddMoveCust_B, g_pfnAddMoveCust_A;
static	void	AddMoveZ_NotMove(void);
static	void	AddMoveZ_R(void);
static	void	AddMoveZ_Initial(void);
static	void	AddMoveCust_B(void);
static	void	AddMoveCust_A(void);
// ÷�ď��(�؍�ڲԂƺ���ڲ�)�̌���
inline	void	AddCutterTextIntegrated(CPointD& pt)
{
	(*g_pfnAddCommentText)(pt);
	(*g_pfnAddCutterText)(pt);
}
// ÷�ď��(�J�nڲԂƺ���ڲ�)�̌���
inline	void	AddStartTextIntegrated(CPointD& pt)
{
	(*g_pfnAddCommentText)(pt);
	(*g_pfnAddStartText)(pt);
}
// ÷�ď��(�ړ�ڲԂƺ���ڲ�)�̌���
inline	void	AddMoveTextIntegrated(CPointD& pt)
{
	(*g_pfnAddCommentText)(pt);
	(*g_pfnAddMoveText)(pt);
}
// Z���̈ړ�(�؍�)�ް�����
inline	void	AddMoveGdata(int nCode, double dZ, double dFeed)
{
	CNCMake*	mkNCD = new CNCMake(nCode, dZ, dFeed);
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
}
// Z���̏㏸
inline	void	AddMoveGdataZup(void)
{
	CPointD	pt( CDXFdata::ms_pData->GetEndMakePoint() );
	// �I�_���W�ł̺��Đ���
	AddCutterTextIntegrated(pt);
	// Z���̏㏸
	AddMoveGdata(0, g_dZReturn, -1.0);
}
// Z���̉��~
inline	void	AddMoveGdataZdown(void)
{
	// Z���̌��݈ʒu��R�_���傫��(����)�Ȃ�R�_�܂ő�����
	if ( CNCMake::ms_xyz[NCA_Z] > g_dZG0Stop )
		AddMoveGdata(0, g_dZG0Stop, -1.0);
	// ���H�ςݐ[���ւ�Z���؍�ړ�
	double	dZValue;
	switch ( GetNum(MKNC_NUM_MAKEEND) ) {
	case 1:		// �̾�Ĉړ�
		dZValue = g_dZCut + GetDbl(MKNC_DBL_MAKEEND);
		break;
	case 2:		// �Œ�Z�l
		dZValue = GetDbl(MKNC_DBL_MAKEEND);
		break;
	default:
		dZValue = HUGE_VAL;
		break;
	}
	if ( CNCMake::ms_xyz[NCA_Z] > dZValue )
		AddMoveGdata(1, dZValue, GetDbl(MKNC_DBL_MAKEENDFEED));
	// �؍�_�܂Ő؍푗��
	if ( CNCMake::ms_xyz[NCA_Z] > g_dZCut )
		AddMoveGdata(1, g_dZCut, GetDbl(MKNC_DBL_ZFEED));
}
inline	void	AddMoveGdataG0(const CDXFdata* pData)
{
	// G0�ړ��ް��̐���
	CNCMake* mkNCD = new CNCMake(0, pData->GetStartMakePoint());
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
	// Z���̉��~
	AddMoveGdataZdown();
}
inline	void	AddMoveGdataG1(const CDXFdata* pData)
{
	// Z���̉��~
	AddMoveGdataZdown();
	// G1�ړ��ް��̐���
	CNCMake* mkNCD = new CNCMake(1, pData->GetStartMakePoint());
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
}
// �؍��ް��̐���
inline	void	AddMakeGdata(CDXFdata* pData)
{
	CPointD	pt( pData->GetStartMakePoint() );
	// �J�n�ʒu�Ɠ�����÷���ް��̐���
	AddCutterTextIntegrated(pt);
	// �؍��ް�����
	CNCMake*	mkNCD = new CNCMake(pData,
		pData->GetMakeType() == DXFPOINTDATA ?
			GetDbl(MKNC_DBL_DRILLFEED) : GetDbl(MKNC_DBL_FEED) );
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
	pData->SetMakeFlg();
}
// �؍��ް��̐����i�[���j
inline	void	AddMakeGdataDeep(CDXFdata* pData, BOOL bDeep)
{
	CPointD	pt( pData->GetStartMakePoint() );
	// �J�n�ʒu�Ɠ�����÷���ް��̐���
	AddCutterTextIntegrated(pt);
	// �؍��ް�����
	CNCMake*	mkNCD = new CNCMake(pData,
		bDeep ? GetDbl(MKNC_DBL_DEEPFEED) : GetDbl(MKNC_DBL_FEED) );
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
	pData->SetMakeFlg();
}
// �C���ް��̐���
inline	void	AddMakeGdata(const CString& strData)
{
	CNCMake*	mkNCD = new CNCMake(strData);
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
}
// �ړ��w��ڲԂ��ް�����
inline	void	AddMakeMove(CDXFdata* pData)
{
	CPointD	pt( pData->GetStartMakePoint() );
	// �J�n�ʒu�Ɠ�����÷���ް��̐���
	AddMoveTextIntegrated(pt);
	// �ړ��w��
	CNCMake*	mkNCD = new CNCMake(pData);
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
	pData->SetMakeFlg();
}
// �Œ軲�ٷ�ݾٺ���
inline	void	AddMakeGdataCycleCancel(void)
{
	if ( CDXFdata::ms_pData->GetMakeType() == DXFPOINTDATA ) {
		AddMakeGdata( CNCMake::MakeCustomString(80) );
		InitialCycleBaseVariable();
	}
}

// ̪��ލX�V
static	int		g_nFase;			// ̪��އ�
static	void	SendFaseMessage(int = -1, LPCTSTR = NULL, LPCTSTR = NULL);
inline	void	SendProgressPos(int i)
{
	if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
		g_pParent->m_ctReadProgress.SetPos(i);
}

// ���בւ��⏕�֐�
int		CircleSizeCompareFunc1(CDXFdata*, CDXFdata*);	// �~�ް��������
int		CircleSizeCompareFunc2(CDXFdata*, CDXFdata*);	// �~�ް��~�����
int		DrillOptimaizeCompareFuncX(CDXFdata*, CDXFdata*);	// X��
int		DrillOptimaizeCompareFuncY(CDXFdata*, CDXFdata*);	// Y��

// ��޽گ�ފ֐�
static	CCriticalSection	g_csMakeAfter;	// MakeNCDAfterThread()�گ��ۯ���޼ު��
static	UINT	MakeNCDAfterThread(LPVOID);	// ��n���گ��
/*
	MAKENCDTHREADPARAM::evStart �ɂ���
		���� SearchObjectThread() �ɂ�����
		�P�̃O���[�o���C�x���g�ŕ����X���b�h�̓����������s�������������ǂ����C
		�����̌��ʁC���܂��R���g���[���ł��Ȃ����Ƃ������D
		����āC�e�N���X���b�h���ƂɃ��[�v�J�n�C�x���g��ݒ肷�邱�Ƃɂ����D
*/
class	MAKENCDTHREADPARAM {
public:
	CEvent	evStart,		// ٰ�ߊJ�n�����
			evEnd;			// �I���҂��m�F
	BOOL	bThread,		// �گ�ނ̌p���׸�
			bResult;		// �گ�ނ̌���
	int		nOrder,			// �گ�ނ̏����Ώ�
			nOrder2;
	// MAKENCDTHREADPARAM::CEvent ���蓮����Ăɂ��邽�߂̺ݽ�׸�
	MAKENCDTHREADPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE),
		bThread(TRUE), bResult(TRUE), nOrder(-1)
	{}
};
#define	LPMAKENCDTHREADPARAM	MAKENCDTHREADPARAM *
//
static	void	SetSearchRange(void);		// �e�گ�ނ̌����͈�
static	UINT	SearchObjectThread(LPVOID);		// ���̐؍��޼ު�Ă̌���(from GetNearPointCutter)
static	UINT	ClearPointMapThread(LPVOID);	// CMap��޼ު�ĸر���ޯ���׳��޽گ��
typedef	struct	tagSEARCHRESULT {
	CDXFdata*	pData;
	double		dGap;
	BOOL		bMatch;
} SEARCHRESULT, *LPSEARCHRESULT;
extern	int		g_nProcesser;		// ��۾����(NCVC.cpp)
static	LPMAKENCDTHREADPARAM	g_lpmSearch = NULL;
static	MAKENCDTHREADPARAM		g_pmClearMap;
static	LPSEARCHRESULT	g_lpSearchResult = NULL;
static	LPHANDLE		g_phSearchEnd = NULL;

//////////////////////////////////////////////////////////////////////
// NC�����گ��
//////////////////////////////////////////////////////////////////////

UINT MakeNCD_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_Thread()\nStart", DBG_GREEN);
#endif
	int		i, nResult = IDCANCEL;

#ifdef _DBG_NCMAKE_TIME
	// ���ݎ������擾
	CTime t1 = CTime::GetCurrentTime();
#endif

	// ��۰��ٕϐ�������
	LPNCVCTHREADPARAM	pParam = (LPNCVCTHREADPARAM)pVoid;
	g_pParent = pParam->pParent;
	g_pDoc = (CDXFDoc *)(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);

	// �������\��
	g_nFase = 0;
	SendFaseMessage(-1, g_szWait);
	g_pMakeOpt = NULL;

	// �گ�������
	LPHANDLE	hSearchThread = NULL;
	HANDLE		hClearMapThread = NULL;

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		int		nType;
		BOOL	bResult = FALSE;

		// �گ������ق�K�v���m��
		hSearchThread	= new HANDLE[g_nProcesser];

		// NC������߼�ݵ�޼ު�Ă̐���
		g_pMakeOpt = new CNCMakeOption(NULL);	// �ǂݍ��݂͂��Ȃ�
		CNCMake::ms_pMakeOpt = g_pMakeOpt;

		// NC��������
		nType = (int)( ((LPNCVCTHREADPARAM)pParam)->pParam );

		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
#ifdef _DEBUG
		dbg.printf("g_csMakeAfter Unlock OK");
#endif

		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		CDXFdata::ms_ptOrg = g_pDoc->GetCutterOrigin();
		g_pDoc->GetCircleObject()->OrgTuning(FALSE);	// ���ʓI�Ɍ��_����ۂɂȂ�
		InitialVariable();
		// �������蓖��
		i = g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER);
		g_obDXFdata.SetSize(0, i);
		g_obPoint.SetSize(0, i);
		g_obCircle.SetSize(0, i);
		g_obMakeGdata.SetSize(0, i*2);
		g_mpDXFdata.InitHashTable(max(17, GetPrimeNumber(i*2)));
		g_mpEuler.InitHashTable(max(17, GetPrimeNumber(i)));
		g_mpDXFtext.InitHashTable(max(17, GetPrimeNumber(i)));
		i = g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER);
		g_obStartData.SetSize(0, max(10, i*2));
		g_mpDXFstarttext.InitHashTable(max(17, GetPrimeNumber(i)));
		i = g_pDoc->GetDxfLayerDataCnt(DXFMOVLAYER);
		g_mpDXFmove.InitHashTable(max(17, GetPrimeNumber(i*2)));
		g_mpDXFmovetext.InitHashTable(max(17, GetPrimeNumber(i)));
		g_mpDXFcomment.InitHashTable(max(17, GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFCOMLAYER))));

		// �֘A�گ�ދN��
		CWinThread*	pThread;
		for ( i=0; i<g_nProcesser; i++ ) {
			g_lpmSearch[i].bThread = TRUE;
			pThread = AfxBeginThread(SearchObjectThread, (LPVOID)i);
			hSearchThread[i] = NC_DuplicateHandle(pThread->m_hThread);
		}
		g_pmClearMap.bThread = TRUE;
		g_pmClearMap.evEnd.SetEvent();
		pThread = AfxBeginThread(ClearPointMapThread, NULL);
		hClearMapThread = NC_DuplicateHandle(pThread->m_hThread);

		// Ҳ�ٰ�߂�
		switch ( nType ) {
		case ID_FILE_DXF2NCD:		// �P��ڲ�
			bResult = SingleLayer();
			break;

		case ID_FILE_DXF2NCD_EX1:	// �����̐؍����̧��
		case ID_FILE_DXF2NCD_EX2:	// ����Z���W
			bResult = MultiLayer(nType);
			break;
		}

		// �߂�l���
		if ( bResult && IsThread() )
			nResult = IDOK;
#ifdef _DEBUG
		dbg.printf("MakeNCD_Thread All Over!!!");
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

#ifdef _DBG_NCMAKE_TIME
	// �I���������擾�C�������Ԃ̌v�Z
	CTime t2 = CTime::GetCurrentTime();
	CTimeSpan ts = t2 - t1;
	CString	strTime( ts.Format("%H:%M:%S") );
	AfxMessageBox( strTime );
#endif

	// �֘A�گ�ޏI���w��
	if ( hClearMapThread ) {
		g_pmClearMap.bThread = FALSE;
		g_pmClearMap.evStart.SetEvent();
		WaitForSingleObject(hClearMapThread, INFINITE);
		CloseHandle(hClearMapThread);
	}
	g_pmClearMap.evEnd.ResetEvent();
	if ( hSearchThread ) {
		for ( i=0; i<g_nProcesser; i++ ) {
			g_lpmSearch[i].bThread = FALSE;
			g_lpmSearch[i].evStart.SetEvent();
		}
		WaitForMultipleObjects(g_nProcesser, hSearchThread, TRUE, INFINITE);
		for ( i=0; i<g_nProcesser; i++ ) {
			CloseHandle(hSearchThread[i]);
			g_lpmSearch[i].evEnd.ResetEvent();
		}
	}

	// �I������
	CDXFmap::ms_dTolerance = NCMIN;		// �K��l�ɖ߂�
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��
	// ��������NC���ނ̏����گ��(�D��x��������)
	AfxBeginThread(MakeNCDAfterThread, NULL,
		/*THREAD_PRIORITY_LOWEST*/
		THREAD_PRIORITY_IDLE
		/*THREAD_PRIORITY_BELOW_NORMAL*/);

	// ������޼ު�č폜
	if ( g_pMakeOpt )
		delete	g_pMakeOpt;
	// �گ������ٍ폜
	if ( hSearchThread )
		delete[]	hSearchThread;

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	g_bData = FALSE;
	CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	CNCMake::InitialVariable();
}

void InitialCycleBaseVariable(void)
{
	CNCMake::ms_dCycleZ[1] =
		CNCMake::ms_dCycleR[1] =
			CNCMake::ms_dCycleP[1] = HUGE_VAL;
}

//////////////////////////////////////////////////////////////////////

BOOL SingleLayer(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SingleLayer()\nStart", DBG_CYAN);
#endif
	// NC������߼�ݓǂݍ���
	g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetInitList()->GetHead());
	// �������Ƃɕω��������Ұ���ݒ�
	SetStaticOption();
	// ���H���_�J�n�ʒu�w��ڲԏ���
	if ( !SetStartData() )
		return FALSE;
	// Z���̐؍���W���
	g_dZCut = RoundUp(GetDbl(MKNC_DBL_ZCUT));
	g_dDeep = RoundUp(GetDbl(MKNC_DBL_DEEP));
	// �Œ軲�ق̐؂荞�ݍ��W���
	CNCMake::ms_dCycleZ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLZ));
	CNCMake::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
	CNCMake::ms_dCycleP[0] = GetNum(MKNC_NUM_DWELL);
	InitialCycleBaseVariable();
	// ��۰��ٕϐ��ɐ����Ώ۵�޼ު�Ă̺�߰
	SetGlobalArray_Single();
	// �����J�n
	if ( !MakeNCD_MainThread(CString()) ) {
#ifdef _DEBUG
		dbg.printf("Error:MakeNCD_MainThread()");
#endif
		return FALSE;
	}

	// �ŏI����
	if ( g_obMakeGdata.GetSize() == 0 ) {
		AfxMessageBox(IDS_ERR_MAKENC, MB_OK|MB_ICONSTOP);
		return FALSE;
	}
	// �I�����ށÇ�ق̏o�͂Ȃ�
	if ( !MakeNCD_FinalFunc() ) {
#ifdef _DEBUG
		dbg.printf("Error:MakeNCD_FinalFunc()");
#endif
		return FALSE;
	}

	return TRUE;
}

BOOL MultiLayer(int nType)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MultiLayer()\nStart", DBG_CYAN);
	CMagaDbg	dbgE("MultiLayer() Error", DBG_RED);
#endif
	extern	LPCTSTR	gg_szCat;
	int		i, j, nLayerCnt = g_pDoc->GetLayerCnt();
	BOOL	bPartOut = FALSE;	// �P��ł��ʏo�͂������TRUE
	CLayerData*	pLayer;
	CString	strLayer;

	if ( nType == ID_FILE_DXF2NCD_EX2 ) {
		// ��ƂȂ�NC������߼�ݓǂݍ���
		g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetInitList()->GetHead());
		// �������Ƃɕω��������Ұ���ݒ�
		SetStaticOption();
	}
	// ���H���_�J�n�ʒu�w��ڲԏ���
	if ( !SetStartData() )
		return FALSE;

	// ڲԖ���ٰ��
	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		// �؍�ڲԂőΏۂ�ڲԂ���
		if ( !pLayer->IsCutType() || !pLayer->IsCutTarget() )
			continue;
		pLayer->SetLayerFlags();
		strLayer = pLayer->GetStrLayer();
#ifdef _DEBUG
		dbg.printf("No.%d ID=%d Name=%s", i+1,
			pLayer->GetListNo(), strLayer);
#endif
		// ����ڲԖ����޲�۸ނɕ\��
		SendFaseMessage(-1, g_szWait, strLayer);
		//
		if ( nType == ID_FILE_DXF2NCD_EX1 ) {
			// NC������߼�ݓǂݍ���
			if ( pLayer->GetInitFile().CompareNoCase(g_pMakeOpt->GetInitFile()) != 0 ) {
				g_pMakeOpt->ReadMakeOption(pLayer->GetInitFile());	// �Ⴄ�Ƃ�����
				SetStaticOption();
			}
			// Z���̐؍���W���
			g_dZCut = RoundUp(GetDbl(MKNC_DBL_ZCUT));
			g_dDeep = RoundUp(GetDbl(MKNC_DBL_DEEP));
		}
		else {
			// ����Z���W�␳
			g_dZCut = g_dDeep = RoundUp(pLayer->GetZCut());
		}
		// �Œ軲�ق̐؂荞�ݍ��W���
		CNCMake::ms_dCycleZ[0] = pLayer->IsDrillZ() ?
					g_dZCut : RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		CNCMake::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
		CNCMake::ms_dCycleP[0] = GetNum(MKNC_NUM_DWELL);
		InitialCycleBaseVariable();
		// ��۰��ٕϐ��ɐ����Ώ۵�޼ު�Ă̺�߰
		SetGlobalArray_Multi(pLayer);
		// �����J�n
#ifdef _DEBUG
		dbg.printf("Layer=%s Cut=%f Start", strLayer, g_dZCut);
#endif
		if ( !MakeNCD_MainThread(strLayer) ) {
#ifdef _DEBUG
			dbgE.printf("MakeNCD_MainThread() Error");
#endif
			return FALSE;
		}
		// �ʏo�͂łȂ��Ȃ�(���בւ����Ă���̂œr���Ɋ��荞�ނ��Ƃ͂Ȃ�)
		if ( !pLayer->IsPartOut() )
			continue;
		// --- �ȉ��ʏo�݂͂̂̏���
		if ( g_bData ) {	// NC�����؁H
			// MakeNCD_FinalFunc(�I�����ށÇ�ق̏o��)�̎��s
			if ( MakeNCD_FinalFunc(pLayer->GetNCFile()) ) {
				// ��۰��ٕϐ�������
				InitialVariable();
				// NC�����Ϗ��̍폜
				for ( j=0; j<g_obMakeGdata.GetSize() && IsThread(); j++ )
					delete	g_obMakeGdata[j];
				g_obMakeGdata.RemoveAll();
			}
			else {
#ifdef _DEBUG
				dbgE.printf("MakeNCD_FinalFunc_Multi() Error");
#endif
				return FALSE;
			}
			bPartOut = TRUE;	// �P��ł��ʏo�͂���
		}
		else {
#ifdef _DEBUG
			dbg.printf("Layer=%s CDXFdata::ms_pData NULL", pLayer->GetStrLayer());
#endif
			// �Y��ڲԂ��ް��Ȃ�
			pLayer->SetLayerFlags(1);
		}
	}	// End of for main loop (Layer)

	// --- �ŏI����
	if ( g_bData ) {	// ٰ�߂��S�̏o�͂ŏI��
		if ( g_obMakeGdata.GetSize() == 0 ) {
			if ( bPartOut ) {	// �ʏo�͂������
				// �ʏo�͈ȊO��ڲԏ����擾���Cܰ�ݸ�ү���ޏo�͂�
				for ( i=0; i<nLayerCnt; i++ ) {
					pLayer = g_pDoc->GetLayerData(i);
					if ( !pLayer->IsPartOut() )
						pLayer->SetLayerFlags(1);
				}
			}
			else {
				// �װү����
				AfxMessageBox(IDS_ERR_MAKENC, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
		}
		else {
			// �I�����ށÇ�ق̏o�͂Ȃ�
			if ( !MakeNCD_FinalFunc() ) {
#ifdef _DEBUG
				dbgE.printf("MakeNCD_FinalFunc()");
#endif
				return FALSE;
			}
		}
	}

	// �ʏo�͂�ܰ�ݸ�ү����
	CString	strMiss;
	for ( i=0; i<nLayerCnt; i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( pLayer->GetLayerFlags() == 0 )
			continue;
		if ( !strMiss.IsEmpty() )
			strMiss += gg_szCat;
		strMiss += pLayer->GetStrLayer();
	}
	if ( !strMiss.IsEmpty() ) {
		CString	strMsg;
		strMsg.Format(IDS_ERR_FAILMULTILAYER, strMiss);
		AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
	}

	return TRUE;
}

void SetStaticOption(void)
{
	g_dZInitial = RoundUp(GetDbl(MKNC_DBL_G92Z));		// �Ƽ�ٓ_
	g_dZG0Stop  = RoundUp(GetDbl(MKNC_DBL_ZG0STOP));	// R�_���W
	// AddMoveGdata(Z���̏㏸)�Ŏg�p
	g_dZReturn = GetNum(MKNC_NUM_ZRETURN) == 0 ? g_dZInitial : g_dZG0Stop;
	// �ړ��w��ڲԂ�Z��
	switch ( GetNum(MKNC_NUM_MOVEZ) ) {
	case 1:		// R�_
		g_pfnAddMoveZ = &AddMoveZ_R;
		break;
	case 2:		// �Ƽ�ٓ_
		g_pfnAddMoveZ = &AddMoveZ_Initial;
		break;
	default:	// ���̂܂�(��)
		g_pfnAddMoveZ = &AddMoveZ_NotMove;
		break;
	}
	// �ړ��w��ڲԂ̶��Ѻ���
	g_pfnAddMoveCust_B = g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_B).IsEmpty() ?
		&AddMoveZ_NotMove : &AddMoveCust_B;
	g_pfnAddMoveCust_A = g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_A).IsEmpty() ?
		&AddMoveZ_NotMove : &AddMoveCust_A;
	// ���H�������e��
	CDXFmap::ms_dTolerance = GetFlg(MKNC_FLG_DEEP) ?
		NCMIN : GetDbl(MKNC_DBL_TOLERANCE);
	// �[���̏���
	if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
		g_pfnDeepHead = &MakeLoopAddDeepHeadAll;
		g_pfnDeepTail = &MakeLoopAddDeepTailAll;
	}
	else {
		g_pfnDeepHead = &MakeLoopAddDeepHead;
		g_pfnDeepTail = &MakeLoopAddDeepTail;
	}

	// CDXFdata�̐ÓI�ϐ�������
	CDXFdata::ms_fXRev = GetFlg(MKNC_FLG_XREV);
	CDXFdata::ms_fYRev = GetFlg(MKNC_FLG_YREV);
	CDXFpoint::ms_pfnOrgDrillTuning = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ?
		&CDXFpoint::OrgTuning_Seq : &CDXFpoint::OrgTuning_XY;

	// CNCMake�̐ÓI�ϐ�������
	if ( !g_bData ) {
		// ABS, INC �֌W�Ȃ� G92�l�ŏ�����
		for ( int i=0; i<NCXYZ; i++ )
			CNCMake::ms_xyz[i] = RoundUp(GetDbl(MKNC_DBL_G92X+i));
	}
	// ������߼�݂ɂ��ÓI�ϐ��̏�����
	CNCMake::SetStaticOption();
}

BOOL SetStartData(void)
{
	ASSERT(CDXFdata::ms_pData);

	int		i, j, nLayerCnt = g_pDoc->GetLayerCnt(), nDataCnt;
	CDXFdata*	pData;
	CDXFdata*	pMatchData = NULL;
	CDXFdata*	pDataResult = NULL;
	CDXFcircleEx*	pStartCircle = NULL;
	double		dGap, dGapMin = HUGE_VAL;
	CString		strLayer;
	CLayerData*	pLayer;
	CDXFarray	obArray;

	obArray.SetSize(0, g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER));
	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutTarget() || pLayer->GetLayerType()!=DXFSTRLAYER )
			continue;
		// ���H�J�n�ʒu�w��ڲԂ�÷�ď�񌴓_����
		nDataCnt = pLayer->GetDxfTextSize();
		for ( j=0; j<nDataCnt && IsThread(); j++ ) {
			pData = pLayer->GetDxfTextData(j);
			pData->OrgTuning(FALSE);
			g_mpDXFstarttext.SetMakePointMap(pData);
		}
		// ���H�J�n�ʒu�w��ڲԂ̌��_�����ƈړ��J�n�ʒu����ٰ��(OrgTuning)
		nDataCnt = pLayer->GetDxfSize();
		for ( j=0; j<nDataCnt && IsThread(); j++ ) {
			pData = pLayer->GetDxfData(j);
			if ( pData->IsKindOf(RUNTIME_CLASS(CDXFcircleEx)) )
				pStartCircle = (CDXFcircleEx *)pData;
			else {
				dGap = pData->OrgTuning();
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
				}
				obArray.Add(pData);
			}
		}
	}
	g_pfnAddStartText = g_mpDXFstarttext.IsEmpty() ?
		&AddText_Exclude : &AddStartText_Target;
	// ���H�J�n�ʒu�w��ڲԂ̵�޼ު�ĉ��o�^(MakeLoop)
	while ( pDataResult && IsThread() ) {
		g_obStartData.Add(pDataResult);
		// �ް�ϰ�
		pDataResult->SetSearchFlg();
		// ���̗v�f����(GetNearPoint)
		pMatchData = pDataResult;
		pDataResult = NULL;
		dGapMin = HUGE_VAL;
		for ( i=0; i<obArray.GetSize() && IsThread(); i++ ) {
			pData = obArray[i];
			if ( pData->IsSearchFlg() )
				continue;
			if ( pData->IsMatchObject(pMatchData) ) {
				pDataResult = pData;
				break;
			}
			dGap = pData->GetEdgeGap(pMatchData);
			if ( dGap < dGapMin ) {
				pDataResult = pData;
				if ( sqrt(dGap) < NCMIN )	// �ړ��w��ڲԂ�NCMIN�Œ�
					break;
				dGapMin = dGap;
			}
		}	// End for loop
	}	// End while loop
	// ���H�J�n�ʒu�ɉ~�ް����g���Ă�����
	if ( pStartCircle && IsThread() ) {
		// ���_����
		pStartCircle->OrgTuning(FALSE);	// CDXFcircleEx::OrgTuning()
		// �ړ��ް����o�^
		if ( !pMatchData || pMatchData->GetEndMakePoint()!=pStartCircle->GetEndMakePoint() )
			g_obStartData.Add(pStartCircle);
	}

	return IsThread();
}

void SetGlobalArray_Single(void)
{
	int			i, j, nLayerCnt = g_pDoc->GetLayerCnt(), nDataCnt;
	CDXFdata*	pData;
	CLayerData*	pLayer;

	// �؍�Ώ�ڲ�(�\��ڲ�)���߰
	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( pLayer->IsCutTarget() && pLayer->IsCutType() ) {
			// �؍��ް����߰
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ )
				SetGlobalArray_Sub(pLayer->GetDxfData(j));
			// ÷���ް���ϯ�ߍ쐬
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfTextData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFtext.SetMakePointMap(pData);
			}
		}
	}
	// ���̑�ڲ��ް���ϯ�ߍ쐬
	SetGlobalMap_Other();
}

void SetGlobalArray_Multi(CLayerData* pLayer)
{
	int			i, nCnt;
	CDXFdata*	pData;

	g_obDXFdata.RemoveAll();
	g_mpDXFdata.RemoveAll();
	g_mpDXFmove.RemoveAll();
	g_mpDXFmovetext.RemoveAll();
	g_mpDXFtext.RemoveAll();
	g_mpDXFcomment.RemoveAll();
	g_obPoint.RemoveAll();
	g_obCircle.RemoveAll();
	g_ltDeepGlist.RemoveAll();
	g_obDrillGroup.RemoveAll();
	g_obDrillAxis.RemoveAll();
	g_mpEuler.RemoveAll();

	// �w��ڲԂ�DXF�ް���޼ު�Ă��߰
	nCnt = pLayer->GetDxfSize();
	for ( i=0; i<nCnt && IsThread(); i++ )
		SetGlobalArray_Sub(pLayer->GetDxfData(i));
	// ÷���ް���ϯ�ߍ쐬
	nCnt = pLayer->GetDxfTextSize();
	for ( i=0; i<nCnt && IsThread(); i++ ) {
		pData = pLayer->GetDxfTextData(i);
		pData->OrgTuning(FALSE);
		g_mpDXFtext.SetMakePointMap(pData);
	}
	// ���̑�ڲ��ް���ϯ�ߍ쐬
	SetGlobalMap_Other();
}

void SetGlobalArray_Sub(CDXFdata* pData)
{
	// �ȉ~�ް��̕ϐg(���a�Z�a���������ȉ~�Ȃ�)
	if ( GetFlg(MKNC_FLG_ELLIPSE) && pData->GetType()==DXFELLIPSEDATA &&
			fabs(1.0-((CDXFellipse *)pData)->GetShortMagni()) < EPS ) {	// �{��1.0
		pData->ChangeMakeType( ((CDXFellipse *)pData)->GetArc() ?
			DXFARCDATA : DXFCIRCLEDATA);	// �~�ʂ��~�ް��ɕϐg
	}
	// �e��޼ު�����߂��Ƃ̏���
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		// �����ŏd���������s�������������悢���C
		// OrgTuning() �O�ł͍��W���������ł��Ȃ� !!!
		g_obPoint.Add(pData);
		break;
	case DXFCIRCLEDATA:
		// �~�ް��̔��a���w��l�ȉ��Ȃ�
		if ( GetFlg(MKNC_FLG_DRILLCIRCLE) &&
				((CDXFcircle *)pData)->GetMakeR() <= GetDbl(MKNC_DBL_DRILLCIRCLE) ) {
			// �����H�ް��ɕϐg���ēo�^
			pData->ChangeMakeType(DXFPOINTDATA);
			g_obCircle.Add(pData);
			break;
		}
		// through
	default:
		// �u�����H�̂݁v�ȊO�Ȃ�
		if ( GetNum(MKNC_NUM_DRILLPROCESS) != 2 )
			g_obDXFdata.Add(pData);
		break;
	}
}

void SetGlobalMap_Other(void)
{
	// �������猴�_���������޼ު�Ă͋����v�Z�̕K�v�Ȃ�
	int			i, j, nType, nLayerCnt = g_pDoc->GetLayerCnt(), nDataCnt;
	CLayerData*	pLayer;
	CDXFdata*	pData;

	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutTarget() )
			continue;
		nType = pLayer->GetLayerType();
		if ( nType == DXFMOVLAYER ) {
			// �ړ��w��ڲԂ�ϯ�ߐ���
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFmove.SetMakePointMap(pData);
			}
			// �ړ��w��ڲ�÷�Ă�ϯ�ߐ���
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfTextData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFmovetext.SetMakePointMap(pData);
			}
		}
		else if ( nType == DXFCOMLAYER ) {
			// ����ڲԂ�ϯ�ߐ���
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfTextData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFcomment.SetMakePointMap(pData);
			}
		}
	}

#ifdef _DEBUGOLD
	CDumpContext	dc;
	dc.SetDepth(1);
	g_mpDXFmove.Dump(dc);
#endif

	// �ĂԂׂ��֐��̏�����
	g_pfnGetMatchPointMove = g_mpDXFmove.IsEmpty() ?
		&GetMatchPointMove_Exclude : &GetMatchPointMove_Target;
	g_pfnAddMoveText = g_mpDXFmovetext.IsEmpty() ?
		&AddText_Exclude : &AddMoveText_Target;
	g_pfnAddCutterText = g_mpDXFtext.IsEmpty() ?
		&AddText_Exclude : &AddCutterText_Target;
	g_pfnAddCommentText = g_mpDXFcomment.IsEmpty() ?
		&AddText_Exclude : &AddCommentText_Target;
}

BOOL MakeNCD_FinalFunc(LPCTSTR lpszFileName/*=NULL*/)
{
	// �ŏI�Ƽ��Z���W�ւ̕��A
	if ( GetNum(MKNC_NUM_ZRETURN)!=0 || CDXFdata::ms_pData->GetMakeType()!=DXFPOINTDATA )
		AddMoveGdata(0, g_dZInitial, -1);
	// G����̯��(�I������)
	AddCustomCode(g_pMakeOpt->GetStr(MKNC_STR_FOOTER), NULL);
	// ̧�ُo��̪���
	return OutputNCcode(lpszFileName);
}

BOOL OutputNCcode(LPCTSTR lpszFileName)
{
	CString	strPath, strFile,
			strNCFile(lpszFileName ? lpszFileName : g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_obMakeGdata.GetSize(), g_szFinal, strFile);
	try {
		CStdioFile	fp(strNCFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
		for ( int i=0; i<g_obMakeGdata.GetSize() && IsThread(); i++ ) {
			g_obMakeGdata[i]->WriteGcode(fp);
			SendProgressPos(i);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	g_pParent->m_ctReadProgress.SetPos(g_obMakeGdata.GetSize());
	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC����Ҳݽگ��
//////////////////////////////////////////////////////////////////////

// NC��������
BOOL MakeNCD_MainThread(CString& strLayer)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_MainThread()\nStart", DBG_MAGENTA);
#endif

	// ��۸�ڽ�ް������
	SetSearchRange();
	g_nFase = 1;

	// ���a�ŕ��בւ�
	if ( GetNum(MKNC_NUM_DRILLSORT) == 0 )
		g_obCircle.Sort(CircleSizeCompareFunc1);	// ����
	else
		g_obCircle.Sort(CircleSizeCompareFunc2);	// �~��
	// ���H�J�n�ʒu�w���ް��̾��
	if ( !g_bData && !g_obStartData.IsEmpty() )
		CDXFdata::ms_pData = g_obStartData.GetTail();

	// Ҳݕ���
	switch ( GetNum(MKNC_NUM_DRILLPROCESS) ) {
	case 0:		// ��Ɍ����H
		// �����H�ް��̏����ƍŒZ�J�n�ʒu���ް����
		if ( !CallMakeDrill_Sub(strLayer) )
			return FALSE;
		// �؍��ް��̏����ƍŒZ�J�n�ʒu���ް����
		// �����H�ް��̍Ō�ɋ߂��ް�����
		if ( !CallMakeLoop(MAKECUTTER, strLayer) )
			return FALSE;
		break;

	case 1:		// ��Ō����H
		// �؍��ް��̏����ƍŒZ�J�n�ʒu���ް����
		if ( !CallMakeLoop(MAKECUTTER, strLayer) )
			return FALSE;
		// through
	case 2:		// �����H�̂�
		// �����H�ް��̏����ƍŒZ�J�n�ʒu���ް����
		// �؍��ް��̍Ō�ɋ߂��ް�����
		if ( !CallMakeDrill_Sub(strLayer) )
			return FALSE;
		break;

	default:
		return FALSE;
	}

	// �؍��ް��I����̈ړ��w��ڲ�����
	return MakeLoopAddLastMove();
}

//////////////////////////////////////////////////////////////////////
// NC����Ҳݽگ�ޕ⏕�֐��Q
//////////////////////////////////////////////////////////////////////

BOOL CallMakeLoop(ENMAKETYPE enMake, CString& strLayer)
{
	CDXFdata*	pData;
	CString		strBuf;
	BOOL		bMatch;

	// ̪���1 ( OrgTuning_XXX)
	switch( enMake ) {
	case MAKECUTTER:
		tie(pData, bMatch) = OrgTuningCutter();
		break;
	case MAKEDRILLPOINT:
		tie(pData, bMatch) = OrgTuningDrillPoint();
		break;
	case MAKEDRILLCIRCLE:
		pData = OrgTuningDrillCircle();
		bMatch = FALSE;
		break;
	}

	if ( !IsThread() )
		return FALSE;
	if ( !pData )
		return TRUE;

	if ( !g_bData ) {
		g_bData = TRUE;
		// G����ͯ��(�J�n����)
		AddCustomCode(g_pMakeOpt->GetStr(MKNC_STR_HEADER), pData);
		// ���H�J�n�ʒu�w��ڲԏ���
		AddMakeStart();
	}
	else {
		// �����H�ȊO�őO�̐؍�I���ʒu�Ǝ��̐؍�J�n�ʒu���Ⴄ�Ȃ�
		if ( CDXFdata::ms_pData->GetMakeType() != DXFPOINTDATA &&
				CDXFdata::ms_pData->GetEndMakePoint() != pData->GetStartMakePoint() ) {
			// Z���̏㏸
			AddMoveGdata(0, g_dZReturn, -1.0);
		}
	}
	// ڲԂ��Ƃ̺���
	if ( !strLayer.IsEmpty() && GetFlg(MKNC_FLG_LAYERCOMMENT) ) {
		strBuf.Format(IDS_MAKENCD_LAYERBREAK, strLayer);
		AddMakeGdata(strBuf);
		// �������Ă����Ȃ��悤��ڲԖ���ر
		strLayer.Empty();
	}
	// ��]��
	strBuf = CNCMake::MakeSpindle(pData->GetMakeType());
	if ( !strBuf.IsEmpty() )
		AddMakeGdata(strBuf);

	// ̪���2
	SendFaseMessage();

	// NC����ٰ��
	BOOL	bResult = FALSE;
	switch ( enMake ) {
	case MAKECUTTER:
		bResult = MakeLoopEuler(bMatch, pData);
		break;
	case MAKEDRILLPOINT:
		bResult = MakeLoopDrillPoint(bMatch, pData);
		break;
	case MAKEDRILLCIRCLE:
		if ( bResult=MakeLoopDrillCircle() ) {
			// �~�ް��̏I������
			if ( GetFlg(MKNC_FLG_DRILLBREAK) ) {
				VERIFY(strBuf.LoadString(IDS_MAKENCD_CIRCLEEND));
				AddMakeGdata(strBuf);
			}
		}
		break;
	}

	return bResult;
}

BOOL CallMakeDrill_Sub(CString& strLayer)
{
	if ( GetFlg(MKNC_FLG_DRILLCIRCLE) ) {
		// �~�ް��������H�ް��Ƃ���ꍇ
		switch ( GetNum(MKNC_NUM_DRILLCIRCLEPROCESS) ) {
		case 1:	// �~����
			if ( !CallMakeLoop(MAKEDRILLCIRCLE, strLayer) )
				return FALSE;
			if ( !CallMakeLoop(MAKEDRILLPOINT, strLayer) )
				return FALSE;
			break;

		case 2:	// �~�����
			if ( !CallMakeLoop(MAKEDRILLPOINT, strLayer) )
				return FALSE;
			// through
		case 0:	// ���_����
			if ( !CallMakeLoop(MAKEDRILLCIRCLE, strLayer) )
				return FALSE;
			break;
		}
	}
	else {
		// ���_�ް����������H
		if ( !CallMakeLoop(MAKEDRILLPOINT, strLayer) )
			return FALSE;
	}

	// �Œ軲�ٷ�ݾ�
	AddMakeGdataCycleCancel();

	return TRUE;
}

tuple<CDXFdata*, BOOL> OrgTuningCutter(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningCutter()", DBG_GREEN);
#endif
	if ( g_obDXFdata.IsEmpty() )
		return NULL;

	// ̪���1
	SendFaseMessage(g_obDXFdata.GetSize());

	// ���_�����Ɛ؍�J�n�߲�Č���ٰ��
	BOOL	bMatch = FALSE;
	double	orgGap, orgGapMin = HUGE_VAL;	// ���_�܂ł̋���
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
#ifdef _DEBUG
	int	nResult;
#endif

	for ( int i=0; i<g_obDXFdata.GetSize() && IsThread(); i++ ) {
		pData = g_obDXFdata[i];
		// ���_�����Ƌ����v�Z + NC�����׸ނ̏�����
		orgGap = pData->OrgTuning();
		// ���_�Ɉ�ԋ߂��߲�Ă�T��
		if ( orgGap < orgGapMin ) {
			orgGapMin = orgGap;
			pDataResult = pData;
#ifdef _DEBUG
			nResult = i;
#endif
			if ( sqrt(orgGap) < CDXFmap::ms_dTolerance )
				bMatch = TRUE;
		}
		// ���Wϯ�߂ɓo�^
		g_mpDXFdata.SetMakePointMap(pData);
		//
		SendProgressPos(i);
	}
#ifdef _DEBUG
	dbg.printf("FirstPoint %d Gap=%f", nResult, orgGapMin);
#endif
	g_pParent->m_ctReadProgress.SetPos(g_obDXFdata.GetSize());

	return make_tuple(pDataResult, bMatch);
}

tuple<CDXFdata*, BOOL> OrgTuningDrillPoint(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillPoint()", DBG_GREEN);
#endif
	if ( g_obPoint.IsEmpty() )
		return NULL;

	// ̪���1
	SendFaseMessage(g_obPoint.GetSize());

	// �œK���ɔ����z��̺�߰��
	CDXFsort& obDrill = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? g_obDrillAxis : g_obDrillGroup;
	obDrill.SetSize(0, g_obPoint.GetSize());

	// ���_�����Ɛ؍�J�n�߲�Č���ٰ��
	int		i, j;
	BOOL	bMatch = FALSE, bCalc = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? TRUE : FALSE;
	double	orgGap, orgGapMin = HUGE_VAL;	// ���_�܂ł̋���
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
			// ������ݒ肳��Ă���Ɓu���_�Ɉ�ԋ߂��߲�Ă�T���v�̓��_�ȏ���
#ifdef _DEBUG
	int	nResult;
#endif

	// �����H��޼ު�Ă� g_obDrillGroup �ɺ�߰
	for ( i=0; i<g_obPoint.GetSize() && IsThread(); i++ ) {
		pData = g_obPoint[i];
		// ���_�����Ƌ����v�Z + NC�����׸ނ̏�����
		orgGap = pData->OrgTuning(bCalc);
		// �d�����W������
		if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
			for ( j=0; j<obDrill.GetSize() && IsThread(); j++ ) {
				if ( obDrill[j]->IsMatchObject(pData) ||
						sqrt(obDrill[j]->GetEdgeGap(pData, FALSE)) < GetDbl(MKNC_DBL_TOLERANCE) ) {
					pData->SetMakeFlg();
					break;
				}
			}
		}
		// ���_�Ɉ�ԋ߂��߲�Ă�T��
		if ( !pData->IsMakeFlg() ) {
			obDrill.Add(pData);
			if ( orgGap < orgGapMin ) {
				orgGapMin = orgGap;
				pDataResult = pData;
#ifdef _DEBUG
				nResult = i;
#endif
				if ( sqrt(orgGap) < GetDbl(MKNC_DBL_TOLERANCE) )
					bMatch = TRUE;
			}
		}
		SendProgressPos(i);
	}
#ifdef _DEBUG
	dbg.printf("FirstPoint %d Gap=%f", nResult, orgGapMin);
#endif

	g_pParent->m_ctReadProgress.SetPos(g_obPoint.GetSize());

	return make_tuple(pDataResult, bMatch);
}

CDXFdata* OrgTuningDrillCircle(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillCircle()", DBG_GREEN);
#endif
	if ( g_obCircle.IsEmpty() )
		return NULL;

	// ̪���1
	SendFaseMessage(g_obCircle.GetSize());

	// �K�����בւ������(���a���Ƃɏ���)�̂Ō��_�����̂�
	for ( int i=0; i<g_obCircle.GetSize() && IsThread(); i++ ) {
		g_obCircle[i]->OrgTuning(FALSE);
		SendProgressPos(i);
	}
	g_pParent->m_ctReadProgress.SetPos(g_obCircle.GetSize());

	// ��а�ް���Ԃ�
	return g_obCircle[0];
}

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopEuler(BOOL bMatch, CDXFdata* pData)
{
	// ---------------------------------------------------
	// MakeLoopEuler() �̏����S�ʂ́C���x�̍������W��n��
	// ---------------------------------------------------
	CDXFdata*	pDataMove;
	BOOL		bMove = FALSE, bCust = TRUE;
	int			i, nCnt, nPos = 0, nSetPos = 64;
	CPointD		pt;

	// OrgTuning()�Ō��݈ʒu�Ɠ�����W��������Ȃ�����
	if ( !bMatch ) {
		pDataMove = MakeLoopAddFirstMove(MAKECUTTER);
		if ( pDataMove )
			pData = pDataMove;
	}

	// GetNearPoint() �̌��ʂ� NULL �ɂȂ�܂�
	while ( pData && IsThread() ) {
		// CMap�ر�گ�ނ̏I���҂�
		g_pmClearMap.evEnd.Lock();
		g_pmClearMap.evEnd.ResetEvent();
		// ���� pData ����_��
		g_mpEuler.SetMakePointMap(pData);
		pData->SetSearchFlg();
		// pData �̎n�_�E�I�_�ň�M�����T��
		for ( i=0; i<pData->GetPointNumber(); i++ ) {
			pt = pData->GetTunPoint(i);
			if ( pt != HUGE_VAL ) {
				if ( !MakeLoopEulerSearch(pt) )
					return FALSE;
			}
		}
		// CMap��޼ު�Ă�NC����
		if ( (nCnt=MakeLoopEulerAdd()) < 0 )
			return FALSE;
		//
		nPos += nCnt;
		if ( nSetPos < nPos ) {
			g_pParent->m_ctReadProgress.SetPos(nPos);
			while ( nSetPos < nPos )
				nSetPos += nSetPos;
		}
		// ���̐؍��߲�Č���
		pData = CDXFdata::ms_pData;
		while ( IsThread() ) {
			// ���̗v�f�Ɉ�ԋ߂��v�f
			tie(pData, bMatch) = GetNearPointCutter(pData);
			if ( !pData )
				break;	// Ҳ�ٰ�ߏI������
			// ���������W�Ȃ�(==Z���̈ړ����K�v)
			if ( !bMatch ) {
				// �ړ��w��ڲԂ�����
				pDataMove = CDXFdata::ms_pData;
				if ( (*g_pfnGetMatchPointMove)(pDataMove) ) {	// �O���޼ު�ĂŒT��
					pData = pDataMove;
					if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPAPROCESS)==0 ) {
						g_ltDeepGlist.AddTail(pData);
						pData->SetMakeFlg();
					}
					else {
						bMove = TRUE;
						// �ړ��ް��O�̶��Ѻ��ޑ}��
						if ( bCust ) {
							(*g_pfnAddMoveCust_B)();
							bCust = FALSE;	// �A�����ē���Ȃ��悤��
						}
						// �w�����ꂽZ�ʒu�ňړ�
						(*g_pfnAddMoveZ)();
						// �ړ��ް��̐���
						AddMakeMove(pData);
						// �ړ��ް���Ҕ�
						CDXFdata::ms_pData = pData;
					}
					continue;	// �ĒT��
				}
			}
			// GetMatchPoint_Move() �� continue ����ȊO��
			break;
		}
		bCust = TRUE;
		// �ړ��ް���̶��Ѻ��ޑ}��
		if ( bMove && IsThread() ) {
			// �ړ��ް��̏I�_��÷���ް��̐���
			pt = CDXFdata::ms_pData->GetEndMakePoint();
			AddMoveTextIntegrated(pt);
			// ���Ѻ���
			(*g_pfnAddMoveCust_A)();
			bMove = FALSE;
		}
		// Z���̏㏸
		if ( pData && !GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_TOLERANCE)==0 )
			AddMoveGdataZup();
	} // End of while
	g_pParent->m_ctReadProgress.SetPos(nPos);

	// �S�̐[���̌㏈��
	if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPAPROCESS)==0 ) {
		if ( !MakeLoopDeepAdd() )
			return FALSE;
	}

	return IsThread();
}

BOOL MakeLoopEulerSearch(CPointD& pt)
{
	int			i, j;
	BOOL		bResult = TRUE;
	CPointD		ptSrc;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// ���W�𷰂�ϯ�ߌ���
	if ( g_mpDXFdata.Lookup(pt, pobArray) ) {
		for ( i=0; i<pobArray->GetSize() && bResult && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() && !pData->IsSearchFlg() ) {
				g_mpEuler.SetMakePointMap(pData);
				pData->SetSearchFlg();
				// ��޼ު�Ă̒[�_������Ɍ���
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					ptSrc = pData->GetTunPoint(j);
					if ( ptSrc != HUGE_VAL && pt != ptSrc ) {
						if ( !MakeLoopEulerSearch(ptSrc) ) {
							bResult = FALSE;
							break;
						}
					}
				}
			}
		}
	}
	if ( !IsThread() )
		bResult = FALSE;
	return bResult;
}

int MakeLoopEulerAdd(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopEulerAdd()", DBG_MAGENTA);
#endif
	int			i, nObCnt, nOddCnt = 0;
	BOOL		bEuler = FALSE;	// ��M�����v���𖞂����Ă��邩
	double		dGap, dGapMin = HUGE_VAL, dGapMin2 = HUGE_VAL;
	POSITION	pos;
	CPointD		pt, ptStart, ptStart2,
				ptKey( CDXFdata::ms_pData->GetEndCutterPoint() );
	CDXFdata*	pData;
	CDXFarray*	pArray;
	CDXFarray*	pStartArray = NULL;
	CDXFarray*	pStartArray2;
	CDXFlist	ltEuler(1024);		// ��M������

	// --- �ް�����
#ifdef _DEBUG
	dbg.printf("g_mpEuler.GetCount()=%d", g_mpEuler.GetCount());
	for ( pos = g_mpEuler.GetStartPosition(); pos; ) {
		g_mpEuler.GetNextAssoc(pos, pt, pArray);
		dbg.printf("pt.x=%f pt.y=%f", pt.x, pt.y);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			dbg.printf("Type=%d", pData->GetMakeType());
		}
	}
#endif
	// ��޼ު�ēo�^���̊��T�� + ����׸ނ̸ر
	for ( pos = g_mpEuler.GetStartPosition(); pos && IsThread(); ) {
		g_mpEuler.GetNextAssoc(pos, pt, pArray);
		// ���W���ɑ΂���o�^��޼ު�Đ�����̋ߐڵ�޼ު�Ă�����
		// (�~�ް��Ͷ��Ă��Ȃ����� obArray->GetSize() ���g���Ȃ�)
		for ( i=0, nObCnt=0; i<pArray->GetSize() && IsThread(); i++ ) {
			pData = pArray->GetAt(i);
			pData->ClearSearchFlg();
			if ( !pData->IsStartEqEnd() )	// �n�_�ɖ߂��ް��ȊO
				nObCnt++;
		}
		dGap = GAPCALC(ptKey - pt);
		if ( nObCnt & 0x01 ) {
			nOddCnt++;
			// ���D��I�Ɍ���
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pStartArray = pArray;
				ptStart = pt;
			}
		}
		else {
			// �����̏ꍇ�ł���ԋ߂����W���m��
			if ( dGap < dGapMin2 ) {
				dGapMin2 = dGap;
				pStartArray2 = pArray;
				ptStart2 = pt;
			}
		}
	}
	if ( !pStartArray ) {		// nOddCnt == 0
		bEuler = TRUE;		// ��M�����v���𖞂����Ă���
		// ����W���Ȃ��ꍇ�͋������W�̍ł��߂���޼ު�Ĕz����g��
		pStartArray = pStartArray2;
		ptStart = ptStart2;
	}
	else if ( nOddCnt == 2 ) {	// ��_���Q��
		bEuler = TRUE;		// ��M�����v���𖞂����Ă���
	}
#ifdef _DEBUG
	dbg.printf("FirstPoint x=%f y=%f", ptStart.x, ptStart.y);
#endif
	if ( !IsThread() )
		return -1;

	// --- ��M�����̐���(�ċA�Ăяo���ɂ��؍\�����)
	ASSERT( pStartArray );
	ASSERT( !pStartArray->IsEmpty() );
	MakeLoopEulerAdd_with_one_stroke(ptStart, pStartArray, ltEuler, bEuler);

	// --- �؍��ް�����
	ASSERT( !ltEuler.IsEmpty() );
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// �؍��ް�����
		for ( pos = ltEuler.GetHeadPosition(); pos; ) {
			pData = ltEuler.GetNext(pos);
			pData->SetMakeFlg();
		}
		g_ltDeepGlist.AddTail(&ltEuler);
		// �[�����S�̂��ۂ�
		if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
			g_ltDeepGlist.AddTail((CDXFdata *)NULL);	// Z���ړ���ϰ��
			// �Ō�ɐ��������ް���Ҕ�
			CDXFdata::ms_pData = pData;
		}
		else {
			if ( !MakeLoopDeepAdd() )
				return -1;
		}
	}
	else {
		// �؍��ް��܂ł̈ړ�
		pData = ltEuler.GetHead();
		if ( GetNum(MKNC_NUM_TOLERANCE) == 0 )
			AddMoveGdataG0(pData);
		else
			AddMoveGdataG1(pData);
		// �؍��ް�����
		for ( pos = ltEuler.GetHeadPosition(); pos; ) {
			pData = ltEuler.GetNext(pos);
			AddMakeGdata(pData);
		}
		// �Ō�ɐ��������ް���Ҕ�
		CDXFdata::ms_pData = pData;
	}

	// ��M�����v���𖞂����Ă��Ȃ��Ƃ�����
	if ( !bEuler ) {
		// ��M�����ɘR�ꂽ��޼ު�Ă̻���׸ނ�������
		for ( pos = g_mpEuler.GetStartPosition(); pos && IsThread(); ) {
			g_mpEuler.GetNextAssoc(pos, pt, pArray);
			for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
				pData = pArray->GetAt(i);
				if ( !pData->IsMakeFlg() )
					pData->ClearSearchFlg();
			}
		}
	}
	// CMap��޼ު�ĸر
	g_pmClearMap.evStart.SetEvent();

	return ltEuler.GetCount();
}

BOOL MakeLoopEulerAdd_with_one_stroke
	(CPointD& pt, CDXFarray* pArray, CDXFlist& ltEuler, BOOL bEuler)
{
	int			i;
	CDXFdata*	pData;
	CDXFarray*	pNextArray;
	CPointD		ptNext;
	POSITION	pos, posTail = ltEuler.GetTailPosition();	// ���̎��_�ł̉��o�^ؽĂ̍Ō�

	// �܂����̍��W�z��̉~(�ɏ�������)�ް������o�^
	for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() && pData->IsStartEqEnd() ) {
			pData->GetEdgeGap(pt);	// ��޼ު�Ă�pt�l��ptStart�̋߂����ɓ���ւ�
			ltEuler.AddTail( pData );
			pData->SetSearchFlg();
		}
	}

	// �~�ȊO���ް��Ŗ؍\���̎�������
	for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() ) {
			pData->GetEdgeGap(pt);
			ltEuler.AddTail(pData);
			pData->SetSearchFlg();
			ptNext = pData->GetEndCutterPoint();
			VERIFY( g_mpEuler.Lookup(ptNext, pNextArray) );	// Lookup()�Ŏ��s���邱�Ƃ͂Ȃ�
			// ���̍��W�z�������
			if ( MakeLoopEulerAdd_with_one_stroke(ptNext, pNextArray, ltEuler, bEuler) )
				return TRUE;	// �ċA�𔲂���
			// ���̍��W�z��̂��Ԗڂ̃m�[�h�ł͂Ȃ������̂�
			// ���o�^�������o�^ٰĂ͉���
			ltEuler.RemoveTail();
			pData->ClearSearchFlg();
		}
	}
	// �����Ώۂ������Ȃ���
	if ( !bEuler || !IsThread() ) {	// ��M�����̗v���𖞂����Ă��Ȃ�
		// �z���g�͍Œ��o�H��T��
		return TRUE;		// �����I��
	}
	else {
		// �S�����o�^�o�������ǂ���
		for ( pos = g_mpEuler.GetStartPosition(); pos && IsThread(); ) {
			g_mpEuler.GetNextAssoc(pos, pt, pArray);
			for ( i=pArray->GetSize(); --i>=0 && IsThread(); ) {
				if ( !pArray->GetAt(i)->IsSearchFlg() ) {
					pos = NULL;
					break;
				}
			}
		}
		if ( i < 0 )
			return TRUE;	// �S���I��
	}
	// ���̍��W�z��̌������I�������̂Ŗ؍\���̏�ʂֈړ��D
	// �~�ް����܂ޑS�Ẳ��o�^ؽĂ��폜
	ltEuler.GetNext(posTail);
	for ( pos=posTail; pos && IsThread(); pos=posTail) {
		pData = ltEuler.GetNext(posTail);	// ��Ɏ��̗v�f���擾
		pData->ClearSearchFlg();			// �׸ނ�������
		ltEuler.RemoveAt(pos);				// �v�f�폜
	}

	return FALSE;
}

BOOL MakeLoopDeepAdd(void)
{
	int			nCnt;
	BOOL		bAction = TRUE;		// �܂��͐�����
	double		dZCut = g_dZCut;	// g_dZCut�ޯ�����
	CDXFdata*	pData;
	POSITION	pos;

	if ( g_ltDeepGlist.IsEmpty() )
		return TRUE;

	// �Ō��Z���ړ�ϰ��͍폜
	if ( g_ltDeepGlist.GetTail() == NULL )
		g_ltDeepGlist.RemoveTail();
	// ��]��
	CString	strSpindle( CNCMake::MakeSpindle(DXFLINEDATA) );
	if ( !strSpindle.IsEmpty() )
		AddMakeGdata(strSpindle);
	// �؍��ް��܂ł̈ړ�
	ASSERT( g_ltDeepGlist.GetHead() );
	AddMoveGdataG0( g_ltDeepGlist.GetHead() );

	// �[�����u�S�́v�̏ꍇ�Cİ�ٌ���*�[���ï�߂���۸�ڽ���۰ق̍Đݒ�
	if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
		// İ�ٌ���(�[���ï�߶��Ă͐؂�グ)
		nCnt = (int)ceil(fabs((g_dDeep - g_dZCut) / GetDbl(MKNC_DBL_ZSTEP)))
			* g_ltDeepGlist.GetCount();
		SendFaseMessage( nCnt );
	}

	nCnt = 0;
	// �[���ŏI�ʒu�܂ŉ��o�^�ް���NC����
	// g_dZCut > g_dDeep �ł̏����ł͐��l�덷�����������Ƃ�ٰ�ߒE�o���Ȃ�����
	// g_dZCut - g_dDeep > EPS �Ƃ���
	while ( g_dZCut - g_dDeep > EPS && IsThread() ) {
		pData = bAction ? (*g_pfnDeepHead)(FALSE) : (*g_pfnDeepTail)(FALSE);
		if ( !IsThread() )
			return FALSE;
		CDXFdata::ms_pData = pData;
		// ����݂̐؂�ւ�(�����؍�̂�)
		if ( GetNum(MKNC_NUM_DEEPCPROCESS) == 0 ) {
			bAction = !bAction;
			// �e��޼ު�Ă̎n�_�I�_�����ւ�
			for ( pos = g_ltDeepGlist.GetHeadPosition(); pos && IsThread(); ) {
				pData = g_ltDeepGlist.GetNext(pos);
				if ( pData )
					pData->ReversePt();
			}
		}
		// Z���̉��~
		g_dZCut += GetDbl(MKNC_DBL_ZSTEP);
		if ( g_dZCut - g_dDeep > EPS ) {
			// ����ʍs�؍������
			MakeLoopDeepZDown();
		}
		// ��۸�ڽ�ް�̍X�V
		if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 )
			g_pParent->m_ctReadProgress.SetPos(++nCnt * g_ltDeepGlist.GetCount());
	}
	if ( !IsThread() )
		return FALSE;

	// �ŏIZ�l
	g_dZCut = g_dDeep;

	// �ŏI�d�グ��߼�݂̓K�p
	BOOL	bFinish;
	if ( GetFlg(MKNC_FLG_DEEPFINISH) ) {
		bFinish = TRUE;
		// �ʏ��]���Ǝd�グ��]�����Ⴄ�Ƃ�
		if ( GetNum(MKNC_NUM_SPINDLE) != GetNum(MKNC_NUM_DEEPSPINDLE) ) {
			// Z���㏸
			MakeLoopDeepZUp();
			// �d�グ�p��]���ɕύX
			AddMakeGdata( CNCMake::MakeSpindle(DXFLINEDATA, TRUE) );
			// ��޼ު�Đ؍�ʒu�ֈړ�
			pData = bAction ? g_ltDeepGlist.GetHead() : g_ltDeepGlist.GetTail();
			AddMoveGdataG0(pData);
		}
		else {
			// �d�グ�ʂւ�Z�����~
			MakeLoopDeepZDown();
		}
	}
	else {
		bFinish = FALSE;
		// �d�グ�ʂւ�Z�����~
		MakeLoopDeepZDown();
	}

	// �d�グ�ʂ��ް�����
	CDXFdata::ms_pData = bAction ? (*g_pfnDeepHead)(bFinish) : (*g_pfnDeepTail)(bFinish);

	// �[���؍�ɂ�����Z���̏㏸
	MakeLoopDeepZUp();

	// ��n��
	g_ltDeepGlist.RemoveAll();
	g_dZCut = dZCut;

	return IsThread();
}

CDXFdata* MakeLoopAddDeepHead(BOOL bDeep)
{
	CDXFdata*	pData;
	for ( POSITION pos = g_ltDeepGlist.GetHeadPosition(); pos && IsThread(); ) {
		pData = g_ltDeepGlist.GetNext(pos);
		AddMakeGdataDeep(pData, bDeep);
	}
	return pData;
}

CDXFdata* MakeLoopAddDeepTail(BOOL bDeep)
{
	CDXFdata*	pData;
	for ( POSITION pos = g_ltDeepGlist.GetTailPosition(); pos && IsThread(); ) {
		pData = g_ltDeepGlist.GetPrev(pos);
		AddMakeGdataDeep(pData, bDeep);
	}
	return pData;
}

CDXFdata* MakeLoopAddDeepHeadAll(BOOL bDeep)
{
	BOOL		bMoveZ = FALSE;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;
	for ( POSITION pos = g_ltDeepGlist.GetHeadPosition(); pos && IsThread(); ) {
		pData = g_ltDeepGlist.GetNext(pos);
		if ( pData ) {
			// �P�O�̵�޼ު�Ă��ړ��ް��Ȃ�
			if ( pDataResult && !pDataResult->GetLayerData()->IsCutType() )
				AddMoveGdataZdown();	// Z���̉��~
			MakeLoopAddDeepZProc(bMoveZ, bDeep, pData);
			bMoveZ = FALSE;
			pDataResult = pData;
		}
		else
			bMoveZ = TRUE;
	}
	ASSERT(pDataResult);
	return pDataResult;
}

CDXFdata* MakeLoopAddDeepTailAll(BOOL bDeep)
{
	BOOL		bMoveZ = FALSE;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;
	for ( POSITION pos = g_ltDeepGlist.GetTailPosition(); pos && IsThread(); ) {
		pData = g_ltDeepGlist.GetPrev(pos);
		if ( pData ) {
/*
			// �t���̏ꍇ�� NULL��ڲ� ���L���ɓ����̂ŁC�ȉ��̏����͕s�v
			if ( pDataResult && !pDataResult->GetLayerData()->IsCutType() )
				AddMoveGdataZdown();
*/
			MakeLoopAddDeepZProc(bMoveZ, bDeep, pData);
			bMoveZ = FALSE;
			pDataResult = pData;
		}
		else
			bMoveZ = TRUE;
	}
	ASSERT(pDataResult);
	return pDataResult;
}

void MakeLoopAddDeepZProc(BOOL bMoveZ, BOOL bDeep, CDXFdata* pData)
{
	if ( pData->GetLayerData()->IsCutType() ) {
		if ( bMoveZ ) {		// Z���ړ�ϰ�
			MakeLoopDeepZUp();	// Z���̏㏸
			AddMoveGdataG0(pData);	// pData�܂ňړ�
		}
		// �؍��ް��̐���
		AddMakeGdataDeep(pData, bDeep);
	}
	else {
		// Z���̕��A
		if ( GetNum(MKNC_NUM_MOVEZ) != 0 )		// �u���̂܂܁v�ȊO�Ȃ�
			MakeLoopDeepZUp();	// Z���̏㏸
		// �ړ��ް��̐���
		AddMakeMove(pData);
	}
}

void MakeLoopDeepZDown(void)
{
	const CDXFdata* pDataHead = g_ltDeepGlist.GetHead();
	const CDXFdata* pDataTail = g_ltDeepGlist.GetTail();

	// �����؍킩��A�̵�޼ު�Ă���ٰ�߂Ȃ�
	if ( GetNum(MKNC_NUM_DEEPCPROCESS)==0 ||
			pDataHead->GetStartMakePoint()==pDataTail->GetEndMakePoint() ) {
		// ���̐[�����W�ցCZ���̍~���̂�
		AddMoveGdata(1, g_dZCut, GetDbl(MKNC_DBL_ZFEED));
		return;
	}

	// ����ʍs�؍�̏ꍇ
	// �܂�Z���̏㏸
	MakeLoopDeepZUp();
	// �擪��޼ު�ĂɈړ�
	AddMoveGdataG0(pDataHead);
}

void MakeLoopDeepZUp(void)
{
	if ( GetNum(MKNC_NUM_DEEPZPROCESS) == 0 ) {
		// �������Z�����A
		AddMoveGdataZup();
	}
	else {
		// R�_�܂Ő؍푗���Z�����A
		AddMoveGdata(1, g_dZG0Stop, GetDbl(MKNC_DBL_MAKEENDFEED));
		// �Ƽ�ٓ_���A�Ȃ�
		if ( GetNum(MKNC_NUM_ZRETURN) == 0 )
			AddMoveGdataZup();
	}
}

BOOL MakeLoopDrillPoint(BOOL bMatch, CDXFdata* pData)
{
	BOOL	bResult = FALSE;

	// ����ŕ��בւ��C�������ɂ����ް��𒊏o
	switch ( GetNum(MKNC_NUM_OPTIMAIZEDRILL) ) {
	case 1:		// X���
		g_obDrillAxis.SetSize(0, g_obDrillGroup.GetSize());
		g_obDrillGroup.Sort(DrillOptimaizeCompareFuncY);
		// ���݈ʒu�ɋ߂�������
		bResult = MakeLoopDrillPointSeqChk(FALSE, g_obDrillGroup) ?
			MakeLoopDrillPointXY(TRUE) : MakeLoopDrillPointXYRevers(TRUE);
		break;

	case 2:		// Y���
		g_obDrillAxis.SetSize(0, g_obDrillGroup.GetSize());
		g_obDrillGroup.Sort(DrillOptimaizeCompareFuncX);
		bResult = MakeLoopDrillPointSeqChk(TRUE, g_obDrillGroup) ?
			MakeLoopDrillPointXY(FALSE) : MakeLoopDrillPointXYRevers(FALSE);
		break;

	default:	// �Ȃ�
		bResult = MakeLoopAddDrill(bMatch, pData);
		break;
	}

	return bResult;
}

BOOL MakeLoopDrillPointSeqChk(BOOL bXY, CDXFsort& pObArray)	// bXY==TRUE -> X��
{
	if ( pObArray.GetSize() <= 1 )	// �P���Ȃ��ׂ�܂ł��Ȃ�
		return TRUE;

	CDXFdata*	pData1 = pObArray.GetHead();
	CDXFdata*	pData2 = pObArray.GetTail();

#ifdef _DEBUG
	g_dbg.printf("DrillSeqChk(): ArraySize=%d", pObArray.GetSize());
	g_dbg.printf("[ 0 ] x=%f y=%f",
		pData1->GetEndCutterPoint().x, pData1->GetEndCutterPoint().y );
	g_dbg.printf("[MAX] x=%f y=%f",
		pData2->GetEndCutterPoint().x, pData2->GetEndCutterPoint().y );
#endif

	// ���݈ʒu����̂ǂ��炪�߂���(�����ł͂Ȃ������)
	if ( bXY ) {
		if ( fabs(pData1->GetEndCutterPoint().x - CDXFdata::ms_pData->GetEndCutterPoint().x) <=
			 fabs(pData2->GetEndCutterPoint().x - CDXFdata::ms_pData->GetEndCutterPoint().x) )
			return TRUE;	// �擪���߂�
		else
			return FALSE;	// �������߂�
	}
	else {
		if ( fabs(pData1->GetEndCutterPoint().y - CDXFdata::ms_pData->GetEndCutterPoint().y) <=
			 fabs(pData2->GetEndCutterPoint().y - CDXFdata::ms_pData->GetEndCutterPoint().y) )
			return TRUE;	// �擪���߂�
		else
			return FALSE;	// �������߂�
	}
}

BOOL MakeLoopDrillPointXY(BOOL bXY)
{
	int		i = 0, nPos = 0, n;
	double	dBase, dMargin = fabs(GetDbl(MKNC_DBL_DRILLMARGIN)) * 2.0;
	CDXFsort::PFNCOMPARE	pfnCompare;

	if ( bXY ) {
		n = 1;
		pfnCompare = DrillOptimaizeCompareFuncX;
	}
	else {
		n = 0;
		pfnCompare = DrillOptimaizeCompareFuncY;
	}

	while ( i < g_obDrillGroup.GetSize() && IsThread() ) {
		dBase = g_obDrillGroup[i]->GetEndCutterPoint()[n] + dMargin;
#ifdef _DEBUG
		g_dbg.printf("BasePoint=%f", dBase);
#endif
		g_obDrillAxis.RemoveAll();
		while ( i < g_obDrillGroup.GetSize() &&
				g_obDrillGroup[i]->GetEndCutterPoint()[n] <= dBase && IsThread() ) {
#ifdef _DEBUG
			g_dbg.printf("NowPoint=%f", g_obDrillGroup[i]->GetEndCutterPoint()[n]);
#endif
			g_obDrillAxis.Add(g_obDrillGroup[i++]);
		}
		if ( g_obDrillAxis.IsEmpty() )
			continue;
		g_obDrillAxis.Sort(pfnCompare);
		if ( !MakeLoopDrillPointSeqChk(bXY, g_obDrillAxis) )
			g_obDrillAxis.MakeReverse();
		if ( !MakeLoopAddDrillSeq(nPos) )
			return FALSE;
		nPos = i;
	}

	return IsThread();
}

BOOL MakeLoopDrillPointXYRevers(BOOL bXY)
{
	int		i = g_obDrillGroup.GetUpperBound(), nPos = 0, n;
	double	dBase, dMargin = fabs(GetDbl(MKNC_DBL_DRILLMARGIN)) * 2.0;
	CDXFsort::PFNCOMPARE	pfnCompare;

	if ( bXY ) {
		n = 1;
		pfnCompare = DrillOptimaizeCompareFuncX;
	}
	else {
		n = 0;
		pfnCompare = DrillOptimaizeCompareFuncY;
	}

	while ( i >= 0 && IsThread() ) {
		dBase = g_obDrillGroup[i]->GetEndCutterPoint()[n] - dMargin;
#ifdef _DEBUG
		g_dbg.printf("BasePoint=%f", dBase);
#endif
		g_obDrillAxis.RemoveAll();
		while ( i >= 0 &&
				g_obDrillGroup[i]->GetEndCutterPoint()[n] >= dBase && IsThread() ) {
#ifdef _DEBUG
			g_dbg.printf("NowPoint=%f", g_obDrillGroup[i]->GetEndCutterPoint()[n]);
#endif
			g_obDrillAxis.Add(g_obDrillGroup[i--]);
		}
		if ( g_obDrillAxis.IsEmpty() )
			continue;
		g_obDrillAxis.Sort(pfnCompare);
		if ( !MakeLoopDrillPointSeqChk(bXY, g_obDrillAxis) )
			g_obDrillAxis.MakeReverse();
		if ( !MakeLoopAddDrillSeq(nPos) )
			return FALSE;
		nPos = g_obDrillAxis.GetSize();
	}

	return IsThread();
}

BOOL MakeLoopDrillCircle(void)
{
	BOOL	bMatch;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	int		i, j;
	double	r, orgGap, orgGapMin;
	CString	strBreak;

	// g_obCircle �̏d�����W���폜
	if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
		for ( i=0; i<g_obCircle.GetSize() && IsThread(); i++ ) {
			pData = g_obCircle[i];
			for ( j=i+1; j<g_obCircle.GetSize() && IsThread(); j++ ) {
				pDataResult = g_obCircle[j];
				if ( pDataResult->IsMatchObject(pData) ||
						sqrt(pDataResult->GetEdgeGap(pData, FALSE)) < GetDbl(MKNC_DBL_TOLERANCE) )
					pDataResult->SetMakeFlg();
			}
		}
	}

	// ٰ�ߊJ�n
	g_obDrillGroup.SetSize(0, g_obCircle.GetSize());
	for ( i=0; i<g_obCircle.GetSize() && IsThread(); ) {
		g_obDrillGroup.RemoveAll();
		// �~���ٰ��(���a)���Ƃɏ���
		r = fabs( ((CDXFcircle *)g_obCircle[i])->GetMakeR() );
		bMatch = FALSE;
		pDataResult = NULL;
		orgGapMin = HUGE_VAL;
		for ( ; i<g_obCircle.GetSize() &&
				r==fabs(((CDXFcircle *)g_obCircle[i])->GetMakeR()) && IsThread(); i++ ) {
			pData = g_obCircle[i];
			if ( !pData->IsMakeFlg() ) {
				g_obDrillGroup.Add(pData);
				orgGap = pData->GetEdgeGap(CDXFdata::ms_pData);
				if ( orgGap < orgGapMin ) {
					orgGapMin = orgGap;
					pDataResult = pData;
					if ( sqrt(orgGap) < GetDbl(MKNC_DBL_TOLERANCE) )
						bMatch = TRUE;
				}
			}
		}
		// ���ꔼ�a����ڲ�
		if ( pDataResult ) {
			// �傫�����Ƃɺ��Ă𖄂ߍ���
			if ( GetFlg(MKNC_FLG_DRILLBREAK) ) {
				strBreak.Format(IDS_MAKENCD_CIRCLEBREAK, ((CDXFcircle *)pDataResult)->GetMakeR());
				AddMakeGdata(strBreak);
			}
			// �ް�����
			if ( GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 )
				g_obDrillAxis.Copy(g_obDrillGroup);	// ����Ȃ��̂őS�đΏ�
			if ( !MakeLoopDrillPoint(bMatch, pDataResult) )
				return FALSE;
		}
	}

	return IsThread();
}

BOOL MakeLoopAddDrill(BOOL bMatch, CDXFdata* pData)
{
	CDXFdata*	pDataMove;
	CPointD		pt;
	int			nPos = 0;
	BOOL		bMove = FALSE,		// �ړ�ڲ�Hit
				bCust = TRUE;		// �ړ��ް��O�̶��Ѻ��ޑ}��

	// OrgTuning()�Ō��݈ʒu�Ɠ�����W��������Ȃ�����
	// DrillCircle�n�͕��בւ����Ă��珈������K�v������̂ŃR�R�ŏ���
	if ( !bMatch ) {
		pDataMove = MakeLoopAddFirstMove(MAKEDRILLPOINT);
		if ( pDataMove )
			pData = pDataMove;
	}

	// �ް�����
	AddMakeGdata(pData);

	// GetNearPoint() �̌��ʂ� NULL �ɂȂ�܂�
	while ( IsThread() ) {
		// �Ō�ɐ��������ް���Ҕ�
		CDXFdata::ms_pData = pData;
		// ���̗v�f�Ɉ�ԋ߂��v�f
		tie(pData, bMatch) = GetNearPointDrill(pData);
		if ( !pData )
			break;
		if ( !bMatch ) {
			// �ړ��w��ڲԂ�����
			pDataMove = CDXFdata::ms_pData;
			if ( (*g_pfnGetMatchPointMove)(pDataMove) ) {	// �O���޼ު�ĂŒT��
				// �ړ�ڲԂP�ڂ̏I�[�Ɍ����H�ް���Hit�����
				tie(pData, bMatch) = GetNearPointDrill(pDataMove);
				if ( !bMove && bMatch ) {
					// �ړ�ڲ��ް��͏����������Ƃɂ��āC���̌����H�ް��𐶐�
					pData->SetMakeFlg();
				}
				else {
					bMove = TRUE;
					// �Œ軲�ٷ�ݾ�
					AddMakeGdataCycleCancel();
					// �ړ��ް��O�̶��Ѻ��ޑ}��
					if ( bCust ) {
						(*g_pfnAddMoveCust_B)();
						bCust = FALSE;
					}
					// �w�����ꂽZ�ʒu�ňړ�
					(*g_pfnAddMoveZ)();
					// �ړ��ް��̐���
					AddMakeMove(pDataMove);
					pData = pDataMove;
					continue;	// �ĒT��
				}
			}
		}
		bCust = TRUE;
		// �ړ��ް���̶��Ѻ��ޑ}��
		if ( bMove && IsThread() ) {
			// �ړ��ް��̏I�_��÷���ް��̐���
			pt = CDXFdata::ms_pData->GetEndMakePoint();
			AddMoveTextIntegrated(pt);
			// ���Ѻ���
			(*g_pfnAddMoveCust_A)();
			bMove = FALSE;
		}
		// �ް�����
		AddMakeGdata(pData);
		SendProgressPos(++nPos);
	} // End of while

	g_pParent->m_ctReadProgress.SetPos(nPos);

	return IsThread();
}

BOOL MakeLoopAddDrillSeq(int nPos)
{
	CDXFdata*	pData;
	// g_obDrillAxis�̏��Ԃ�
	for ( int i=0; i<g_obDrillAxis.GetSize() && IsThread(); i++ ) {
		pData = g_obDrillAxis[i];
		// �ް�����
		AddMakeGdata(pData);
		SendProgressPos(i+nPos);
	}
	g_pParent->m_ctReadProgress.SetPos(i+nPos);
	// �Ō�ɐ��������ް���Ҕ�
	CDXFdata::ms_pData = pData;

	return IsThread();
}

CDXFdata* MakeLoopAddFirstMove(ENMAKETYPE enType)
{
	CDXFdata*	pDataMove = CDXFdata::ms_pData;
	CDXFdata*	pDataResult = NULL;
	BOOL		bMatch, bCust = FALSE;

	// �ړ��w��ڲԂ�����
	while ( (*g_pfnGetMatchPointMove)(pDataMove) && IsThread() ) {
		if ( !bCust ) {
			// �ړ��ް��O�̶��Ѻ��ޑ}��(�P�񂾂�)
			(*g_pfnAddMoveCust_B)();
			bCust = TRUE;
		}
		// �w�����ꂽZ�ʒu�ňړ�
		(*g_pfnAddMoveZ)();
		// �ړ��ް��̐���
		pDataResult = pDataMove;
		AddMakeMove(pDataResult);
		CDXFdata::ms_pData = pDataResult;
		// ������W�Ő؍��ް���������� break
		if ( enType == MAKECUTTER ) {
			tie(pDataResult, bMatch) = GetNearPointCutter(pDataResult);
			if ( bMatch )
				break;
		}
		else {
			tie(pDataResult, bMatch) = GetNearPointDrill(pDataResult);
			if ( bMatch )
				break;
		}
	}
	// �ړ��ް���̶��Ѻ��ޑ}��
	if ( bCust && IsThread() ) {
		// �ړ��ް��̏I�_��÷���ް��̐���
		CPointD	pt( CDXFdata::ms_pData->GetEndMakePoint() );
		AddMoveTextIntegrated(pt);
		// ���Ѻ���
		(*g_pfnAddMoveCust_A)();
	}

	return pDataResult;
}

BOOL MakeLoopAddLastMove(void)
{
	CDXFdata*	pDataMove = CDXFdata::ms_pData;
	BOOL		bCust = FALSE;

	// �I�_���W�ł̺��Đ���
	CPointD	pt( pDataMove->GetEndMakePoint() );
	AddCutterTextIntegrated(pt);	// �؍�ڲ�
	AddMoveTextIntegrated(pt);		// �ړ�ڲ�

	// �Ō�̈ړ����ނ�����
	while ( (*g_pfnGetMatchPointMove)(pDataMove) && IsThread() ) {
		if ( !bCust ) {
			(*g_pfnAddMoveCust_B)();
			bCust = TRUE;
		}
		(*g_pfnAddMoveZ)();
		AddMakeMove(pDataMove);
		CDXFdata::ms_pData = pDataMove;
	}
	if ( bCust && IsThread() ) {
		CPointD	pt( CDXFdata::ms_pData->GetEndMakePoint() );
		AddMoveTextIntegrated(pt);
		(*g_pfnAddMoveCust_A)();
	}

	return IsThread();
}

tuple<CDXFdata*, BOOL> GetNearPointCutter(CDXFdata* pData)
{
	CDXFdata*	pDataResult = NULL;
	BOOL		bMatch;
	int			i, nResult, nThreadCnt = g_nProcesser;
	CWordArray	obThread;	// �I���گ�ވꗗ
	if ( g_nProcesser > 1 )
		obThread.SetSize(0, g_nProcesser);

	// ��޼ު�Č����J�n
	for ( i=0; i<g_nProcesser; i++ ) {
		g_lpSearchResult[i].pData  = pData;
		g_lpSearchResult[i].bMatch = FALSE;
		g_lpmSearch[i].bResult = TRUE;
		g_lpmSearch[i].evStart.SetEvent();
	}

	while ( --nThreadCnt >= 0 ) {
		// ��޼ު�Č����I���҂�(�ǂꂩ���I������Ηǂ�)
		// CMultiLock�ł�g_lpmSearch[n].evEnd�̈������n���Ȃ��̂�
		// �����WaitForMultipleObjects()���g��
		nResult = WaitForMultipleObjects(g_nProcesser, g_phSearchEnd, FALSE, INFINITE) - WAIT_OBJECT_0;
		// �I�������گ�ނ̌��ʌ���
		if ( nResult < 0 || nResult >= g_nProcesser )
			break;
		// �e�گ�ތ��ʂ̌���(����Ō�����Α����͒��f)
		if ( g_lpSearchResult[nResult].bMatch /*&&g_lpSearchResult[nResult].pData*/ ) {
			pDataResult = g_lpSearchResult[nResult].pData;
			for ( i=0; i<g_nProcesser; i++ )
				g_lpmSearch[i].bResult = FALSE;
			// ���ɏI�������گ�޲���ď���
			for ( i=0; i<obThread.GetSize(); i++ )
				g_lpmSearch[obThread[i]].evEnd.SetEvent();
			// �S�Ă̽گ�ނ��I������܂ő҂�
			WaitForMultipleObjects(g_nProcesser, g_phSearchEnd, TRUE, INFINITE);
			for ( i=0; i<g_nProcesser; i++ )
				g_lpmSearch[i].evEnd.ResetEvent();
			bMatch = TRUE;
			break;
		}
		// ����WaitForMultipleObjects()�ł� nResult �ȊO�̽گ�ނő҂K�v������
		g_lpmSearch[nResult].evEnd.ResetEvent();
		if ( g_nProcesser > 1 )
			obThread.Add(nResult);
	}

	if ( nThreadCnt < 0 ) {
		// �e�گ�ނ̌��ʂ��r
		double	dGap = HUGE_VAL;	// �w��_�Ƃ̋���
		for ( i=0; i<g_nProcesser; i++ ) {
			g_lpmSearch[i].evEnd.ResetEvent();
			if ( g_lpSearchResult[i].pData && dGap > g_lpSearchResult[i].dGap ) {
				dGap = g_lpSearchResult[i].dGap;
				pDataResult = g_lpSearchResult[i].pData;
			}
		}
		bMatch = FALSE;
	}

	return make_tuple(pDataResult, bMatch);
}

tuple<CDXFdata*, BOOL> GetNearPointDrill(CDXFdata* pData)
{
	CDXFdata*	pDataSrc = pData;
	CDXFdata*	pDataResult = NULL;
	BOOL	bMatch = FALSE;
	double	dGap, dGapMin = HUGE_VAL;	// �w��_�Ƃ̋���

	// ���݈ʒu�Ɠ������C�܂��͋߂��v�f������
	for ( int i=0; i<g_obDrillAxis.GetSize() && IsThread(); i++ ) {
		pData = g_obDrillAxis[i];
		if ( pData->IsMakeFlg() )
			continue;
		// �������f
		if ( pData->IsMatchObject(pDataSrc) ) {
			pDataResult = pData;
			bMatch = TRUE;
			break;
		}
		// ���݈ʒu�Ƃ̋����v�Z
		dGap = pData->GetEdgeGap(pDataSrc);
		if ( dGap < dGapMin ) {
			pDataResult = pData;
			if ( sqrt(dGap) < GetDbl(MKNC_DBL_TOLERANCE) ) {
				bMatch = TRUE;
				break;
			}
			dGapMin = dGap;
		}
	}

	return make_tuple(pDataResult, bMatch);
}

BOOL GetMatchPointMove_Target(CDXFdata*& pDataResult)
{
	// ���݈ʒu�Ɠ������v�f����������
	CPointD	pt( pDataResult->GetEndMakePoint() );
	CDXFarray*	pobArray;
	BOOL		bMatch = FALSE;
	
	if ( g_mpDXFmove.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pData->GetEdgeGap(pt);	// �n�_�𒲐�
				pDataResult = pData;
				bMatch = TRUE;
				break;
			}
		}
	}

	return bMatch;
}

BOOL GetMatchPointMove_Exclude(CDXFdata*&)
{
	return FALSE;
}

// ����ͯ�ް, ̯�ް����
struct CMakeCustomCode	// parse() ����Ăяo��
{
	string&	m_strResult;
	const CDXFdata*	m_pData;

	CMakeCustomCode(string& r, const CDXFdata* pData) : m_strResult(r), m_pData(pData) {}

	void operator()(const char* s, const char* e) const
	{
		extern	const	DWORD	g_dwSetValFlags[];
		static	LPCTSTR	szCustomCode[] = {
			"G90orG91", "G92_INITIAL", "G92X", "G92Y", "G92Z",
			"SPINDLE", "G0XY_INITIAL",
			"MakeUser", "MakeDate", "MakeTime", "MakeNCD", "MakeDXF", "MakeCondition"
		};
		static	CStringKeyIndex	stOrder(SIZEOF(szCustomCode), szCustomCode);
		static	LPCTSTR	szReplaceErr = "???";

		string	str(s+1, e-1);	// �O��� "{}" ����
		CString	strBuf;
		TCHAR	szUserName[_MAX_PATH];
		DWORD	dwResult;
		CTime	time;
		double	dValue[VALUESIZE];
		int		nTestCode = stOrder.GetIndex(str.c_str());
		// replace
		switch ( nTestCode ) {
		case 0:		// G90orG91
			m_strResult += CNCMake::MakeCustomString(GetNum(MKNC_NUM_G90)+90);
			break;
		case 1:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			dValue[NCA_Z] = GetDbl(MKNC_DBL_G92Z);
			m_strResult += CNCMake::MakeCustomString(92, dValue, NCD_X|NCD_Y|NCD_Z, FALSE);
			break;
		case 2:		// G92X
		case 3:		// G92Y
		case 4:		// G92Z
			dValue[nTestCode-2] = GetDbl(MKNC_DBL_G92X+nTestCode-2);
			m_strResult += CNCMake::MakeCustomString(-1, dValue,
								g_dwSetValFlags[nTestCode-2], FALSE);
			break;
		case 5:		// SPINDLE
			if ( m_pData )					// Header
				m_strResult += CNCMake::MakeSpindle(m_pData->GetMakeType());
			else if ( CDXFdata::ms_pData )	// Footer
				m_strResult += CNCMake::MakeSpindle(CDXFdata::ms_pData->GetMakeType());
			break;
		case 6:		// G0XY_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			m_strResult += CNCMake::MakeCustomString(0, dValue, NCD_X|NCD_Y);
			break;
		case 7:		// MakeUser
			dwResult = _MAX_PATH;
			// հ�ޖ��Ɋ������܂܂�Ă���Ɛ������Ȃ�
			m_strResult += GetUserName(szUserName, &dwResult) && IsNCchar(szUserName) ?
				szUserName : szReplaceErr;
			break;
		case 8:		// MakeDate
			time = CTime::GetCurrentTime();
			VERIFY(strBuf.LoadString(ID_INDICATOR_DATE_F2));// %y/%m/%d
			m_strResult += time.Format(strBuf);
			break;
		case 9:		// MakeTime
			time = CTime::GetCurrentTime();
			VERIFY(strBuf.LoadString(ID_INDICATOR_TIME_F));	// %H:%M
			m_strResult += time.Format(strBuf);
			break;
		case 10:	// MakeNCD
			Path_Name_From_FullPath(g_pDoc->GetNCFileName(), CString(), strBuf);
			m_strResult += IsNCchar(strBuf) ? strBuf : szReplaceErr;
			break;
		case 11:	// MakeDXF
			strBuf = g_pDoc->GetTitle();
			m_strResult += IsNCchar(strBuf) ? strBuf : szReplaceErr;
			break;
		case 12:	// MakeCondition
			Path_Name_From_FullPath(g_pMakeOpt->GetInitFile(), CString(), strBuf);
			m_strResult += IsNCchar(strBuf) ? strBuf : szReplaceErr;
			break;
		}
	}
};

struct CMakeCustomCode2	// parse() ����Ăяo��
{
	string&	strResult;
	CMakeCustomCode2(string& r) : strResult(r) {}

	void operator()(const char* s, const char* e) const
	{
		string  str(s, e);
		strResult += str;
	}
};

void AddCustomCode(const CString& strFileName, const CDXFdata* pData)
{
	using namespace boost::spirit;

	CString	strBuf;
	string	strResult;
	CMakeCustomCode		custom1(strResult, pData);
	CMakeCustomCode2	custom2(strResult);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) ) {
			// �\�����
			strResult.clear();
			parse((LPCTSTR)strBuf,
				*( *(anychar_p - '{')[custom2] >> comment_p('{', '}')[custom1] )
			);
			AddMakeGdata( strResult.c_str() );
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// ���̴װ�͐�������(�x���̂�)
	}
}

BOOL IsNCchar(LPCTSTR lpsz)
{
	BOOL	bResult = TRUE;
	for ( int i=0; i<lstrlen(lpsz); i++ ) {
		if ( isprint(lpsz[i]) == 0 ) {
			bResult = FALSE;
			break;
		}
	}
	return bResult;
}

// ���H�J�n�w���ް��̐���
void AddMakeStart(void)
{
	CPointD	pt;
	// �@�B���_�ł�÷���ް�����
	AddStartTextIntegrated(pt);	// (0, 0)
	
	CDXFdata*	pData = NULL;
	CNCMake*	mkNCD;
	// ������������Ă΂�Ȃ��̂� IsMakeFlg() �̔��f�͕K�v�Ȃ�
	for ( int i=0; i<g_obStartData.GetSize() && IsThread(); i++ ) {
		pData = g_obStartData[i];
		// �J�n�ʒu�Ɠ�����÷���ް��̐���
		pt = pData->GetStartMakePoint();
		AddStartTextIntegrated(pt);
		// �ړ��w��
		mkNCD = new CNCMake(pData);
		ASSERT( mkNCD );
		g_obMakeGdata.Add(mkNCD);
	}
	if ( pData ) {
		// �I���ʒu�Ɠ�����÷���ް��̐���
		pt = pData->GetEndMakePoint();
		AddStartTextIntegrated(pt);
	}
}

// �ړ��w��ڲԂ�Z�����A����
void AddMoveZ_NotMove(void)
{
	// ���̂܂�
}

void AddMoveZ_R(void)
{
	// Z���̌��݈ʒu��R�_��菬����(�Ⴂ)�Ȃ�
	if ( CNCMake::ms_xyz[NCA_Z] < g_dZG0Stop )
		AddMoveGdata(0, g_dZG0Stop, -1);
}

void AddMoveZ_Initial(void)
{
	// Z���̌��݈ʒu���Ƽ�ٓ_��菬����(�Ⴂ)�Ȃ�
	if ( CNCMake::ms_xyz[NCA_Z] < g_dZInitial )
		AddMoveGdata(0, g_dZInitial, -1);
}

void AddMoveCust_B(void)
{
	AddMakeGdata(g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_B));
}

void AddMoveCust_A(void)
{
	AddMakeGdata(g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_A));
}

// ÷�ď��̐���
void AddCutterText_Target(CPointD& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFtext.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				CString	strText( ((CDXFtext *)pData)->GetStrValue() );
				strText.Replace("\\n", "\n");
				AddMakeGdata(strText);
				pData->SetMakeFlg();
				break;
			}
		}
	}
}

void AddStartText_Target(CPointD& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFstarttext.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				CString	strText( ((CDXFtext *)pData)->GetStrValue() );
				strText.Replace("\\n", "\n");
				AddMakeGdata(strText);
				pData->SetMakeFlg();
				break;
			}
		}
	}
}

void AddMoveText_Target(CPointD& pt)
{
	CDXFarray*	pobArray;

	if ( g_mpDXFmovetext.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				CString	strText( ((CDXFtext *)pData)->GetStrValue() );
				strText.Replace("\\n", "\n");
				AddMakeGdata(strText);
				pData->SetMakeFlg();
				return;
			}
		}
	}
}

void AddCommentText_Target(CPointD& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFcomment.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				AddMakeGdata( "(" + ((CDXFtext *)pData)->GetStrValue() + ")" );
				pData->SetMakeFlg();
				break;
			}
		}
	}
}

void AddText_Exclude(CPointD&)
{
	// �������Ȃ�
}

// ̪��ޏo��
void SendFaseMessage
	(int nRange/*=-1*/, LPCTSTR lpszMsg1/*=NULL*/, LPCTSTR lpszMsg2/*=NULL*/)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_Thread()", DBG_GREEN);
	dbg.printf("Phase%d Start", g_nFase);
#endif
	if ( nRange > 0 )
		g_pParent->m_ctReadProgress.SetRange32(0, nRange);

	CString	strMsg;
	if ( lpszMsg1 )
		strMsg = lpszMsg1;
	else
		strMsg.Format(IDS_MAKENCD_FASE, g_nFase++);
	g_pParent->SetFaseMessage(strMsg, lpszMsg2);
}

//////////////////////////////////////////////////////////////////////
// ���בւ��⏕�֐�
//////////////////////////////////////////////////////////////////////

int CircleSizeCompareFunc1(CDXFdata* pFirst, CDXFdata* pSecond)
{
	return (int)( (fabs(((CDXFcircle *)pFirst)->GetMakeR()) -
						fabs(((CDXFcircle *)pSecond)->GetMakeR())) * 1000.0 );
}

int CircleSizeCompareFunc2(CDXFdata* pFirst, CDXFdata* pSecond)
{
	return (int)( (fabs(((CDXFcircle *)pSecond)->GetMakeR()) -
						fabs(((CDXFcircle *)pFirst)->GetMakeR())) * 1000.0 );
}

int DrillOptimaizeCompareFuncX(CDXFdata* pFirst, CDXFdata* pSecond)
{
	// �l���傫���Ȃ�\��������
	int		nResult;
	double	dResult = pFirst->GetEndCutterPoint().x - pSecond->GetEndCutterPoint().x;
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int DrillOptimaizeCompareFuncY(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	double	dResult = pFirst->GetEndCutterPoint().y - pSecond->GetEndCutterPoint().y;
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

//////////////////////////////////////////////////////////////////////
// ��޽گ��
//////////////////////////////////////////////////////////////////////

// �e�گ�ނ̌����͈�
void SetSearchRange(void)
{
	int	i, nCnt = 0, nStep = max(1, g_obDXFdata.GetSize()/g_nProcesser);
	for ( i=0; i<g_nProcesser-1; i++ ) {
		g_lpmSearch[i].nOrder = nCnt;
		nCnt = min(nCnt+nStep, g_obDXFdata.GetSize());
		g_lpmSearch[i].nOrder2 = nCnt;
	}
	g_lpmSearch[i].nOrder = nCnt;
	nCnt = min(nCnt+nStep, g_obDXFdata.GetSize());
	g_lpmSearch[i].nOrder2 = max(nCnt, g_obDXFdata.GetSize());
}

// ���̐؍��޼ު�Ă̌���(from GetNearPointCutter)
UINT SearchObjectThread(LPVOID lpVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SearchObjectThread()\nStart", DBG_BLUE);
#endif
	int	i, s, e, n = (int)lpVoid;	// ��۰��ٕϐ��̂ǂ���g����
	CDXFdata*	pDataSrc;
	CDXFdata*	pData;
	double	dGap, dGapMin;

	while ( IsThread() ) {
		// ��ʂ̎��s���������܂ų���
		g_lpmSearch[n].evStart.Lock();
		g_lpmSearch[n].evStart.ResetEvent();
		// �p���׸�����
		if ( !g_lpmSearch[n].bThread )
			break;
		pDataSrc = g_lpSearchResult[n].pData;
		g_lpSearchResult[n].pData  = NULL;
		dGapMin = HUGE_VAL;
		s = g_lpmSearch[n].nOrder;
		e = g_lpmSearch[n].nOrder2;
		// �����J�n
#ifdef _DEBUG
		dbg.printf("ID=%d Start s=%d e=%d", n, s, e);
#endif
		// ���݈ʒu�Ɠ������C�܂��͋߂��v�f������
		// ���̽گ�ނœ��������W�����݂����Ƃ� g_lpmSearch[n].bResult ���U�Ō������f
		for ( i=s; i<e && g_lpmSearch[n].bResult && g_lpmSearch[n].bThread && IsThread(); i++ ) {
			pData = g_obDXFdata[i];
			if ( pData->IsMakeFlg() )
				continue;
			// �������f
//			if ( pData->IsMatchObject(pDataSrc) ) {
//				g_lpSearchResult[n].pData = pData;
//				g_lpSearchResult[n].bMatch = TRUE;
//				break;
//			}
			// ���݈ʒu�Ƃ̋����v�Z(���e�����͍��Wϯ�߂ɂ�)
			dGap = pData->GetEdgeGap(pDataSrc);
			if ( dGap < dGapMin ) {
				g_lpSearchResult[n].pData = pData;
//				if ( sqrt(dGap) < CDXFmap::ms_dTolerance ) {
//					g_lpSearchResult[n].bMatch = TRUE;
//					break;
//				}
				dGapMin = dGap;
			}
		}
		// �����I��
#ifdef _DEBUG
		dbg.printf("ID=%d End", n);
#endif
		g_lpSearchResult[n].dGap = dGapMin;
		g_lpmSearch[n].evEnd.SetEvent();
	}

	g_lpmSearch[n].evEnd.SetEvent();
#ifdef _DEBUG
	dbg.printf("ID=%d Thread End", n);
#endif
	return 0;
}

// CMap��޼ު�ĸر���ޯ���׳��޽گ��
UINT ClearPointMapThread(LPVOID lpVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("ClearPointMapThread()\nStart", DBG_BLUE);
#endif

	while ( IsThread() ) {
		// ��ʂ̎��s���������܂ų���
		g_pmClearMap.evStart.Lock();
		g_pmClearMap.evStart.ResetEvent();
		// �p���׸�����
		if ( !g_pmClearMap.bThread )
			break;
		// �֐����s
		g_mpEuler.RemoveAll();	// ���Ԃ�������ꍇ������
		// �I��
#ifdef _DEBUG
		dbg.printf("End");
#endif
		g_pmClearMap.evEnd.SetEvent();
	}

	g_pmClearMap.evEnd.SetEvent();
#ifdef _DEBUG
	dbg.printf("Thread End");
#endif
	return 0;
}

// NC�����̸�۰��ٕϐ�������(��n��)�گ��
UINT MakeNCDAfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

	int		i;
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCDAfterThread()\nStart", TRUE, DBG_RED);
#endif
	g_obDXFdata.RemoveAll();
	g_mpDXFdata.RemoveAll();
	g_mpDXFstarttext.RemoveAll();
	g_mpDXFmove.RemoveAll();
	g_mpDXFmovetext.RemoveAll();
	g_mpDXFtext.RemoveAll();
	g_mpDXFcomment.RemoveAll();
	for ( i=0; i<g_obMakeGdata.GetSize(); i++ )
		delete	g_obMakeGdata[i];
	g_obMakeGdata.RemoveAll();
	g_obStartData.RemoveAll();
	g_ltDeepGlist.RemoveAll();
	g_obPoint.RemoveAll();
	g_obCircle.RemoveAll();
	g_obDrillGroup.RemoveAll();
	g_obDrillAxis.RemoveAll();
	g_mpEuler.RemoveAll();

	g_csMakeAfter.Unlock();
	return 0;
}

// ��۾�����ɉ������گ�ނ̌Ăяo��(from CNCVCApp::InitInstance())
void SetMakeThreadFunction(void)
{
	// ��۾�����ɉ������e�ϐ��̈�̊m��
	ASSERT( g_nProcesser > 0 );
	g_lpmSearch			= new MAKENCDTHREADPARAM[g_nProcesser];
	g_lpSearchResult	= new SEARCHRESULT[g_nProcesser];
	g_phSearchEnd		= new HANDLE[g_nProcesser];
	for ( int i=0; i<g_nProcesser; i++ )
		g_phSearchEnd[i] = HANDLE(g_lpmSearch[i].evEnd);
}

void DestructThreadFunction(void)
{
	if ( g_lpmSearch )
		delete[]	g_lpmSearch;
	if ( g_lpSearchResult )
		delete[]	g_lpSearchResult;
	if ( g_phSearchEnd )
		delete[]	g_phSearchEnd;
}
