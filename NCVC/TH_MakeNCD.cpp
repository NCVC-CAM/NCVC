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

/*
!!!ATTENTION!!!
�������Ԃ̕\���F�����ذ��ł͊O���̂�Y�ꂸ��
#define	_DBG_NCMAKE_TIME
*/

using namespace boost;

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/*
	CDXFdata �� GetType() �� GetMakeType() �̎g�������ɒ��ӁI�I
*/
// �悭�g���ϐ���Ăяo���̊ȗ��u��
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
static	CDXFmap		g_mpDXFdata,	// ���W�𷰂�CDXFdata���i�[
			g_mpDXFstarttext,				// �J�nڲ�
			g_mpDXFmove, g_mpDXFmovetext,	// �ړ�ڲ�ϯ��
			g_mpDXFtext, g_mpDXFcomment;	// ���H÷�āC���Đ�p
static	CDXFarray	g_obPoint;		// �����H�ް�
static	CDXFsort	g_obCircle;		// �~�ް��������H����Ƃ��̉��o�^
static	CDXFsort	g_obDrillGroup;	// ��ٰ�ߕ������������H�ް�
static	CDXFsort	g_obDrillAxis;	// -> �����W�ŕ��בւ�
static	CDXFsort	g_obStartData;	// ���H�J�n�ʒu�w���ް�
static	CDXFlist	g_ltDeepGlist;	// �[���؍�p�̉��o�^
static	CTypedPtrArrayEx<CPtrArray, CNCMake*>	g_obMakeGdata;	// ���H�ް�

static	BOOL		g_bData;		// �e���������Ő������ꂽ��
static	double		g_dZCut;		// Z���̐؍���W == RoundUp(GetDbl(MKNC_DBL_ZCUT))
static	double		g_dDeep;		// �[���̐؍���W == RoundUp(GetDbl(MKNC_DBL_DEEP))
static	double		g_dZInitial;	// Z���̲Ƽ�ٓ_ == RoundUp(GetDbl(MKNC_DBL_G92Z))
static	double		g_dZG0Stop;		// Z����R�_ == RoundUp(GetDbl(MKNC_DBL_ZG0STOP))
static	double		g_dZReturn;		// Z���̕��A���W
static	int			g_nCorrect;		// �␳����

// ��ފ֐�
static	void	InitialVariable(void);		// �ϐ�������
static	void	InitialCycleBaseVariable(void);	// �Œ軲�ق��ް��l������
static	BOOL	SingleLayer(int);			// �P��ڲ�, �`�󏈗�
static	BOOL	MultiLayer(int);			// ����ڲԏ���(2����݌��p)
static	void	SetStaticOption(void);		// �ÓI�ϐ��̏�����
static	BOOL	SetStartData(void);			// ���H�J�n�ʒu�w��ڲԂ̉��o�^, �ړ��w��ڲԂ̏���
static	void	SetGlobalMap(void);
static	void	SetGlobalMapToLayer(const CLayerData*);
static	void	SetGlobalMapToLayer_Drill(CDXFdata*);
static	void	SetGlobalMapToOther(void);
static	BOOL	MakeNCD_MainFunc(const CLayerData*);		// NC������Ҳ�ٰ��
static	BOOL	MakeNCD_ShapeFunc(void);
static	BOOL	MakeNCD_FinalFunc(LPCTSTR = NULL);	// �I�����ށÇ�ُo�͂Ȃ�
static	BOOL	OutputNCcode(LPCTSTR);		// NC���ނ̏o��

// �e���ߕʂ̏����Ăяo��
static	enum	ENMAKETYPE {MAKECUTTER, MAKECORRECT, MAKEDRILLPOINT, MAKEDRILLCIRCLE};
static	BOOL	CallMakeDrill(const CLayerData*, CString&);
static	BOOL	CallMakeLoop(ENMAKETYPE, const CLayerData*, CString&);

// ���_����(TRUE:IsMatch)
static	tuple<CDXFdata*, BOOL>	OrgTuningCutter(const CLayerData*);
static	tuple<CDXFdata*, BOOL>	OrgTuningDrillPoint(void);
static	CDXFdata*	OrgTuningDrillCircle(void);

// �ް���͊֐�
static	BOOL		MakeLoopEuler(const CLayerData*, CDXFdata*);
static	BOOL		MakeLoopEulerSearch(const CPointD&, CDXFmap&);
static	int			MakeLoopEulerAdd(const CDXFmap*);
static	BOOL		MakeLoopEulerAdd_with_one_stroke(const CDXFmap*, BOOL, BOOL, const CPointD&, const CDXFarray*, CDXFlist&);
static	BOOL		MakeLoopShape(CDXFshape*);
static	int			MakeLoopShapeSearch(const CDXFshape*);
static	BOOL		MakeLoopShapeAdd(CDXFshape*, CDXFdata*);
static	BOOL		MakeLoopShapeAdd_ChainList(CDXFshape*, CDXFdata*);
static	BOOL		MakeLoopShapeAdd_EulerMap(CDXFshape*);
static	BOOL		MakeLoopShapeAdd_EulerMap_Make(CDXFshape*, CDXFmap*, BOOL&);
static	BOOL		MakeLoopShapeAdd_EulerMap_Search(const CPointD&, CDXFmap*, CDXFmap*);
static	BOOL		MakeLoopDeepAdd(void);
typedef	CDXFdata*	(*PFNDEEPPROC)(BOOL, BOOL);
static	PFNDEEPPROC	g_pfnDeepProc;
static	CDXFdata*	MakeLoopDeepAdd_Euler(BOOL, BOOL);
static	CDXFdata*	MakeLoopDeepAdd_All(BOOL, BOOL);
static	void		MakeLoopDeepZDown(void);
static	void		MakeLoopDeepZUp(void);
static	BOOL		MakeLoopDrillPoint(CDXFdata*);
static	BOOL		MakeLoopDrillPointSeqChk(BOOL, CDXFsort&);
static	BOOL		MakeLoopDrillPointXY(BOOL);
static	BOOL		MakeLoopDrillPointXYRevers(BOOL);
static	BOOL		MakeLoopDrillCircle(void);
static	BOOL		MakeLoopAddDrill(CDXFdata*);
static	BOOL		MakeLoopAddDrillSeq(int);
static	CDXFdata*	MakeLoopAddFirstMove(ENMAKETYPE);
static	BOOL		MakeLoopAddLastMove(void);

// �����Ŏw�肵����޼ު�ĂɈ�ԋ߂���޼ު�Ă�Ԃ�(�߂�l:Z���̈ړ����K�v��)
static	CDXFdata*	GetNearPointCutter(const CLayerData*, const CDXFdata*);
static	CDXFshape*	GetNearPointShape(const CPointD&);
static	tuple<CDXFdata*, BOOL>	GetNearPointDrill(CDXFdata*);
static	BOOL	GetMatchPointMove(CDXFdata*&);

// ͯ�ް,̯�ް���̽�߼�ٺ��ސ���
static	void	AddCustomCode(const CString&, const CDXFdata*);
static	BOOL	IsNCchar(LPCTSTR);

// ÷�ď��
static	void	AddCommentText(const CPointD&);
static	void	AddStartTextIntegrated(const CPointD& pt);
static	void	AddMoveTextIntegrated(const CPointD& pt);
static	void	AddCutterTextIntegrated(const CPointD& pt);

// NC�ް��o�^�֐�
static	void	AddMakeStart(void);		// ���H�J�n�w���ް��̐���
typedef void	(*PFNADDMOVE)(void);	// �ړ��w��ڲԂ̈ړ����ɂ����铮��֐�
static	PFNADDMOVE	g_pfnAddMoveZ, g_pfnAddMoveCust_B, g_pfnAddMoveCust_A;
static	void	AddMoveZ_NotMove(void);
static	void	AddMoveZ_R(void);
static	void	AddMoveZ_Initial(void);
static	void	AddMoveCust_B(void);
static	void	AddMoveCust_A(void);

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
	// �I�_���W�ł̺��Đ���
	AddCutterTextIntegrated( CDXFdata::ms_pData->GetEndMakePoint() );
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
	// �J�n�ʒu�Ɠ�����÷���ް��̐���
	AddCutterTextIntegrated( pData->GetStartMakePoint() );
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
	// �J�n�ʒu�Ɠ�����÷���ް��̐���
	AddCutterTextIntegrated( pData->GetStartMakePoint() );
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
	// �J�n�ʒu�Ɠ�����÷���ް��̐���
	AddMoveTextIntegrated( pData->GetStartMakePoint() );
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
// �������o��
inline	void	AddMakeText(CDXFdata* pData)
{
	CString	strText( ((CDXFtext *)pData)->GetStrValue() );
	strText.Replace("\\n", "\n");
	AddMakeGdata(strText);
	pData->SetMakeFlg();
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
static	CCriticalSection	g_csMakeAfter;	// MakeNCD_AfterThread()�گ��ۯ���޼ު��
static	UINT	MakeNCD_AfterThread(LPVOID);	// ��n���گ��

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

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		BOOL	bResult = FALSE;
		// NC��������
		int		nID = (int)(pParam->wParam);

		// NC������߼�ݵ�޼ު�Ă̐���
		g_pMakeOpt = new CNCMakeOption(NULL);	// �ǂݍ��݂͂��Ȃ�
		CNCMake::ms_pMakeOpt = g_pMakeOpt;

		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
#ifdef _DEBUG
		dbg.printf("g_csMakeAfter Unlock OK");
#endif

		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		{
			optional<CPointD>	ptResult = g_pDoc->GetCutterOrigin();
			CDXFdata::ms_ptOrg = ptResult ? *ptResult : 0.0;
		}
		g_pDoc->GetCircleObject()->OrgTuning(FALSE);	// ���ʓI�Ɍ��_����ۂɂȂ�
		InitialVariable();
		// �������蓖��
		i = g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER);
		g_obPoint.SetSize(0, i);
		g_obCircle.SetSize(0, i);
		i *= 2;
		g_obMakeGdata.SetSize(0, i);
		i = max(17, GetPrimeNumber(i));
		g_mpDXFdata.InitHashTable(i);
		g_mpDXFtext.InitHashTable(i);
		i = g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER);
		g_obStartData.SetSize(0, max(10, i*2));
		g_mpDXFstarttext.InitHashTable(max(17, GetPrimeNumber(i)));
		i = max(17, GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFMOVLAYER)*2));
		g_mpDXFmove.InitHashTable(i);
		g_mpDXFmovetext.InitHashTable(i);
		g_mpDXFcomment.InitHashTable(max(17, GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFCOMLAYER))));

		// Ҳ�ٰ�߂�
		switch ( nID ) {
		case ID_FILE_DXF2NCD:		// �P��ڲ�
		case ID_FILE_DXF2NCD_SHAPE:	// �`����H
			bResult = SingleLayer(nID);
			break;

		case ID_FILE_DXF2NCD_EX1:	// �����̐؍����̧��
		case ID_FILE_DXF2NCD_EX2:	// ����Z���W
			bResult = MultiLayer(nID);
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

	// �I������
	CDXFmap::ms_dTolerance = NCMIN;		// �K��l�ɖ߂�
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��
	// ��������NC���ނ̏����گ��(�D��x��������)
	AfxBeginThread(MakeNCD_AfterThread, NULL,
		/*THREAD_PRIORITY_LOWEST*/
		THREAD_PRIORITY_IDLE
		/*THREAD_PRIORITY_BELOW_NORMAL*/);

	// ������޼ު�č폜
	if ( g_pMakeOpt )
		delete	g_pMakeOpt;

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	g_bData = FALSE;
	g_nCorrect = -1;
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

BOOL SingleLayer(int nID)
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
	SetGlobalMap();

	// �����J�n
	if ( nID == ID_FILE_DXF2NCD ) {
		if ( !MakeNCD_MainFunc(NULL) ) {
#ifdef _DEBUG
			dbg.printf("Error:MakeNCD_MainFunc()");
#endif
			return FALSE;
		}
	}
	else {
		if ( !MakeNCD_ShapeFunc() ) {
#ifdef _DEBUG
			dbg.printf("Error:MakeNCD_ShapeFunc()");
#endif
			return FALSE;
		}
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

BOOL MultiLayer(int nID)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MultiLayer()\nStart", DBG_CYAN);
	CMagaDbg	dbgE("MultiLayer() Error", DBG_RED);
#endif
	extern	LPCTSTR	gg_szCat;
	int		i, j, nLayerCnt = g_pDoc->GetLayerCnt();
	BOOL	bPartOut = FALSE;	// �P��ł��ʏo�͂������TRUE
	CLayerData*	pLayer;

	if ( nID == ID_FILE_DXF2NCD_EX2 ) {
		// ��ƂȂ�NC������߼�ݓǂݍ���
		g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetInitList()->GetHead());
		// �������Ƃɕω��������Ұ���ݒ�
		SetStaticOption();
	}
	// ���H���_�J�n�ʒu�w��ڲԏ���
	if ( !SetStartData() )
		return FALSE;

	// �؍�ڲԈȊO��ϯ�ߍ쐬
	SetGlobalMapToOther();

	// ڲԖ���ٰ��
	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		// �؍�ڲԂőΏۂ�ڲԂ���
		if ( !pLayer->IsMakeTarget() )
			continue;
		pLayer->SetLayerFlags();
		// ����ڲԖ����޲�۸ނɕ\��
		g_nFase = 1;
		SendFaseMessage(-1, g_szWait, pLayer->GetStrLayer());
		//
		if ( nID == ID_FILE_DXF2NCD_EX1 ) {
			// NC������߼�ݓǂݍ���
			if ( pLayer->GetInitFile().CompareNoCase(g_pMakeOpt->GetInitFile()) != 0 ) {
				g_pMakeOpt->ReadMakeOption(pLayer->GetInitFile());	// �Ⴄ�Ƃ�����
				SetStaticOption();
			}
			// Z���̐؍���W���
			g_dZCut = RoundUp(GetDbl(MKNC_DBL_ZCUT));
			g_dDeep = RoundUp(GetDbl(MKNC_DBL_DEEP));
			// �Œ軲�ق̐؂荞�ݍ��W���
			CNCMake::ms_dCycleZ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		}
		else {
			// ����Z���W�␳
			g_dZCut = g_dDeep = RoundUp(pLayer->GetZCut());
			// �Œ軲�ق̐؂荞�ݍ��W���
			CNCMake::ms_dCycleZ[0] = pLayer->IsDrillZ() ?
						g_dZCut : RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		}
		// �Œ軲�ق��̑��̍��W���
		CNCMake::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
		CNCMake::ms_dCycleP[0] = GetNum(MKNC_NUM_DWELL);
		InitialCycleBaseVariable();
		// ��۰��ٕϐ��ɐ����Ώ۵�޼ު�Ă̺�߰
		g_mpDXFdata.RemoveAll();
		g_mpDXFtext.RemoveAll();
		g_obPoint.RemoveAll();
		g_obCircle.RemoveAll();
		g_ltDeepGlist.RemoveAll();
		g_obDrillGroup.RemoveAll();
		g_obDrillAxis.RemoveAll();
		SetGlobalMapToLayer(pLayer);
		// �ꎞ�̈�Ŏg�p����ϯ�߂��폜
		g_mpDXFdata.RemoveAll();

		// �����J�n
#ifdef _DEBUG
		dbg.printf("No.%d ID=%d Name=%s Cut=%f", i+1,
			pLayer->GetListNo(), pLayer->GetStrLayer(), g_dZCut);
#endif
		if ( !MakeNCD_MainFunc(pLayer) ) {
#ifdef _DEBUG
			dbgE.printf("MakeNCD_MainFunc() Error");
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
	g_pfnDeepProc = GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ?
		&MakeLoopDeepAdd_All : &MakeLoopDeepAdd_Euler;

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

	int		i, j;
	CDXFdata*	pData;
	CDXFdata*	pMatchData = NULL;
	CDXFdata*	pDataResult = NULL;
	CDXFcircleEx*	pStartCircle = NULL;
	double		dGap, dGapMin = HUGE_VAL;
	CString		strLayer;
	CLayerData*	pLayer;
	CDXFarray	obArray;
	obArray.SetSize(0, g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER));

	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutTarget() || pLayer->GetLayerType()!=DXFSTRLAYER )
			continue;
		// ���H�J�n�ʒu�w��ڲԂ�÷�ď�񌴓_����
		for ( j=0; j<pLayer->GetDxfTextSize() && IsThread(); j++ ) {
			pData = pLayer->GetDxfTextData(j);
			pData->OrgTuning(FALSE);
			g_mpDXFstarttext.SetMakePointMap(pData);
		}
		// ���H�J�n�ʒu�w��ڲԂ̌��_�����ƈړ��J�n�ʒu����ٰ��(OrgTuning)
		for ( j=0; j<pLayer->GetDxfSize() && IsThread(); j++ ) {
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

void SetGlobalMap(void)
{
	CLayerData*	pLayer;
	// �؍�Ώ�ڲ�(�\��ڲ�)���߰
	for ( int i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( pLayer->IsMakeTarget() )
			SetGlobalMapToLayer(pLayer);
	}
	// �ꎞ�̈�Ŏg�p����ϯ�߂��폜
	g_mpDXFdata.RemoveAll();

	// ���̑�ڲ��ް���ϯ�ߍ쐬
	SetGlobalMapToOther();
}

void SetGlobalMapToLayer(const CLayerData* pLayer)
{
	int		i;
	CDXFdata*	pData;

	// �w��ڲԂ�DXF�ް���޼ު��(�����H�̂�)���߰
	for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++ )
		SetGlobalMapToLayer_Drill(pLayer->GetDxfData(i));

	// ÷���ް���ϯ�ߍ쐬
	for ( i=0; i<pLayer->GetDxfTextSize() && IsThread(); i++ ) {
		pData = pLayer->GetDxfTextData(i);
		pData->OrgTuning(FALSE);
		g_mpDXFtext.SetMakePointMap(pData);
	}
}

void SetGlobalMapToLayer_Drill(CDXFdata* pData)
{
	// �ȉ~�ް��̕ϐg(���a�Z�a���������ȉ~�Ȃ�)
	if ( GetFlg(MKNC_FLG_ELLIPSE) && pData->GetType()==DXFELLIPSEDATA ) {
		CDXFellipse* pEllipse = (CDXFellipse *)pData;
		if ( fabs( RoundUp(pEllipse->GetLongLength()) - RoundUp(pEllipse->GetShortLength()) ) < EPS )
			pData->ChangeMakeType( ((CDXFellipse *)pData)->GetArc() ?
				DXFARCDATA : DXFCIRCLEDATA);	// �~�ʂ��~�ް��ɕϐg
	}

	CDXFarray*	pDummy = NULL;
	CPointD		pt;
	// �e��޼ު�����߂��Ƃ̏���
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		// �d������
		if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
			pt = pData->GetNativePoint(0);
			if ( !g_mpDXFdata.Lookup(pt, pDummy) ) {
				g_obPoint.Add(pData);
				g_mpDXFdata.SetAt(pt, pDummy);
			}
		}
		else
			g_obPoint.Add(pData);
		break;
	case DXFCIRCLEDATA:
		// �~�ް��̔��a���w��l�ȉ��Ȃ�
		if ( GetFlg(MKNC_FLG_DRILLCIRCLE) &&
				((CDXFcircle *)pData)->GetMakeR() <= GetDbl(MKNC_DBL_DRILLCIRCLE) ) {
			// �����H�ް��ɕϐg���ēo�^
			pData->ChangeMakeType(DXFPOINTDATA);
			// �����ł͏d���������s��Ȃ��B
			// ���בւ�����(OrgTuningDrillCircle)�ɍs��
			g_obCircle.Add(pData);
			break;
		}
		break;
	}
}

void SetGlobalMapToOther(void)
{
	// �������猴�_���������޼ު�Ă͋����v�Z�̕K�v�Ȃ�
	int			i, j, nType;
	CLayerData*	pLayer;
	CDXFdata*	pData;

	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutTarget() )
			continue;
		nType = pLayer->GetLayerType();
		if ( nType == DXFMOVLAYER ) {
			// �ړ��w��ڲԂ�ϯ�ߐ���
			for ( j=0; j<pLayer->GetDxfSize() && IsThread(); j++ ) {
				pData = pLayer->GetDxfData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFmove.SetMakePointMap(pData);
			}
			// �ړ��w��ڲ�÷�Ă�ϯ�ߐ���
			for ( j=0; j<pLayer->GetDxfTextSize() && IsThread(); j++ ) {
				pData = pLayer->GetDxfTextData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFmovetext.SetMakePointMap(pData);
			}
		}
		else if ( nType == DXFCOMLAYER ) {
			// ����ڲԂ�ϯ�ߐ���
			for ( j=0; j<pLayer->GetDxfTextSize() && IsThread(); j++ ) {
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

BOOL MakeNCD_MainFunc(const CLayerData* pLayer)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_MainFunc()\nStart", DBG_MAGENTA);
#endif
	CString	strLayer;
	if ( pLayer )
		strLayer = pLayer->GetStrLayer();

	// ���H�J�n�ʒu�w���ް��̾��
	if ( !g_bData && !g_obStartData.IsEmpty() )
		CDXFdata::ms_pData = g_obStartData.GetTail();

	// Ҳݕ���
	switch ( GetNum(MKNC_NUM_DRILLPROCESS) ) {
	case 0:		// ��Ɍ����H
		// �����H�ް��̏����ƍŒZ�J�n�ʒu���ް����
		if ( !CallMakeDrill(pLayer, strLayer) )
			return FALSE;
		// �؍��ް��̏����ƍŒZ�J�n�ʒu���ް����
		// �����H�ް��̍Ō�ɋ߂��ް�����
		if ( !CallMakeLoop(MAKECUTTER, pLayer, strLayer) )
			return FALSE;
		break;

	case 1:		// ��Ō����H
		// �؍��ް��̏����ƍŒZ�J�n�ʒu���ް����
		if ( !CallMakeLoop(MAKECUTTER, pLayer, strLayer) )
			return FALSE;
		// through
	case 2:		// �����H�̂�
		// �����H�ް��̏����ƍŒZ�J�n�ʒu���ް����
		// �؍��ް��̍Ō�ɋ߂��ް�����
		if ( !CallMakeDrill(pLayer, strLayer) )
			return FALSE;
		break;

	default:
		return FALSE;
	}

	// �؍��ް��I����̈ړ��w��ڲ�����
	return MakeLoopAddLastMove();
}

BOOL MakeNCD_ShapeFunc(void)
{
	CDXFshape*	pShape;
	CString		strBuf;
	int			nMapCnt = 0;

	// ̪���1
	// ���_������ϯ�߂̐����׸ނ�ر
	for ( int i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ )
		nMapCnt += g_pDoc->GetLayerData(i)->AllShape_OrgTuning();
	if ( !IsThread() )
		return FALSE;

	// G����ͯ��(�J�n����)
	AddCustomCode(g_pMakeOpt->GetStr(MKNC_STR_HEADER), NULL);
	// ���H�J�n�ʒu�w��ڲԏ���
	AddMakeStart();
	if ( !g_obStartData.IsEmpty() )
		CDXFdata::ms_pData = g_obStartData.GetTail();
	// ��]��
	strBuf = CNCMake::MakeSpindle(DXFLINEDATA);
	if ( !strBuf.IsEmpty() )
		AddMakeGdata(strBuf);
	// ���H�O�ړ��w���̐���
	MakeLoopAddFirstMove(MAKECUTTER);

	// ���_�ɋ߂����Wϯ�߂�����
	pShape = GetNearPointShape(CDXFdata::ms_pData->GetEndCutterPoint()+CDXFdata::ms_ptOrg);
	if ( !pShape )
		return TRUE;

	// ̪���2
	SendFaseMessage(nMapCnt);

	// NC����ٰ��
	return MakeLoopShape(pShape);
}

//////////////////////////////////////////////////////////////////////
// NC����Ҳݽگ�ޕ⏕�֐��Q
//////////////////////////////////////////////////////////////////////

BOOL CallMakeDrill(const CLayerData* pLayer, CString& strLayer)
{
	// �����H��������
	if ( GetFlg(MKNC_FLG_DRILLCIRCLE) ) {
		// �~�ް��������H�ް��Ƃ���ꍇ
		switch ( GetNum(MKNC_NUM_DRILLCIRCLEPROCESS) ) {
		case 1:	// �~����
			if ( !CallMakeLoop(MAKEDRILLCIRCLE, pLayer, strLayer) )
				return FALSE;
			if ( !CallMakeLoop(MAKEDRILLPOINT, pLayer, strLayer) )
				return FALSE;
			break;

		case 2:	// �~�����
			if ( !CallMakeLoop(MAKEDRILLPOINT, pLayer, strLayer) )
				return FALSE;
			// through
		case 0:	// ���_����
			if ( !CallMakeLoop(MAKEDRILLCIRCLE, pLayer, strLayer) )
				return FALSE;
			break;
		}
	}
	else {
		// ���_�ް����������H
		if ( !CallMakeLoop(MAKEDRILLPOINT, pLayer, strLayer) )
			return FALSE;
	}

	// �Œ軲�ٷ�ݾ�
	AddMakeGdataCycleCancel();

	return TRUE;
}

BOOL CallMakeLoop(ENMAKETYPE enMake, const CLayerData* pLayer, CString& strLayer)
{
	CDXFdata*	pData;
	BOOL		bMatch;
	CString		strBuf;

	// ̪���1 ( OrgTuning_XXX)
	switch( enMake ) {
	case MAKECUTTER:
		tie(pData, bMatch) = OrgTuningCutter(pLayer);
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

	// ���H�O�ړ��w���̐���
	if ( !bMatch && enMake!=MAKEDRILLCIRCLE ) {
		// OrgTuning()�Ō��݈ʒu�Ɠ�����W��������Ȃ�����
		CDXFdata* pDataMove = MakeLoopAddFirstMove(enMake);
		if ( pDataMove ) {
			// �ړ���ɋ߂��؍��ް����Č���
			if ( enMake == MAKECUTTER )
				pData = GetNearPointCutter(pLayer, pDataMove);
			else
				tie(pData, bMatch) = GetNearPointDrill(pDataMove);
		}
	}

	// ̪���2
	SendFaseMessage();

	// NC����ٰ��
	BOOL	bResult = FALSE;
	switch ( enMake ) {
	case MAKECUTTER:
		bResult = MakeLoopEuler(pLayer, pData);
		break;
	case MAKEDRILLPOINT:
		bResult = MakeLoopDrillPoint(pData);
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

tuple<CDXFdata*, BOOL> OrgTuningCutter(const CLayerData* pLayerTarget)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningCutter()", DBG_GREEN);
	int			nDbg = -1;
	CLayerData*	pLayerDbg = NULL;
#endif
	int		i, nCnt = 0, nLayerLoop, nDataLoop;
	BOOL	bMatch = FALSE, bCalc = !g_bData;
	double	dGap, dGapMin = HUGE_VAL;	// ���_�܂ł̋���
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	CLayerData*	pLayer;

	// ̪���1(������������)
	if ( pLayerTarget ) {
		nLayerLoop = 1;
		nDataLoop  = pLayerTarget->GetDxfSize();
	}
	else {
		nLayerLoop = g_pDoc->GetLayerCnt();
		nDataLoop  = 0;
		for ( i=0; i<nLayerLoop; i++ ) {
			pLayer = g_pDoc->GetLayerData(i);
			if ( pLayer->IsMakeTarget() )
				nDataLoop += pLayer->GetDxfSize();
		}
	}
	SendFaseMessage(nDataLoop);

	// ���_�����Ɛ؍�J�n�߲�Č���ٰ��
	while ( nLayerLoop-- > 0 && IsThread() ) {
		pLayer = pLayerTarget ? (CLayerData *)pLayerTarget : g_pDoc->GetLayerData(nLayerLoop);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++, nCnt++ ) {
			pData = pLayer->GetDxfData(i);
			if ( pData->GetMakeType() != DXFPOINTDATA ) {
				// ���_�����Ƌ����v�Z + NC�����׸ނ̏�����
				dGap = pData->OrgTuning(bCalc);
				if ( !bCalc )
					dGap = pData->GetEdgeGap(CDXFdata::ms_pData);
				// ���_(�܂��͌��݈ʒu)�Ɉ�ԋ߂��߲�Ă�T��
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
#ifdef _DEBUG
					nDbg = i;
					pLayerDbg = pLayer;
#endif
					if ( sqrt(dGap) < CDXFmap::ms_dTolerance )
						bMatch = TRUE;
				}
				// ���Wϯ�߂ɓo�^
				g_mpDXFdata.SetMakePointMap(pData);
			}
			SendProgressPos(nCnt);
		}
	}

#ifdef _DEBUG
	if ( pLayerDbg ) {
		dbg.printf("FirstPoint Layer=%s Cnt=%d Gap=%f",
			pLayerDbg->GetStrLayer(), nDbg, dGapMin);
	}
	else {
		dbg.printf("FirstPoint Cnt=%d Gap=%f",
			nDbg, dGapMin);
	}
#endif
	g_pParent->m_ctReadProgress.SetPos(nDataLoop);

	return make_tuple(pDataResult, bMatch);
}

tuple<CDXFdata*, BOOL> OrgTuningDrillPoint(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillPoint()", DBG_GREEN);
#endif
	CDXFdata*	pDataResult = NULL;
	if ( g_obPoint.IsEmpty() )
		return make_tuple(pDataResult, FALSE);

	int		i, nLoop = g_obPoint.GetSize();
	BOOL	bMatch = FALSE, bCalc;
	double	dGap, dGapMin = HUGE_VAL;	// ���_�܂ł̋���
	CDXFdata*	pData;

	// ̪���1
	SendFaseMessage(nLoop);

	// �œK���ɔ����z��̺�߰��
	CDXFsort& obDrill = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? g_obDrillAxis : g_obDrillGroup;
	obDrill.SetSize(0, nLoop);

	// ���_�����Ɛ؍�J�n�߲�Č���ٰ��
#ifdef _DEBUG
	int	nDbg = -1;
#endif

	// �؍�J�n�߲�Ă̌v�Z���K�v���ǂ���
	if ( g_bData )
		bCalc = FALSE;
	else
		bCalc = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? TRUE : FALSE;

	// �����H��޼ު�Ă� g_obDrillGroup �ɺ�߰
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = g_obPoint[i];
		// ���_�����Ƌ����v�Z + NC�����׸ނ̏�����
		dGap = pData->OrgTuning(bCalc);
		// ���_�Ɉ�ԋ߂��߲�Ă�T��
		if ( !pData->IsMakeFlg() ) {
			obDrill.Add(pData);
			if ( !bCalc )
				dGap = pData->GetEdgeGap(CDXFdata::ms_pData);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pDataResult = pData;
#ifdef _DEBUG
				nDbg = i;
#endif
				if ( sqrt(dGap) < GetDbl(MKNC_DBL_TOLERANCE) )
					bMatch = TRUE;
			}
		}
		SendProgressPos(i);
	}
#ifdef _DEBUG
	dbg.printf("FirstPoint %d Gap=%f", nDbg, dGapMin);
#endif

	g_pParent->m_ctReadProgress.SetPos(nLoop);

	return make_tuple(pDataResult, bMatch);
}

CDXFdata* OrgTuningDrillCircle(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillCircle()", DBG_GREEN);
#endif
	if ( g_obCircle.IsEmpty() )
		return NULL;

	int		i, j, nLoop = g_obCircle.GetSize();
	CPointD	pt;
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	SendFaseMessage(nLoop);

	// ̪���1 ���_����
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		g_obCircle[i]->OrgTuning(FALSE);	// ���בւ���̂ŋߐڌv�Z�͕s�v
		SendProgressPos(i);
	}
	g_pParent->m_ctReadProgress.SetPos(nLoop);
	// ̪���2 ���בւ�
	if ( GetNum(MKNC_NUM_DRILLSORT) == 0 )
		g_obCircle.Sort(CircleSizeCompareFunc1);	// ����
	else
		g_obCircle.Sort(CircleSizeCompareFunc2);	// �~��

	if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
		// ̪���3 �d������
		SendFaseMessage(nLoop);
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pData1 = g_obCircle[i];
			if ( !pData1->IsMakeFlg() ) {
				pt = pData1->GetStartMakePoint();
				for ( j=i+1; j<nLoop && IsThread(); j++ ) {
					pData2 = g_obCircle[j];
					if ( pt == pData2->GetStartMakePoint() ) 
						pData2->SetMakeFlg();
				}
			}
			SendProgressPos(i);
		}
		g_pParent->m_ctReadProgress.SetPos(nLoop);
	}

	// ��а�ް���Ԃ�
	return g_obCircle[0];
}

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopEuler(const CLayerData* pLayer, CDXFdata* pData)
{
	// ---------------------------------------------------
	// MakeLoopEuler() �̏����S�ʂ́C���x�̍������W��n��
	// ---------------------------------------------------
	CDXFdata*	pDataMove;
	CDXFmap		mpEuler;		// ��M������޼ު�Ă��i�[
	BOOL		bMatch, bMove, bCust;
	int			i, nCnt, nPos = 0, nSetPos = 64;
	CPointD		pt;

	i = GetPrimeNumber( g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER)*2 );
	mpEuler.InitHashTable(max(17, i));

	// GetNearPoint() �̌��ʂ� NULL �ɂȂ�܂�
	while ( pData && IsThread() ) {
		mpEuler.RemoveAll();
		// ���� pData ����_��
		mpEuler.SetMakePointMap(pData);
		pData->SetSearchFlg();
		// pData �̎n�_�E�I�_�ň�M�����T��
		for ( i=0; i<pData->GetPointNumber(); i++ ) {
			pt = pData->GetTunPoint(i);
			if ( !MakeLoopEulerSearch(pt, mpEuler) )
				return FALSE;
		}
		// CMap��޼ު�Ă�NC����
		if ( (nCnt=MakeLoopEulerAdd(&mpEuler)) < 0 )
			return FALSE;
		//
		nPos += nCnt;
		if ( nSetPos < nPos ) {
			g_pParent->m_ctReadProgress.SetPos(nPos);
			while ( nSetPos < nPos )
				nSetPos += nSetPos;
		}

		// �ړ��w��ڲԂ�����
		bCust = TRUE;
		bMove = FALSE;
		pDataMove = CDXFdata::ms_pData;
		bMatch = GetMatchPointMove(pDataMove);
		while ( bMatch && IsThread() ) {
			if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPAPROCESS)==0 ) {
				// �Ō��Z���ړ���ϰ��������΍폜
				if ( !g_ltDeepGlist.IsEmpty() && g_ltDeepGlist.GetTail() == NULL )
					g_ltDeepGlist.RemoveTail();
				// �ړ��ް��o�^
				g_ltDeepGlist.AddTail(pDataMove);
				pDataMove->SetMakeFlg();
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
				AddMakeMove(pDataMove);
			}
			// �ړ��ް���Ҕ�
			CDXFdata::ms_pData = pDataMove;
			// ���̈ړ��ް�����
			bMatch = GetMatchPointMove(pDataMove);
		}
		// �ړ��ް���̶��Ѻ��ޑ}��
		if ( bMove && IsThread() ) {
			// �ړ��ް��̏I�_��÷���ް��̐���
			pt = pDataMove->GetEndMakePoint();
			AddMoveTextIntegrated(pt);
			// ���Ѻ���
			(*g_pfnAddMoveCust_A)();
		}

		// ���̐؍��߲�Č���
		pData = GetNearPointCutter(pLayer, pDataMove);

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

BOOL MakeLoopEulerSearch(const CPointD& ptKey, CDXFmap& mpEuler)
{
	int			i, j;
	CPointD		pt;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// ���W�𷰂ɑS�̂�ϯ�ߌ���
	if ( g_mpDXFdata.Lookup(const_cast<CPointD&>(ptKey), pobArray) ) {
		for ( i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() && !pData->IsSearchFlg() ) {
				mpEuler.SetMakePointMap(pData);
				pData->SetSearchFlg();
				// ��޼ު�Ă̒[�_������Ɍ���
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					pt = pData->GetTunPoint(j);
					if ( ptKey != pt ) {
						if ( !MakeLoopEulerSearch(pt, mpEuler) )
							break;
					}
				}
			}
		}
	}

	return IsThread();
}

int MakeLoopEulerAdd(const CDXFmap* pEuler)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopEulerAdd()", DBG_MAGENTA);
#endif
	BOOL		bEuler = FALSE;		// ��M�����v���𖞂����Ă��邩
	POSITION	pos;
	CPointD		pt, ptNow(CDXFdata::ms_pData->GetEndCutterPoint());
	CDXFdata*	pData;
	CDXFarray*	pStartArray;
	CDXFlist	ltEuler;			// ��M������

#ifdef _DEBUGOLD
	CDumpContext	dc;
	dc.SetDepth(1);
	pEuler->Dump(dc);
#endif

	// ���̍��Wϯ�߂���M�����v���𖞂����Ă��邩
	tie(bEuler, pStartArray, pt) = pEuler->IsEulerRequirement(ptNow);
	if ( !IsThread() )
		return -1;
#ifdef _DEBUG
	dbg.printf("FirstPoint x=%f y=%f Euler=%d", pt.x, pt.y, bEuler);
#endif

	// --- ��M�����̐���(�ċA�Ăяo���ɂ��؍\�����)
	ASSERT( pStartArray );
	ASSERT( !pStartArray->IsEmpty() );
	if ( !MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, FALSE, pt, pStartArray, ltEuler) ) {
		// ��M�����ł���n�Y�₯�ǎ��s����������ɘa���Ă�蒼��
		bEuler = FALSE;
		MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, FALSE, pt, pStartArray, ltEuler);
	}

	// --- �؍��ް�����
	ASSERT( !ltEuler.IsEmpty() );
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// �؍��ް�����
		for ( pos=ltEuler.GetHeadPosition(); pos && IsThread(); ) {
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
		for ( pos=ltEuler.GetHeadPosition(); pos && IsThread(); ) {
			pData = ltEuler.GetNext(pos);
			AddMakeGdata(pData);
		}
		// �Ō�ɐ��������ް���Ҕ�
		CDXFdata::ms_pData = pData;
	}

	// ��M�����v���𖞂����Ă��Ȃ��Ƃ�����
	if ( !bEuler )
		pEuler->AllMapObject_ClearSearchFlg();	// ��M�����ɘR�ꂽ��޼ު�Ă̻���׸ނ�������

	return ltEuler.GetCount();
}

BOOL MakeLoopEulerAdd_with_one_stroke
	(const CDXFmap* pEuler, BOOL bEuler, BOOL bMakeShape, 
		const CPointD& pt, const CDXFarray* pArray, CDXFlist& ltEuler)
{
	int			i, nLoop = pArray->GetSize();
	CDXFdata*	pData;
	CDXFarray*	pNextArray;
	CPointD		ptNext;
	POSITION	pos, posTail = ltEuler.GetTailPosition();	// ���̎��_�ł̉��o�^ؽĂ̍Ō�

	// �܂����̍��W�z��̉~(�ɏ�������)�ް������o�^
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() && pData->IsStartEqEnd() ) {
			pData->GetEdgeGap(pt);	// pt�l�ɋ߂������޼ު�Ă̎n�_�ɓ���ւ�
			ltEuler.AddTail( pData );
			pData->SetSearchFlg();
		}
	}

	// �~�ȊO���ް��Ŗ؍\���̎�������
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() ) {
			pData->GetEdgeGap(pt);
			ltEuler.AddTail(pData);
			pData->SetSearchFlg();
			ptNext = pData->GetEndCutterPoint();
			if ( bMakeShape ) {
				if ( !pEuler->Lookup(ptNext+CDXFdata::ms_ptOrg, pNextArray) )
					NCVC_CriticalErrorMsg(__FILE__, __LINE__);
			}
			else {
				if ( !pEuler->Lookup(ptNext, pNextArray) )
					NCVC_CriticalErrorMsg(__FILE__, __LINE__);
			}
			// ���̍��W�z�������
			if ( MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, bMakeShape, ptNext, pNextArray, ltEuler) )
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
		if ( pEuler->IsAllSearchFlg() )
			return TRUE;	// �S���I��
	}
	// ���̍��W�z��̌������I�������̂Ŗ؍\���̏�ʂֈړ��D
	// �~�ް����܂ޑS�Ẳ��o�^ؽĂ��폜
	if ( posTail ) {
		ltEuler.GetNext(posTail);	// posTail�̎�����
		for ( pos=posTail; pos && IsThread(); pos=posTail) {
			pData = ltEuler.GetNext(posTail);	// ��Ɏ��̗v�f���擾
			pData->ClearSearchFlg();			// �׸ނ�������
			ltEuler.RemoveAt(pos);				// �v�f�폜
		}
	}

	return FALSE;
}

BOOL MakeLoopShape(CDXFshape* pShape)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopShape()", DBG_RED);
#endif
	CDXFdata*	pData;
	const	CPointD		ptOrg(CDXFdata::ms_ptOrg);
			CPointD		pt;
	int		nCnt, nPos = 0;

	while ( pShape && IsThread() ) {
#ifdef _DEBUG
		dbg.printf("ParentMapName=%s", pShape->GetShapeName());
#endif
		// ϯ�߂̓������琶��
		pShape->SetShapeFlag(DXFMAPFLG_SEARCH);	// �eϯ�߂͌����ΏۊO
		if ( (nCnt=MakeLoopShapeSearch(pShape)) < 0 )
			return FALSE;
		// �eϯ�ߎ��g�̐���
		pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		pShape->GetSelectObjectFromShape(pt, NULL, &pData);	// �߂�l�s�v
		if ( !MakeLoopShapeAdd(pShape, pData) )
			return FALSE;
		// ��۸�ڽ�ް�̍X�V
		nPos += nCnt+1;
		g_pParent->m_ctReadProgress.SetPos(nPos);
		// ����ϯ�߂�����
		pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		pShape = GetNearPointShape(pt);		// Ȳè�ލ��W�Ō���
	}

	// �S�̐[���̌㏈��
	if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPAPROCESS)==0 ) {
		if ( !MakeLoopDeepAdd() )
			return FALSE;
	}

	return IsThread();
}

int MakeLoopShapeSearch(const CDXFshape* pShapeBase)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopShapeSearch()", DBG_RED);
#endif

	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CShapeArray	obShape;	// �����̍��Wϯ�߈ꗗ
	CRectD	rcBase( pShapeBase->GetMaxRect() );
	int		i, j, nCnt = 0;
	obShape.SetSize(0, 64);

	// pShapeBase�������̋�`�������Wϯ�߂�����
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		for ( j=0; j<pLayer->GetShapeSize(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			if ( !pShape->IsMakeFlg() && !pShape->IsSearchFlg() &&
					pShape->GetShapeAssemble()!=DXFSHAPE_EXCLUDE &&
					rcBase.PtInRect(pShape->GetMaxRect()) ) {
				pShape->SetShapeFlag(DXFMAPFLG_SEARCH);
				obShape.Add(pShape);
				nCnt += MakeLoopShapeSearch(pShape);
			}
		}
	}

	if ( obShape.IsEmpty() || !IsThread() )
		return 0;

	// obShape�ɒ~�ς��ꂽ�ް��̐���
	const	CPointD		ptOrg(CDXFdata::ms_ptOrg);
			CPointD		pt(CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg);
	double		dGap, dGapMin;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	CDXFshape*	pShapeResult;

	pShapeResult = obShape[0];
	j = obShape.GetSize();

	while ( pShapeResult && IsThread() ) {
		dGapMin = HUGE_VAL;
		pShapeResult = NULL;
		// pt�Ɉ�ԋ߂�Ȳè�ނ̍��Wϯ�߂�����
		for ( i=0; i<j && IsThread(); i++ ) {
			pShape = obShape[i];
			if ( !pShape->IsMakeFlg() ) {
				dGap = pShape->GetSelectObjectFromShape(pt, NULL, &pData);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
					pShapeResult = pShape;
				}
			}
		}
		if ( pShapeResult ) {
			// ���Wϯ�߂�NC����
			if ( !MakeLoopShapeAdd(pShapeResult, pDataResult) )
				return -1;
			// pt���W�̍X�V
			pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		}
	}

	return nCnt;
}

BOOL MakeLoopShapeAdd(CDXFshape* pShape, CDXFdata* pData)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopShapeAdd()", DBG_RED);
	dbg.printf("MapName=%s", pShape->GetShapeName());
#endif
	BOOL	bResult;

	bResult = pShape->GetShapeType() == 0 ?
		MakeLoopShapeAdd_ChainList( pShape, pData ) :
		MakeLoopShapeAdd_EulerMap ( pShape );

	pShape->SetShapeFlag(DXFMAPFLG_MAKE);

	return bResult;
}

BOOL MakeLoopShapeAdd_ChainList(CDXFshape* pShape, CDXFdata* pData)
{
	const	CPointD		ptOrg( CDXFdata::ms_ptOrg );
	const	CPointD		ptNow( CDXFdata::ms_pData->GetEndCutterPoint() );
	double	dGap1, dGap2;
	BOOL	bReverse = FALSE, bNext = FALSE;
	CDXFworking*	pWork;
	CDXFdata*		pDataFix;
	CDXFchain*		pChain = pShape->GetOutlineObject();

	if ( pChain ) {
		// �֊s��޼ު�ėp���_����
		pChain->OrgTuning();
		// �֊s��޼ު�Ăŋ߂��ް�������
		pChain->GetSelectObjectFromShape(ptNow+ptOrg, NULL, &pData);
	}
	else {
		pChain = pShape->GetShapeChain();	// �ؼ��ٵ�޼ު��
		// �ߐڵ�޼ު��(pData)�́A��ʂŌ����ς�
	}
	ASSERT( pData );

	// �����w������ѐ�����������
	tie(pWork, pDataFix) = pShape->GetDirectionObject();
	if ( pChain->GetCount()==1 && pData->GetMakeType()==DXFCIRCLEDATA ) {
		if ( pDataFix ) {
			CPointD	pts( ((CDXFworkingDirection *)pWork)->GetStartPoint() ),
					pte( ((CDXFworkingDirection *)pWork)->GetArrowPoint() );
			// �ؼ��ٵ�޼ު�Ăŉ�]�ݒ�
			CDXFcircle*	pDataOrig = (CDXFcircle *)(pShape->GetShapeChain()->GetHead());
			pDataOrig->SetRoundFixed(pts, pte);
			// �֊s��޼ު�Ă�����΁A������]�ݒ�
			if ( pShape->GetOutlineObject() )
				((CDXFcircle *)pData)->SetRoundFixed(pDataOrig->GetRound());
		}
		pData->GetEdgeGap(ptNow);	// �֊s��޼ު�Ă̋ߐڍ��W�v�Z
	}
	else {
		dGap1 = GAPCALC(pData->GetStartCutterPoint() - ptNow);
		dGap2 = GAPCALC(pData->GetEndCutterPoint()   - ptNow);
		if ( pDataFix ) {
			CPointD	ptFix( ((CDXFworkingDirection *)pWork)->GetArrowPoint() - ptOrg );
			if ( GAPCALC(pDataFix->GetEndCutterPoint() - ptFix) > EPS ) {
				bReverse = TRUE;
				// �J�n��޼ު�Ă̏I�_(bReverse�Ȃ̂Ŏn�_�Ŕ��f)
				// �̕����߂��ꍇ�́A���̵�޼ު�Ă���J�n
				if ( dGap1 < dGap2 )
					bNext = TRUE;
			}
			else {
				if ( dGap1 > dGap2 )
					bNext = TRUE;
			}
		}
		else {
			if ( dGap1 > dGap2 )
				bReverse = TRUE;
		}
		if ( bReverse )
			pChain->ReversPoint();
	}

	// �J�n��޼ު�Ă̌���
	POSITION	pos1, pos2,
				(CDXFchain::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFchain::*pfnGetData)(POSITION&);
	for ( pos1=pChain->GetHeadPosition(); (pos2=pos1) && IsThread(); ) {
		if ( pData == pChain->GetNext(pos1) )
			break;
	}
	pos1 = pos2;

	if ( bReverse ) {
		pfnGetPosition	= &(CDXFchain::GetTailPosition);
		pfnGetData		= &(CDXFchain::GetPrev);
	}
	else {
		pfnGetPosition	= &(CDXFchain::GetHeadPosition);
		pfnGetData		= &(CDXFchain::GetNext);
	}

	// �J�n��޼ު�Ă̍ŏI����
	if ( bNext ) {
		for ( int i=0; i<2; i++ ) {
			pos2 = pos1;
			pData = (pChain->*pfnGetData)(pos1);
			if ( !pos1 )
				pos1 = (pChain->*pfnGetPosition)();
		}
		pos1 = pos2;
	}

	// �؍��ް�����
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// �J�n�߼޼�݂���ٰ��
		while ( pos1 && IsThread() ) {
			pData = (pChain->*pfnGetData)(pos1);
			g_ltDeepGlist.AddTail(pData);
		}
		// [�擪|�I�[]����J�n��޼ު�Ă܂�
		for ( pos1=(pChain->*pfnGetPosition)(); pos1!=pos2 && IsThread(); ) {
			pData = (pChain->*pfnGetData)(pos1);
			g_ltDeepGlist.AddTail(pData);
		}
		// �[�����S�̂��ۂ�
		if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
			// �Ō�ɐ��������ް���Ҕ�
			CDXFdata::ms_pData = pData;
			// Z���ړ���ϰ��
			g_ltDeepGlist.AddTail((CDXFdata *)NULL);
		}
		else
			return MakeLoopDeepAdd();
	}
	else {
		// �؍��ް��܂ł̈ړ�
		if ( GetNum(MKNC_NUM_TOLERANCE) == 0 )
			AddMoveGdataG0(pData);
		else
			AddMoveGdataG1(pData);
		// �J�n�߼޼�݂���ٰ��
		while ( pos1 && IsThread() ) {
			pData = (pChain->*pfnGetData)(pos1);
			AddMakeGdata(pData);
		}
		// [�擪|�I�[]����J�n��޼ު�Ă܂�
		for ( pos1=(pChain->*pfnGetPosition)(); pos1!=pos2 && IsThread(); ) {
			pData = (pChain->*pfnGetData)(pos1);
			AddMakeGdata(pData);
		}
		// �Ō�ɐ��������ް���Ҕ�
		CDXFdata::ms_pData = pData;
		// Z���㏸
		AddMoveGdataZup();
	}

	return TRUE;
}

BOOL MakeLoopShapeAdd_EulerMap(CDXFshape* pShape)
{
	BOOL		bEuler = FALSE;
	CDXFmap*	pEuler = pShape->GetShapeMap();
	// �P��ڂ̐�������
	if ( !MakeLoopShapeAdd_EulerMap_Make( pShape, pEuler, bEuler ) )
		return FALSE;
	if ( bEuler )
		return TRUE;	// �P��ڂőS�Đ�������

	// �����R����ް�����
	int			i;
	POSITION	pos;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	CDXFarray*	pArray;
	CDXFmap		mpLeak;
	double		dGap, dGapMin;
	CPointD		pt, ptKey;
	while ( IsThread() ) {
		// ���݈ʒu�ɋ߂���޼ު�Č���
		pDataResult = NULL;
		dGapMin = HUGE_VAL;
		pt = CDXFdata::ms_pData->GetEndCutterPoint();
		for ( pos=pEuler->GetStartPosition(); pos; ) {
			pEuler->GetNextAssoc(pos, ptKey, pArray);
			for ( i=0; i<pArray->GetSize(); i++ ) {
				pData = pArray->GetAt(i);
				if ( pData->IsMakeFlg() )
					continue;
				pData->ClearSearchFlg();
				dGap = pData->GetEdgeGap(pt);
				if ( dGap < dGapMin ) {
					dGap = dGapMin;
					pDataResult = pData;
				}
			}
		}
		if ( !pDataResult )		// ٰ�ߏI������
			break;
		// �����Wϯ�߂𐶐�
		mpLeak.RemoveAll();
		mpLeak.SetMakePointMap(pDataResult);
		pDataResult->SetSearchFlg();
		for ( i=0; i<pDataResult->GetPointNumber(); i++ ) {
			pt = pDataResult->GetNativePoint(i);
			if ( !MakeLoopShapeAdd_EulerMap_Search(pt, pEuler, &mpLeak) )
				return FALSE;
		}
		// �Q��ڈȍ~�̐�������
		bEuler = FALSE;
		if ( !MakeLoopShapeAdd_EulerMap_Make( pShape, &mpLeak, bEuler ) )
			return FALSE;
	}
	return TRUE;
}

BOOL MakeLoopShapeAdd_EulerMap_Make(CDXFshape* pShape, CDXFmap* pEuler, BOOL& bEuler)
{
	// MakeLoopEulerAdd() �Q�l
	BOOL		bReverse = FALSE;
	POSITION	pos;
	const	CPointD	ptOrg(CDXFdata::ms_ptOrg);
	CPointD		pt;
	CDXFdata*	pData;
	CDXFdata*	pDataFix;
	CDXFarray*	pArray;
	CDXFworking*	pWork;
	CDXFchain	ltEuler;

	// ���̍��Wϯ�߂���M�����v���𖞂����Ă��邩
	tie(bEuler, pArray, pt) = pEuler->IsEulerRequirement(CDXFdata::ms_pData->GetEndCutterPoint()+ptOrg);
	if ( !IsThread() )
		return -1;
	pt -= ptOrg;

	// --- ��M�����̐���(�ċA�Ăяo���ɂ��؍\�����)
	ASSERT( pArray );
	ASSERT( !pArray->IsEmpty() );
	if ( !MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler) ) {
		// ��M�����ł���n�Y�₯�ǎ��s����������ɘa���Ă�蒼��
		bEuler = FALSE;
		MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler);
	}

	// �����w������ѐ�����������
	tie(pWork, pDataFix) = pShape->GetDirectionObject();
	if ( pDataFix ) {
		// �����w����ltEuler�Ɋ܂܂��ꍇ��������
		for ( pos=ltEuler.GetHeadPosition(); pos; ) {
			pData = ltEuler.GetNext(pos);
			if ( pDataFix == pData ) {
				if ( pData->GetMakeType() == DXFCIRCLEDATA ) {
					CPointD	pts( ((CDXFworkingDirection *)pWork)->GetStartPoint() ),
							pte( ((CDXFworkingDirection *)pWork)->GetArrowPoint() );
					((CDXFcircle *)pData)->SetRoundFixed(pts, pte);
				}
				else {
					CPointD	ptFix( ((CDXFworkingDirection *)pWork)->GetArrowPoint() - ptOrg );
					if ( GAPCALC(pData->GetEndCutterPoint() - ptFix) > EPS )
						bReverse = TRUE;
				}
				break;
			}
		}
	}

	POSITION	(CDXFchain::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFchain::*pfnGetData)(POSITION&);
	CDXFdata*&	(CDXFchain::*pfnGetFirst)();
	if ( bReverse ) {
		ltEuler.ReversPoint();
		pfnGetPosition	= &(CDXFchain::GetTailPosition);
		pfnGetData		= &(CDXFchain::GetPrev);
		pfnGetFirst		= &(CDXFchain::GetTail);
	}
	else {
		pfnGetPosition	= &(CDXFchain::GetHeadPosition);
		pfnGetData		= &(CDXFchain::GetNext);
		pfnGetFirst		= &(CDXFchain::GetHead);
	}

	// --- �؍��ް�����
	ASSERT( !ltEuler.IsEmpty() );
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// �؍��ް�����
		for ( pos=(ltEuler.*pfnGetPosition)(); pos && IsThread(); ) {
			pData = (ltEuler.*pfnGetData)(pos);
			pData->SetMakeFlg();
		}
		g_ltDeepGlist.AddTail(&ltEuler);
		// �[�����S�̂��ۂ�
		if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
			// �Ō�ɐ��������ް���Ҕ�
			CDXFdata::ms_pData = g_ltDeepGlist.GetTail();
			// Z���ړ���ϰ��
			g_ltDeepGlist.AddTail((CDXFdata *)NULL);
		}
		else {
			if ( !MakeLoopDeepAdd() )
				return FALSE;
		}
	}
	else {
		// �؍��ް��܂ł̈ړ�
		pData = (ltEuler.*pfnGetFirst)();
		if ( GetNum(MKNC_NUM_TOLERANCE) == 0 )
			AddMoveGdataG0(pData);
		else
			AddMoveGdataG1(pData);
		// �؍��ް�����
		for ( pos=(ltEuler.*pfnGetPosition)(); pos && IsThread(); ) {
			pData = (ltEuler.*pfnGetData)(pos);
			AddMakeGdata(pData);
		}
		// �Ō�ɐ��������ް���Ҕ�
		CDXFdata::ms_pData = pData;
		// Z���㏸
		AddMoveGdataZup();
	}

	return TRUE;
}

BOOL MakeLoopShapeAdd_EulerMap_Search
	(const CPointD& ptKey, CDXFmap* pOrgMap, CDXFmap* pResultMap)
{
	int			i, j;
	CPointD		pt;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// ���W�𷰂ɑS�̂�ϯ�ߌ���
	if ( pOrgMap->Lookup(const_cast<CPointD&>(ptKey), pobArray) ) {
		for ( i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() && !pData->IsSearchFlg() ) {
				pResultMap->SetMakePointMap(pData);
				pData->SetSearchFlg();
				// ��޼ު�Ă̒[�_������Ɍ���
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					pt = pData->GetNativePoint(j);
					if ( ptKey != pt ) {
						if ( !MakeLoopShapeAdd_EulerMap_Search(pt, pOrgMap, pResultMap) )
							break;
					}
				}
			}
		}
	}

	return IsThread();
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

#ifdef _DEBUG
	int	n;
	g_dbg.printf("LayerName=%s", g_ltDeepGlist.GetHead()->GetParentLayer()->GetStrLayer());
	for ( POSITION p=g_ltDeepGlist.GetHeadPosition(); p; ) {
		pData = g_ltDeepGlist.GetNext(p);
		if ( pData )
			n = pData->GetParentLayer()->IsCutType() ? 1 : 2;
		else
			n = 0;
		g_dbg.printf("ListType=%d", n);
	}
#endif

	// ��]��
	CString	strSpindle( CNCMake::MakeSpindle(DXFLINEDATA) );
	if ( !strSpindle.IsEmpty() )
		AddMakeGdata(strSpindle);
	// �؍��ް��܂ł̈ړ�
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
	// g_dZCut - g_dDeep > NCMIN �Ƃ���
	while ( g_dZCut - g_dDeep > NCMIN && IsThread() ) {
		// �[����������
		pData = (*g_pfnDeepProc)(bAction, FALSE);
		CDXFdata::ms_pData = pData;
		// ����݂̐؂�ւ�(�����؍�̂�)
		if ( GetNum(MKNC_NUM_DEEPCPROCESS) == 0 ) {
			bAction = !bAction;
			// �e��޼ު�Ă̎n�_�I�_�����ւ�
			for ( pos=g_ltDeepGlist.GetHeadPosition(); pos && IsThread(); ) {
				pData = g_ltDeepGlist.GetNext(pos);
				if ( pData )
					pData->ReversePt();
			}
		}
		// Z���̉��~
		g_dZCut += GetDbl(MKNC_DBL_ZSTEP);
		if ( pData->GetParentLayer()->IsCutType() && g_dZCut-g_dDeep > NCMIN ) {
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
	CDXFdata::ms_pData = (*g_pfnDeepProc)(bAction, bFinish);

	// �[���؍�ɂ�����Z���̏㏸
	MakeLoopDeepZUp();

	// ��n��
	g_ltDeepGlist.RemoveAll();
	g_dZCut = dZCut;

	return IsThread();
}

CDXFdata* MakeLoopDeepAdd_Euler(BOOL bAction, BOOL bDeep)
{
	POSITION	(CDXFlist::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFlist::*pfnGetData)(POSITION&);
	if ( bAction ) {
		pfnGetPosition	= &(CDXFlist::GetHeadPosition);
		pfnGetData		= &(CDXFlist::GetNext);
	}
	else {
		pfnGetPosition	= &(CDXFlist::GetTailPosition);
		pfnGetData		= &(CDXFlist::GetPrev);
	}

	// �ް�����ٰ��(���]�t�])
	CDXFdata*	pData;
	for ( POSITION pos=(g_ltDeepGlist.*pfnGetPosition)(); pos && IsThread(); ) {
		pData = (g_ltDeepGlist.*pfnGetData)(pos);
		AddMakeGdataDeep(pData, bDeep);
	}

	return pData;
}

CDXFdata* MakeLoopDeepAdd_All(BOOL bAction, BOOL bDeep)
{
	POSITION	(CDXFlist::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFlist::*pfnGetData)(POSITION&);
	if ( bAction ) {
		pfnGetPosition	= &(CDXFlist::GetHeadPosition);
		pfnGetData		= &(CDXFlist::GetNext);
	}
	else {
		pfnGetPosition	= &(CDXFlist::GetTailPosition);
		pfnGetData		= &(CDXFlist::GetPrev);
	}
	BOOL		bMove = FALSE, bBreak = FALSE;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;

	// �ް�����ٰ��(���]�t�])
	for ( POSITION pos=(g_ltDeepGlist.*pfnGetPosition)(); pos && IsThread(); ) {
		// ��޼ު�Ď��o��
		pData = (g_ltDeepGlist.*pfnGetData)(pos);
		if ( pData ) {
			if ( pData->GetParentLayer()->IsCutType() ) {
				// �؍��ް�
				if ( bBreak ) {
					if ( !bMove )	// �ړ��Ȃ����
						MakeLoopDeepZUp();
					// pData�܂ňړ�(Z�����~����)
					AddMoveGdataG0(pData);
				}
				if ( bMove ) {
					// �ړ��ް�������
					AddMoveGdataZdown();
					bMove = FALSE;
				}
				AddMakeGdataDeep(pData, bDeep);
			}
			else {
				// �ړ��ް�
				ASSERT( !bBreak );
				if ( !bMove )
					MakeLoopDeepZUp();
				AddMakeMove(pData);
				bMove = TRUE;
			}
			bBreak = FALSE;
			pDataResult = pData;
		}
		else
			bBreak = TRUE;
	}

	ASSERT(pDataResult);
	return pDataResult;
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

BOOL MakeLoopDrillPoint(CDXFdata* pData)
{
	BOOL	bResult = FALSE;

	// ����ŕ��בւ��C�������ɂ����ް��𒊏o
	switch ( GetNum(MKNC_NUM_OPTIMAIZEDRILL) ) {
	case 1:		// X���
		g_obDrillAxis.SetSize(0, g_obDrillGroup.GetSize());
		g_obDrillGroup.Sort(DrillOptimaizeCompareFuncY);
		// ���݈ʒu�ɋ߂�������
		bResult = MakeLoopDrillPointSeqChk(FALSE, g_obDrillGroup) ?
			MakeLoopDrillPointXYRevers(TRUE) : MakeLoopDrillPointXY(TRUE);
		break;

	case 2:		// Y���
		g_obDrillAxis.SetSize(0, g_obDrillGroup.GetSize());
		g_obDrillGroup.Sort(DrillOptimaizeCompareFuncX);
		bResult = MakeLoopDrillPointSeqChk(TRUE, g_obDrillGroup) ?
			MakeLoopDrillPointXYRevers(FALSE) : MakeLoopDrillPointXY(FALSE);
		break;

	default:	// �Ȃ�
		bResult = MakeLoopAddDrill(pData);
		break;
	}

	return bResult;
}

BOOL MakeLoopDrillPointSeqChk(BOOL bXY, CDXFsort& pObArray)	// bXY==TRUE -> X��
{
	if ( pObArray.GetSize() <= 1 )	// �P���Ȃ��ׂ�܂ł��Ȃ�
		return FALSE;

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
			return FALSE;	// �擪���߂�
		else
			return TRUE;	// �������߂�(���]���K�v)
	}
	else {
		if ( fabs(pData1->GetEndCutterPoint().y - CDXFdata::ms_pData->GetEndCutterPoint().y) <=
			 fabs(pData2->GetEndCutterPoint().y - CDXFdata::ms_pData->GetEndCutterPoint().y) )
			return FALSE;	// �擪���߂�
		else
			return TRUE;	// �������߂�(���]���K�v)
	}
}

BOOL MakeLoopDrillPointXY(BOOL bXY)
{
	int		i = 0, nPos = 0, nLoop = g_obDrillGroup.GetSize(), n;
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

	while ( i < nLoop && IsThread() ) {
		dBase = g_obDrillGroup[i]->GetEndCutterPoint()[n] + dMargin;
#ifdef _DEBUG
		g_dbg.printf("BasePoint=%f", dBase);
#endif
		g_obDrillAxis.RemoveAll();
		while ( i < nLoop &&
				g_obDrillGroup[i]->GetEndCutterPoint()[n] <= dBase && IsThread() ) {
#ifdef _DEBUG
			g_dbg.printf("NowPoint=%f", g_obDrillGroup[i]->GetEndCutterPoint()[n]);
#endif
			g_obDrillAxis.Add(g_obDrillGroup[i++]);
		}
		if ( g_obDrillAxis.IsEmpty() )
			continue;
		g_obDrillAxis.Sort(pfnCompare);
		if ( MakeLoopDrillPointSeqChk(bXY, g_obDrillAxis) )	// ���]�K�v�H
			g_obDrillAxis.Reverse();
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
		if ( MakeLoopDrillPointSeqChk(bXY, g_obDrillAxis) )	// ���]�K�v�H
			g_obDrillAxis.Reverse();
		if ( !MakeLoopAddDrillSeq(nPos) )
			return FALSE;
		nPos = g_obDrillAxis.GetSize();
	}

	return IsThread();
}

BOOL MakeLoopDrillCircle(void)
{
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	int		i, nLoop = g_obCircle.GetSize();
	BOOL	bMatch;
	double	r, dGap, dGapMin;
	CString	strBreak;

	// ٰ�ߊJ�n
	g_obDrillGroup.SetSize(0, nLoop);
	for ( i=0; i<nLoop && IsThread(); ) {
		g_obDrillGroup.RemoveAll();
		// �~���ٰ��(���a)���Ƃɏ���
		r = fabs( ((CDXFcircle *)g_obCircle[i])->GetMakeR() );
		bMatch = FALSE;
		pDataResult = NULL;
		dGapMin = HUGE_VAL;
		for ( ; i<nLoop &&
				r==fabs(((CDXFcircle *)g_obCircle[i])->GetMakeR()) && IsThread(); i++ ) {
			pData = g_obCircle[i];
			if ( !pData->IsMakeFlg() ) {
				g_obDrillGroup.Add(pData);
				dGap = pData->GetEdgeGap(CDXFdata::ms_pData);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
					if ( sqrt(dGap) < GetDbl(MKNC_DBL_TOLERANCE) )
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
			if ( GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 )
				g_obDrillAxis.Copy(g_obDrillGroup);	// ����Ȃ��̂őS�đΏ�
			// ���H�O�ړ��w���̐���
			if ( !bMatch ) {
				pData = MakeLoopAddFirstMove(MAKEDRILLPOINT);
				if ( pData )
					pDataResult = pData;
			}
			// �ް�����
			if ( !MakeLoopDrillPoint(pDataResult) )
				return FALSE;
		}
	}

	return IsThread();
}

BOOL MakeLoopAddDrill(CDXFdata* pData)
{
	CDXFdata*	pDataMove;
	CPointD		pt;
	int			nPos = 0;
	BOOL		bMatch,
				bMove = FALSE,		// �ړ�ڲ�Hit
				bCust = TRUE;		// �ړ��ް��O�̶��Ѻ��ޑ}��

	// �ް�����
	AddMakeGdata(pData);
	CDXFdata::ms_pData = pData;

	// GetNearPoint() �̌��ʂ� NULL �ɂȂ�܂�
	while ( IsThread() ) {
		// ���̗v�f�Ɉ�ԋ߂��v�f
		tie(pData, bMatch) = GetNearPointDrill(CDXFdata::ms_pData);
		if ( !pData )
			break;
		if ( !bMatch ) {
			// �ړ��w��ڲԂ�����
			pDataMove = CDXFdata::ms_pData;		// �O���޼ު�ĂŒT��
			if ( GetMatchPointMove(pDataMove) ) {
				// �ړ�ڲԂP�ڂ̏I�[�Ɍ����H�ް���Hit�����
				tie(pData, bMatch) = GetNearPointDrill(pDataMove);
				if ( !bMove && bMatch ) {
					// �ړ�ڲ��ް��͏����������Ƃɂ��āC���̌����H�ް��𐶐�
					pDataMove->SetMakeFlg();
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
					CDXFdata::ms_pData = pDataMove;
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
		CDXFdata::ms_pData = pData;
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
	CDXFarray*	pobArray;
	CDXFdata*	pDataMove = CDXFdata::ms_pData;
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	BOOL		bMatch, bCust = FALSE;

	// �ړ��w��ڲԂ�����
	while ( GetMatchPointMove(pDataMove) && IsThread() ) {
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
			CPointD	pt(pDataResult->GetEndMakePoint());
			if ( g_mpDXFdata.Lookup(pt, pobArray) ) {
				bMatch = FALSE;
				for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
					pData = pobArray->GetAt(i);
					if ( !pData->IsMakeFlg() ) {
						pDataResult = pData;
						bMatch = TRUE;
						break;
					}
				}
				if ( bMatch )
					break;
			}
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
	while ( GetMatchPointMove(pDataMove) && IsThread() ) {
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

CDXFdata* GetNearPointCutter(const CLayerData* pLayerTarget, const CDXFdata* pDataTarget)
{
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	CLayerData*	pLayer;
	int			i, nLayerLoop = pLayerTarget ? 1 : g_pDoc->GetLayerCnt();
	double		dGap, dGapMin = HUGE_VAL;

	while ( nLayerLoop-- > 0 && IsThread() ) {
		pLayer = pLayerTarget ? const_cast<CLayerData*>(pLayerTarget) : g_pDoc->GetLayerData(nLayerLoop);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++ ) {
			pData = pLayer->GetDxfData(i);
			if ( pData->IsMakeFlg() || pData->GetMakeType()==DXFPOINTDATA )
				continue;
			dGap = pData->GetEdgeGap(pDataTarget);
			if ( dGap < dGapMin ) {
				pDataResult = pData;
				dGapMin = dGap;
			}
		}
	}

	return pDataResult;
}

CDXFshape* GetNearPointShape(const CPointD& pt)
{
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFshape*	pShapeResult = NULL;
	int			i, j;
	double		dGap, dGapMin = HUGE_VAL;

	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( j=0; j<pLayer->GetShapeSize(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			if ( !pShape->IsMakeFlg() && pShape->GetShapeAssemble()!=DXFSHAPE_EXCLUDE ) {
				dGap = pShape->GetSelectObjectFromShape(pt);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pShapeResult = pShape;
				}
			}
		}
	}

	return pShapeResult;
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

BOOL GetMatchPointMove(CDXFdata*& pDataResult)
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

		if ( *s != '{' ) {
			m_strResult += string(s, e);
			return;
		}

		// �O��� "{}" �������ĉ��
		int		nTestCode = stOrder.GetIndex(string(s+1, e-1).c_str());
		CString	strPath, strFile;
		TCHAR	szUserName[_MAX_PATH];
		DWORD	dwResult;
		CTime	time;
		double	dValue[VALUESIZE];
		// replace
		switch ( nTestCode ) {
		case 0:		// G90orG91
			m_strResult += CNCMake::MakeCustomString(GetNum(MKNC_NUM_G90)+90);
			break;
		case 1:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			dValue[NCA_Z] = GetDbl(MKNC_DBL_G92Z);
			m_strResult += CNCMake::MakeCustomString(92, NCD_X|NCD_Y|NCD_Z, dValue, FALSE);
			break;
		case 2:		// G92X
		case 3:		// G92Y
		case 4:		// G92Z
			dValue[nTestCode-2] = GetDbl(MKNC_DBL_G92X+nTestCode-2);
			m_strResult += CNCMake::MakeCustomString(-1, g_dwSetValFlags[nTestCode-2], dValue, FALSE);
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
			m_strResult += CNCMake::MakeCustomString(0, NCD_X|NCD_Y, dValue);
			break;
		case 7:		// MakeUser
			dwResult = _MAX_PATH;
			// հ�ޖ��Ɋ������܂܂�Ă���Ɛ������Ȃ�
			m_strResult += GetUserName(szUserName, &dwResult) && IsNCchar(szUserName) ?
				szUserName : szReplaceErr;
			break;
		case 8:		// MakeDate
			time = CTime::GetCurrentTime();
			VERIFY(strPath.LoadString(ID_INDICATOR_DATE_F2));	// %y/%m/%d
			m_strResult += time.Format(strPath);
			break;
		case 9:		// MakeTime
			time = CTime::GetCurrentTime();
			VERIFY(strPath.LoadString(ID_INDICATOR_TIME_F));	// %H:%M
			m_strResult += time.Format(strPath);
			break;
		case 10:	// MakeNCD
			Path_Name_From_FullPath(g_pDoc->GetNCFileName(), strPath, strFile);
			m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
			break;
		case 11:	// MakeDXF
			Path_Name_From_FullPath(g_pDoc->GetPathName(), strPath, strFile);	// GetTitle()�ɂ�"*"���t���\������
			m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
			break;
		case 12:	// MakeCondition
			Path_Name_From_FullPath(g_pMakeOpt->GetInitFile(), strPath, strFile);
			m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
			break;
		}
	}
};

void AddCustomCode(const CString& strFileName, const CDXFdata* pData)
{
	using namespace boost::spirit;

	CString	strBuf;
	string	strResult;
	CMakeCustomCode		custom(strResult, pData);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) ) {
			// �\�����
			strResult.clear();
			if ( parse((LPCTSTR)strBuf, *( *(anychar_p - '{')[custom] >> comment_p('{', '}')[custom] ) ).hit ) {
				if ( !strResult.empty() )
					AddMakeGdata( strResult.c_str() );
			}
			else
				AddMakeGdata( strBuf );
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
	for ( int i=0; i<lstrlen(lpsz); i++ ) {
//		if ( isprint(lpsz[i]) == 0 )
		if ( IsDBCSLeadByte(lpsz[i]) )
			return FALSE;
	}
	return TRUE;
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
void AddCommentText(const CPointD& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFcomment.Lookup(const_cast<CPointD&>(pt), pobArray) ) {
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

void AddStartTextIntegrated(const CPointD& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFstarttext.Lookup(const_cast<CPointD&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				AddMakeText(pData);
				return;
			}
		}
	}
}

void AddMoveTextIntegrated(const CPointD& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFmovetext.Lookup(const_cast<CPointD&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				AddMakeText(pData);
				return;
			}
		}
	}
}

void AddCutterTextIntegrated(const CPointD& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFtext.Lookup(const_cast<CPointD&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				AddMakeText(pData);
				return;
			}
		}
	}
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
		strMsg.Format(IDS_MAKENCD_FASE, g_nFase);
	g_pParent->SetFaseMessage(strMsg, lpszMsg2);
	g_nFase++;
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

// NC�����̸�۰��ٕϐ�������(��n��)�گ��
UINT MakeNCD_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_AfterThread()\nStart", TRUE, DBG_RED);
#endif
	for ( int i=0; i<g_obMakeGdata.GetSize(); i++ )
		delete	g_obMakeGdata[i];
	g_obMakeGdata.RemoveAll();
	g_mpDXFdata.RemoveAll();
	g_mpDXFstarttext.RemoveAll();
	g_mpDXFmove.RemoveAll();
	g_mpDXFmovetext.RemoveAll();
	g_mpDXFtext.RemoveAll();
	g_mpDXFcomment.RemoveAll();
	g_obStartData.RemoveAll();
	g_ltDeepGlist.RemoveAll();
	g_obPoint.RemoveAll();
	g_obCircle.RemoveAll();
	g_obDrillGroup.RemoveAll();
	g_obDrillAxis.RemoveAll();

	g_csMakeAfter.Unlock();
	return 0;
}
