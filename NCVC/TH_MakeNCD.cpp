// TH_MakeNCD.cpp
//		NC ����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFOption.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"
//#include "boost/bind.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DBG_NCMAKE_TIME	//	�������Ԃ̕\��
#endif

using std::string;
using namespace boost;

// --- CDXFdata �� GetType() �� GetMakeType() �̎g�������ɒ��ӁI�I

// ��۰��ٕϐ���`
static	CThreadDlg*		g_pParent;
static	CDXFDoc*		g_pDoc;
static	CNCMakeMillOpt*	g_pMakeOpt;

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)

// NC�����ɕK�v���ް��Q
static	CDXFmap		g_mpDXFdata,	// ���W�𷰂�CDXFdata���i�[
			g_mpDXFstarttext,				// �J�nڲ�
			g_mpDXFmove, g_mpDXFmovetext,	// �ړ�ڲ�ϯ��
			g_mpDXFtext, g_mpDXFcomment;	// ���H÷�āC���Đ�p
typedef	CTypedPtrMap<CMapStringToOb, CString, CDXFshape*>	CShapeHandleMap;
typedef	CTypedPtrArrayEx<CPtrArray, CDXFsort*>	CDrillAxis;
static	CDXFsort	g_obDrill;		// �����H�ް�
static	CSortArray<CObArray, CDXFcircle*>
					g_obCircle;		// �~�ް��������H����Ƃ��̉��o�^
static	CDXFsort	g_obStartData;	// ���H�J�n�ʒu�w���ް�
static	CDXFlist	g_ltDeepData;	// �[���؍�p�̉��o�^
static	CTypedPtrArrayEx<CPtrArray, CNCMakeMill*>	g_obMakeData;	// ���H�ް�

static	WORD		g_wBindOperator;// Bind����̧�ُo�͎w��
static	BOOL		g_bData;		// �e���������Ő������ꂽ��
static	float		g_dZCut;		// Z���̐؍���W == RoundUp(GetDbl(MKNC_DBL_ZCUT))
static	float		g_dDeep;		// �[���̐؍���W == RoundUp(GetDbl(MKNC_DBL_DEEP))
static	float		g_dZInitial;	// Z���̲Ƽ�ٓ_ == RoundUp(GetDbl(MKNC_DBL_G92Z))
static	float		g_dZG0Stop;		// Z����R�_ == RoundUp(GetDbl(MKNC_DBL_ZG0STOP))
static	float		g_dZReturn;		// Z���̕��A���W
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
static	void	SetGlobalMapToOther(void);
static	BOOL	MakeNCD_MainFunc(CLayerData*);		// NC������Ҳ�ٰ��
static	BOOL	MakeNCD_ShapeFunc(void);
static	BOOL	MakeNCD_FinalFunc(LPCTSTR = NULL);	// �I�����ށÇ�ُo�͂Ȃ�
static	BOOL	OutputMillCode(LPCTSTR);	// NC���ނ̏o��
static	BOOL	CreateShapeThread(void);	// �`��F������

// �e���ߕʂ̏����Ăяo��
enum	ENMAKETYPE {MAKECUTTER, MAKECORRECT, MAKEDRILLPOINT, MAKEDRILLCIRCLE};
static	BOOL	CallMakeDrill(CLayerData*, CString&);
static	BOOL	CallMakeLoop(ENMAKETYPE, CLayerData*, CString&);

// ���_����(TRUE:IsMatch)
static	tuple<CDXFdata*, BOOL>	OrgTuningCutter(const CLayerData*);
static	tuple<CDXFdata*, BOOL>	OrgTuningDrillPoint(void);
static	CDXFdata*	OrgTuningDrillCircle(void);

// �ް���͊֐�
static	BOOL		MakeLoopEuler(const CLayerData*, CDXFdata*);
static	BOOL		MakeLoopEulerSearch(const CPointF&, CDXFmap&);
static	INT_PTR		MakeLoopEulerAdd(const CDXFmap*);
static	BOOL		MakeLoopEulerAdd_with_one_stroke(const CDXFmap*, BOOL, const CPointF&, const CDXFarray*, CDXFlist&);
static	BOOL		MakeLoopShape(CDXFshape*);
static	INT_PTR		MakeLoopShapeSearch(const CDXFshape*);
static	BOOL		MakeLoopShapeAdd(CDXFshape*, CDXFdata*);
static	BOOL		MakeLoopShapeAdd_ChainList(CDXFshape*, CDXFchain*, CDXFdata*);
static	BOOL		MakeLoopShapeAdd_EulerMap(CDXFshape*);
static	BOOL		MakeLoopShapeAdd_EulerMap_Make(CDXFshape*, CDXFmap*, BOOL&);
static	BOOL		MakeLoopShapeAdd_EulerMap_Search(const CPointF&, CDXFmap*, CDXFmap*);
static	BOOL		MakeLoopDeepAdd(void);
static	function<CDXFdata* (BOOL, BOOL, BOOL)>	g_pfnDeepProc;	// MakeLoopDeepAdd_*
static	CDXFdata*	MakeLoopDeepAdd_Euler(BOOL, BOOL, BOOL);
static	CDXFdata*	MakeLoopDeepAdd_All(BOOL, BOOL, BOOL);
static	void		MakeLoopDeepZDown(void);
static	void		MakeLoopDeepZUp(void);
static	BOOL		MakeLoopDrillPoint(CDXFdata*);
static	void		MakeLoopDrillPoint_MakeAxis(int, CDrillAxis&);
static	INT_PTR		MakeLoopDrillPoint_EdgeChk(int, CDXFsort*);
static	INT_PTR		MakeLoopDrillPoint_SeqChk(INT_PTR, int, CDXFsort*);
static	BOOL		MakeLoopDrillCircle(void);
static	BOOL		MakeLoopAddDrill(CDXFdata*);
static	tuple<INT_PTR, CDXFdata*>	MakeLoopAddDrillSeq(INT_PTR, INT_PTR, CDXFsort*);
static	CDXFdata*	MakeLoopAddFirstMove(ENMAKETYPE);
static	BOOL		MakeLoopAddLastMove(void);

// �����Ŏw�肵����޼ު�ĂɈ�ԋ߂���޼ު�Ă�Ԃ�(�߂�l:Z���̈ړ����K�v��)
static	CDXFdata*	GetNearPointCutter(const CLayerData*, const CDXFdata*);
static	CDXFshape*	GetNearPointShape(const CPointF&);
static	tuple<CDXFdata*, BOOL>	GetNearPointDrill(const CDXFdata*);
static	tuple<int, int>			GetNearPointDrillAxis(const CDXFdata*, int, CDrillAxis&);
static	CDXFdata*	GetMatchPointMove(const CDXFdata*);

// ͯ�ް,̯�ް���̽�߼�ٺ��ސ���
static	void	AddCustomMillCode(const CString&, const CDXFdata*);

// ÷�ď��
static	void	AddCommentText(const CPointF&);
static	void	AddStartTextIntegrated(const CPointF& pt);
static	void	AddMoveTextIntegrated(const CPointF& pt);
static	void	AddCutterTextIntegrated(const CPointF& pt);

// NC�ް��o�^�֐�
static	void	AddMakeStart(void);		// ���H�J�n�w���ް��̐���
static	function<void ()>	g_pfnAddMoveZ,	// �ړ��w��ڲԂ̈ړ����ɂ����铮��֐�
							g_pfnAddMoveCust_B,
							g_pfnAddMoveCust_A;
static	void	AddMoveZ_NotMove(void);
static	void	AddMoveZ_R(void);
static	void	AddMoveZ_Initial(void);
static	void	AddMoveCust_B(void);
static	void	AddMoveCust_A(void);
static	function<void (const CDXFdata*)>	g_pfnAddMoveGdata;
static	void	AddMoveGdataG0(const CDXFdata*);
static	void	AddMoveGdataG1(const CDXFdata*);
static	void	AddMoveGdataApproach(const CDXFdata*);

// Z���i���A�v���[�`�̉�
static inline	BOOL	_IsZApproach(const CDXFdata* pData)
{
	return pData->GetMakeType()==DXFLINEDATA && pData->GetLength()>GetDbl(MKNC_DBL_ZAPPROACH);
}
// Z���̈ړ�(�؍�)�ް�����
static inline	void	_AddMoveGdataZ(int nCode, float dZ, float dFeed)
{
	CNCMakeMill*	pNCD = new CNCMakeMill(nCode, dZ, dFeed);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// Z���̏㏸
static inline	void	_AddMoveGdataZup(void)
{
	// �I�_���W�ł̺��Đ���
	AddCutterTextIntegrated( CDXFdata::ms_pData->GetEndCutterPoint() );
	// Z���̏㏸
	_AddMoveGdataZ(0, g_dZReturn, -1.0f);
}
// Z���̉��~
static inline	float	_GetZValue(void)
{
	float	dZValue;
	switch ( GetNum(MKNC_NUM_MAKEEND) ) {
	case 1:		// �̾�Ĉړ�
		dZValue = g_dZCut + GetDbl(MKNC_DBL_MAKEEND);
		break;
	case 2:		// �Œ�Z�l
		dZValue = GetDbl(MKNC_DBL_MAKEEND);
		break;
	default:
		dZValue = HUGE_VALF;
		break;
	}
	return dZValue;
}
static inline	void	_AddMoveGdataZdown(void)
{
	// Z���̌��݈ʒu��R�_���傫��(����)�Ȃ�R�_�܂ő�����
	if ( CNCMakeMill::ms_xyz[NCA_Z] > g_dZG0Stop )
		_AddMoveGdataZ(0, g_dZG0Stop, -1.0f);
	// ���H�ςݐ[���ւ�Z���؍�ړ�
	float	dZValue = _GetZValue();
	if ( CNCMakeMill::ms_xyz[NCA_Z] > dZValue )
		_AddMoveGdataZ(1, dZValue, GetDbl(MKNC_DBL_MAKEENDFEED));
	// �؍�_�܂Ő؍푗��
	if ( CNCMakeMill::ms_xyz[NCA_Z] > g_dZCut )
		_AddMoveGdataZ(1, g_dZCut, GetDbl(MKNC_DBL_ZFEED));
}
// �ړ��ް�
static inline	void	_AddMoveGdata(int nCode, const CDXFdata* pData, float dFeed)
{
	CNCMakeMill* pNCD = new CNCMakeMill(nCode, pData->GetStartMakePoint(), dFeed);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// �؍��ް�
static inline	void	_AddMakeGdata(CDXFdata* pData, float dFeed)
{
	ASSERT( pData );
	// �J�n�ʒu�Ɠ�����÷���ް��̐���
	AddCutterTextIntegrated( pData->GetStartCutterPoint() );
	// �؍��ް�����
	CNCMakeMill*	pNCD = new CNCMakeMill(pData, dFeed);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pData->SetMakeFlg();
}
static inline	void	_AddMakeGdataCut(CDXFdata* pData)
{
	// �؍��ް��̐���(�؍�)
	_AddMakeGdata(pData, GetDbl(MKNC_DBL_FEED));
}
static inline	void	_AddMakeGdataDeep(CDXFdata* pData, BOOL bDeepFin)
{
	// �؍��ް��̐���(�[��)
	_AddMakeGdata(pData, bDeepFin ? GetDbl(MKNC_DBL_DEEPFEED) : GetDbl(MKNC_DBL_FEED));
}
static inline	void	_AddMakeGdataApproach(const CPointF& pts, const CPointF& pte, int nCode, float dFeed = 0.0f)
{
	CPointF	pt( CalcIntersectionPoint_TC(pts, GetDbl(MKNC_DBL_ZAPPROACH), pte) );
	CNCMakeMill* pNCD = new CNCMakeMill(nCode, pt, dFeed);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
static inline	void	_AddMakeGdata3dCut(const CPoint3F& pt3d)
{
	// pt3d��3������
	CNCMakeMill* pNCD = new CNCMakeMill(pt3d, GetDbl(MKNC_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	// �h�E�F������
	if ( GetDbl(MKNC_DBL_ZAPPDWELL) > 0.0f ) {
		CNCMakeMill* pNCD = new CNCMakeMill(GetDbl(MKNC_DBL_ZAPPDWELL));
		ASSERT( pNCD );
		g_obMakeData.Add(pNCD);
	}
}
static inline	void	_AddMakeGdataDrill(CDXFdata* pData)
{
	// �؍��ް��̐���(�����H)
	_AddMakeGdata(pData, GetDbl(MKNC_DBL_DRILLFEED));
}
static inline	void	_AddMakeGdataHelical(CDXFdata* pData)
{
	// �؍��ް��̐���(�[���~�ް����ضِ؍�)
	ASSERT( pData );
	AddCutterTextIntegrated( pData->GetStartCutterPoint() );
	CNCMakeMill*	pNCD = new CNCMakeMill(pData, GetDbl(MKNC_DBL_FEED), &g_dZCut);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pData->SetMakeFlg();
}
// �C���ް��̐���
static inline	void	_AddMakeGdataStr(const CString& strData)
{
	CNCMakeMill*	pNCD = new CNCMakeMill(strData);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// �ړ��w��ڲԂ��ް�����
static inline	void	_AddMakeMove(CDXFdata* pData, BOOL bL0 = FALSE)
{
	// �J�n�ʒu�Ɠ�����÷���ް��̐���
	AddMoveTextIntegrated( pData->GetStartCutterPoint() );
	// �ړ��w��
	CNCMakeMill*	pNCD = new CNCMakeMill(pData, bL0);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pData->SetMakeFlg();
}
// �Œ軲�ٷ�ݾٺ���
static inline	void	_AddMakeGdataCycleCancel(void)
{
	if ( CDXFdata::ms_pData->GetMakeType() == DXFPOINTDATA ) {
		_AddMakeGdataStr( CNCMakeBase::MakeCustomString(80) );
		InitialCycleBaseVariable();
	}
}
// �������o��
static inline	void	_AddMakeText(CDXFdata* pData)
{
	_AddMakeGdataStr(static_cast<CDXFtext*>(pData)->GetStrValue());
	pData->SetMakeFlg();
}

// ̪��ލX�V
static	int		g_nFase;			// ̪��އ�
static	void	SendFaseMessage(INT_PTR = -1, int = -1, LPCTSTR = NULL);
static inline	void	_SetProgressPos64(INT_PTR i)
{
	if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
		g_pParent->m_ctReadProgress.SetPos((int)i);
}
static	inline	void	_SetProgressPos(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetPos((int)n);
}

// ���בւ��⏕�֐�
static	int		CircleSizeCompareFunc1(CDXFcircle*, CDXFcircle*);	// �~�ް��������
static	int		CircleSizeCompareFunc2(CDXFcircle*, CDXFcircle*);	// �~�ް��~�����
static	int		DrillOptimaizeCompareFuncX(CDXFdata*, CDXFdata*);	// X��
static	int		DrillOptimaizeCompareFuncY(CDXFdata*, CDXFdata*);	// Y��
static	int		AreaSizeCompareFunc(CDXFchain*, CDXFchain*);	// �`��W���̖ʐϏ������

// ��޽گ�ފ֐�
static	CCriticalSection	g_csMakeAfter;	// MakeNCD_AfterThread()�گ��ۯ���޼ު��
static	UINT	MakeNCD_AfterThread(LPVOID);	// ��n���گ��

//////////////////////////////////////////////////////////////////////
// NC�����گ��
//////////////////////////////////////////////////////////////////////

UINT MakeNCD_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeNCD_Thread() Start\n");
#endif
#ifdef _DBG_NCMAKE_TIME
	// ���ݎ������擾
	CTime t1 = CTime::GetCurrentTime();
#endif

	int		i, nResult = IDCANCEL;

	// ��۰��ٕϐ�������
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);
	i = (int)(pParam->lParam);
	//	= 0 : Normal
	//	= 1 : ̯��o�͂Ȃ�
	//	> 1 : ̧�ْǉ�Ӱ��, ͯ��/̯��o�͂Ȃ�
	//	=-1 : ̧�ْǉ�Ӱ��, ͯ�ޏo�͂Ȃ�
	//	=-2 : one bind
	if ( i == 1 )
		g_wBindOperator = TH_HEADER;
	else if ( i > 1 )
		g_wBindOperator = TH_APPEND;
	else if ( i == -1 )
		g_wBindOperator = TH_FOOTER | TH_APPEND;
	else
		g_wBindOperator = TH_HEADER | TH_FOOTER;

	// �������\��
	g_nFase = 0;
	SendFaseMessage(-1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		// NC��������
		int		nID = (int)(pParam->wParam);

		// NC������߼�ݵ�޼ު�Ă̐���
		g_pMakeOpt = new CNCMakeMillOpt(NULL);	// �ǂݍ��݂͂��Ȃ�

		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
#ifdef _DEBUG
		printf("g_csMakeAfter Unlock OK\n");
#endif

		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		{
			optional<CPointF>	ptResult = g_pDoc->GetCutterOrigin();
			CDXFdata::ms_ptOrg = ptResult ? *ptResult : 0.0f;
		}
		g_pDoc->GetCircleObject()->OrgTuning(FALSE);	// ���ʓI�Ɍ��_����ۂɂȂ�
		InitialVariable();
		// �������蓖��
		i = g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER);
		g_obDrill.SetSize(0, i);
		g_obCircle.SetSize(0, i);
		i *= 2;
		g_obMakeData.SetSize(0, i);
		i = GetPrimeNumber(i);
		i = max(17, i);
		g_mpDXFdata.InitHashTable(i);
		g_mpDXFtext.InitHashTable(i);
		i = g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER) * 2;
		g_obStartData.SetSize(0, max(10, i));
		i = GetPrimeNumber(i);
		g_mpDXFstarttext.InitHashTable(max(17, i));
		i = GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFMOVLAYER)*2);
		i = max(17, i);
		g_mpDXFmove.InitHashTable(i);
		g_mpDXFmovetext.InitHashTable(i);
		i = GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFCOMLAYER));
		g_mpDXFcomment.InitHashTable(max(17, i));

		// Ҳ�ٰ�߂�
		BOOL	bResult = FALSE;
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
		printf("MakeNCD_Thread All Over!!!\n");
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
	_dp.SetDecimal3();
	CDXFmap::ms_dTolerance = NCMIN;		// �K��l�ɖ߂�
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��
	// ��������NC���ނ̏����گ��(�D��x��������)
	AfxBeginThread(MakeNCD_AfterThread, NULL,
		THREAD_PRIORITY_LOWEST);
//		THREAD_PRIORITY_IDLE);
//		THREAD_PRIORITY_BELOW_NORMAL);

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
	if ( !(g_wBindOperator & TH_APPEND) )
		CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	CNCMakeMill::InitialVariable();		// CNCMakeBase::InitialVariable()
}

void InitialCycleBaseVariable(void)
{
	CNCMakeMill::ms_dCycleZ[1] =
	CNCMakeMill::ms_dCycleR[1] =
	CNCMakeMill::ms_dCycleP[1] =
	CNCMakeMill::ms_dCycleQ[1] = HUGE_VALF;
}

//////////////////////////////////////////////////////////////////////

BOOL SingleLayer(int nID)
{
#ifdef _DEBUG
	printf("SingleLayer() Start\n");
#endif
	// NC������߼�ݓǂݍ���
	g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKEMILL)->GetHead());
	// �������Ƃɕω��������Ұ���ݒ�
	SetStaticOption();
	// ���H���_�J�n�ʒu�w��ڲԏ���
	if ( !SetStartData() )
		return FALSE;
	// Z���̐؍���W���
	g_dZCut = RoundUp(GetDbl(MKNC_DBL_ZCUT));
	g_dDeep = RoundUp(GetDbl(MKNC_DBL_DEEP));
	// �Œ軲�ق̐؂荞�ݍ��W���
	CNCMakeMill::ms_dCycleZ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLZ));
	CNCMakeMill::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
	CNCMakeMill::ms_dCycleQ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLQ));
	CNCMakeMill::ms_dCycleP[0] = RoundUp(GetDbl(MKNC_DBL_DWELL));
	InitialCycleBaseVariable();
	// ��۰��ٕϐ��ɐ����Ώ۵�޼ު�Ă̺�߰
	SetGlobalMap();

	// �����J�n
	if ( nID == ID_FILE_DXF2NCD ) {
		if ( !MakeNCD_MainFunc(NULL) ) {
#ifdef _DEBUG
			printf("Error:MakeNCD_MainFunc()\n");
			// �ǂ��܂ŏ����ł��Ă��邩���킩��
			if ( AfxMessageBox("Output?", MB_YESNO|MB_ICONQUESTION) == IDYES )
				MakeNCD_FinalFunc();
#endif
			return FALSE;
		}
	}
	else {
		if ( !MakeNCD_ShapeFunc() ) {
#ifdef _DEBUG
			printf("Error:MakeNCD_ShapeFunc()\n");
			if ( AfxMessageBox("Output?", MB_YESNO|MB_ICONQUESTION) == IDYES )
				MakeNCD_FinalFunc();
#endif
			return FALSE;
		}
	}

	// �ŏI����
	if ( !g_bData ) {
		AfxMessageBox(IDS_ERR_MAKENC, MB_OK|MB_ICONSTOP);
		return FALSE;
	}
	// �I�����ށÇ�ق̏o�͂Ȃ�
	if ( !MakeNCD_FinalFunc() ) {
#ifdef _DEBUG
		printf("Error:MakeNCD_FinalFunc()\n");
#endif
		return FALSE;
	}

	return TRUE;
}

BOOL MultiLayer(int nID)
{
#ifdef _DEBUG
	printf("MultiLayer() Start\n");
#endif
	extern	LPCTSTR	gg_szCat;	// ", "
	INT_PTR	i, j, nLayerCnt = g_pDoc->GetLayerCnt();
	BOOL	bPartOut = FALSE,	// �P��ł��ʏo�͂������TRUE
			bNotPart = FALSE;	// �S�̏o�͂�ٰ�ߏI���Ȃ�  TRUE
	CLayerData*	pLayer;

	if ( nID == ID_FILE_DXF2NCD_EX2 ) {
		// ��ƂȂ�NC������߼�ݓǂݍ���
		g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKEMILL)->GetHead());
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
		pLayer->SetLayerPartFlag();
		// ����ڲԖ����޲�۸ނɕ\��
		g_nFase = 1;
		SendFaseMessage(-1, IDS_ANA_DATAINIT, pLayer->GetLayerName());
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
			CNCMakeMill::ms_dCycleZ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		}
		else {
			// ����Z���W�␳
			g_dZCut = g_dDeep = RoundUp(pLayer->GetZCut());
			// �Œ軲�ق̐؂荞�ݍ��W���
			CNCMakeMill::ms_dCycleZ[0] = pLayer->IsLayerFlag(LAYER_DRILL_Z) ?
						g_dZCut : RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		}
		// �Œ軲�ق��̑��̍��W���
		CNCMakeMill::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
		CNCMakeMill::ms_dCycleQ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLQ));
		CNCMakeMill::ms_dCycleP[0] = RoundUp(GetDbl(MKNC_DBL_DWELL));
		InitialCycleBaseVariable();
		// ��۰��ٕϐ��ɐ����Ώ۵�޼ު�Ă̺�߰
		g_mpDXFdata.RemoveAll();
		g_mpDXFtext.RemoveAll();
		g_obDrill.RemoveAll();
		g_obCircle.RemoveAll();
		g_ltDeepData.RemoveAll();
		SetGlobalMapToLayer(pLayer);
		// �ꎞ�̈�Ŏg�p����ϯ�߂��폜
		g_mpDXFdata.RemoveAll();

		// �����J�n
#ifdef _DEBUG
		printf("No.%d ID=%d Name=%s Cut=%f\n", i+1,
			pLayer->GetLayerListNo(), LPCTSTR(pLayer->GetLayerName()), g_dZCut);
#endif
		if ( !MakeNCD_MainFunc(pLayer) ) {
#ifdef _DEBUG
			printf("MakeNCD_MainFunc() Error\n");
#endif
			return FALSE;
		}

		// �ʏo�͂łȂ��Ȃ�(���בւ����Ă���̂œr���Ɋ��荞�ނ��Ƃ͂Ȃ�)
		if ( !pLayer->IsLayerFlag(LAYER_PART_OUT) ) {
			bNotPart = TRUE;
			continue;
		}

		// --- �ȉ��ʏo�݂͂̂̏���
		if ( g_bData ) {	// NC�����؁H
			// MakeNCD_FinalFunc(�I�����ށÇ�ق̏o��)�̎��s
			if ( MakeNCD_FinalFunc(pLayer->GetNCFile()) ) {
				// ��۰��ٕϐ�������
				InitialVariable();
				// NC�����Ϗ��̍폜
				for ( j=0; j<g_obMakeData.GetSize() && IsThread(); j++ )
					delete	g_obMakeData[j];
				g_obMakeData.RemoveAll();
			}
			else {
#ifdef _DEBUG
				printf("MakeNCD_FinalFunc_Multi() Error\n");
#endif
				return FALSE;
			}
			bPartOut = TRUE;	// �P��ł��ʏo�͂���
		}
		else {
#ifdef _DEBUG
			printf("Layer=%s CDXFdata::ms_pData NULL\n", LPCTSTR(pLayer->GetLayerName()));
#endif
			// �Y��ڲԂ��ް��Ȃ�
			pLayer->SetLayerPartFlag(TRUE);
		}
	}	// End of for main loop (Layer)

	// --- �ŏI����
	if ( bNotPart ) {	// ٰ�߂��S�̏o�͂ŏI��
		if ( g_bData ) {	// NC�����؁H
			// MakeNCD_FinalFunc(�I�����ށÇ�ق̏o��)�̎��s
			if ( !MakeNCD_FinalFunc() ) {
#ifdef _DEBUG
				printf("MakeNCD_FinalFunc()\n");
#endif
				return FALSE;
			}
		}
		else {				// �o��ż
			if ( bPartOut ) {	// �ʏo�͂������
				// �ʏo�͈ȊO��ڲԏ����擾���Cܰ�ݸ�ү���ޏo�͂�
				for ( i=0; i<nLayerCnt; i++ ) {
					pLayer = g_pDoc->GetLayerData(i);
					if ( !pLayer->IsLayerFlag(LAYER_PART_OUT) )
						pLayer->SetLayerPartFlag(TRUE);
				}
			}
			else {
				// �װү����
				AfxMessageBox(IDS_ERR_MAKENC, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
		}
	}

	// �ʏo�͂�ܰ�ݸ�ү����
	CString	strMiss;
	for ( i=0; i<nLayerCnt; i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( pLayer->IsLayerFlag(LAYER_PART_ERROR) ) {
			if ( !strMiss.IsEmpty() )
				strMiss += gg_szCat;
			strMiss += pLayer->GetLayerName();
		}
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
	// _AddMoveGdataZ(Z���̏㏸)�Ŏg�p
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
	g_pfnAddMoveCust_B = GetStr(MKNC_STR_CUSTMOVE_B).IsEmpty() ?
		&AddMoveZ_NotMove : &AddMoveCust_B;
	g_pfnAddMoveCust_A = GetStr(MKNC_STR_CUSTMOVE_A).IsEmpty() ?
		&AddMoveZ_NotMove : &AddMoveCust_A;
	// ���H�������e��
	CDXFmap::ms_dTolerance = GetFlg(MKNC_FLG_DEEP) ?
		NCMIN : GetDbl(MKNC_DBL_TOLERANCE);
	// �[���̏���
	g_pfnDeepProc = GetNum(MKNC_NUM_DEEPALL) == 0 ?
		&MakeLoopDeepAdd_All : &MakeLoopDeepAdd_Euler;
	// Z���i���A�v���[�`
	if ( GetNum(MKNC_NUM_TOLERANCE) == 0 )
		g_pfnAddMoveGdata = GetDbl(MKNC_DBL_ZAPPROACH) > NCMIN ? &AddMoveGdataApproach : &AddMoveGdataG0;
	else
		g_pfnAddMoveGdata = &AddMoveGdataG1;

	// CDXFdata�̐ÓI�ϐ�������
	CDXFdata::ms_fXRev = GetFlg(MKNC_FLG_XREV);
	CDXFdata::ms_fYRev = GetFlg(MKNC_FLG_YREV);
	CDXFpoint::ms_pfnOrgDrillTuning = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ?
		&CDXFpoint::OrgTuning_Seq : &CDXFpoint::OrgTuning_XY;

	// CNCMakeMill�̐ÓI�ϐ�������
	if ( !g_bData ) {
		// ABS, INC �֌W�Ȃ� G92�l�ŏ�����
		for ( int i=0; i<NCXYZ; i++ )
			CNCMakeMill::ms_xyz[i] = RoundUp(GetDbl(MKNC_DBL_G92X+i));
	}
	// ������߼�݂ɂ��ÓI�ϐ��̏�����
	CNCMakeMill::SetStaticOption(g_pMakeOpt);
}

BOOL SetStartData(void)
{
	ASSERT(CDXFdata::ms_pData);

	int		i, j;
	CDXFdata*	pData;
	CDXFdata*	pMatchData = NULL;
	CDXFdata*	pDataResult = NULL;
	CDXFcircleEx*	pStartCircle = NULL;
	float		dGap, dGapMin = FLT_MAX;
	CString		strLayer;
	CLayerData*	pLayer;
	CDXFarray	obArray;
	obArray.SetSize(0, g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER));

	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsLayerFlag(LAYER_CUT_TARGET) || pLayer->GetLayerType()!=DXFSTRLAYER )
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
				pStartCircle = static_cast<CDXFcircleEx*>(pData);
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
		dGapMin = FLT_MAX;
		for ( i=0; i<obArray.GetSize() && IsThread(); i++ ) {
			pData = obArray[i];
			if ( pData->IsSearchFlg() )
				continue;
			if ( pData->IsMakeMatchObject(pMatchData) ) {
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
	CPointF	pt;
	CDXFarray*	pDummy = NULL;
	CDXFdata*		pData;
	CDXFcircle*		pCircle;
	CDXFellipse*	pEllipse;

	// �w��ڲԂ�DXF�ް���޼ު��(�����H�̂�)���߰
	for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++ ) {
		pData = pLayer->GetDxfData(i);
		// �ȉ~�ް��̕ϐg
		if ( pData->GetType() == DXFELLIPSEDATA ) {
			pEllipse = static_cast<CDXFellipse*>(pData);
			if ( GetFlg(MKNC_FLG_ELLIPSE) && pEllipse->IsLongEqShort() ) {
				// ���a�Z�a���������ȉ~�Ȃ�~�ʂ��~�ް��ɕϐg
				pData->ChangeMakeType( pEllipse->IsArc() ? DXFARCDATA : DXFCIRCLEDATA);
			}
		}
		// �e��޼ު�����߂��Ƃ̏���
		switch ( pData->GetMakeType() ) {
		case DXFPOINTDATA:
			// �d������
			if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
				pt = pData->GetNativePoint(0);
				if ( !g_mpDXFdata.Lookup(pt, pDummy) ) {
					g_obDrill.Add(pData);
					g_mpDXFdata.SetAt(pt, pDummy);
				}
			}
			else
				g_obDrill.Add(pData);
			break;
		case DXFCIRCLEDATA:
			pCircle = static_cast<CDXFcircle*>(pData);
			// �~�ް��̔��a���w��l�ȉ��Ȃ�
			if ( GetFlg(MKNC_FLG_DRILLCIRCLE) &&
					pCircle->IsUnderRadius(GetDbl(MKNC_DBL_DRILLCIRCLE)) ) {
				// �����H�ް��ɕϐg���ēo�^
				pData->ChangeMakeType(DXFPOINTDATA);
				// �����ł͏d���������s��Ȃ��B
				// ���בւ�����(OrgTuningDrillCircle)�ɍs��
				g_obCircle.Add(pCircle);
				break;
			}
			break;
		}
	}

	// ÷���ް���ϯ�ߍ쐬
	for ( i=0; i<pLayer->GetDxfTextSize() && IsThread(); i++ ) {
		pData = pLayer->GetDxfTextData(i);
		pData->OrgTuning(FALSE);
		g_mpDXFtext.SetMakePointMap(pData);
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
		if ( !pLayer->IsLayerFlag(LAYER_CUT_TARGET) )
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
		_AddMoveGdataZ(0, g_dZInitial, -1);
	// G����̯��(�I������)
	if ( g_wBindOperator & TH_FOOTER )
		AddCustomMillCode(GetStr(MKNC_STR_FOOTER), NULL);
	// ̧�ُo��̪���
	return OutputMillCode(lpszFileName);
}

BOOL OutputMillCode(LPCTSTR lpszFileName)
{
	static CString	s_strNCtemp;
	CString	strPath, strFile, strWriteFile,
			strNCFile(lpszFileName ? lpszFileName : g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_obMakeData.GetSize(), IDS_ANA_DATAFINAL, strFile);

	// �ꎞ̧�ِ������ۂ�
	if ( g_wBindOperator & TH_HEADER ) {
		if ( !GetStr(MKNC_STR_PERLSCRIPT).IsEmpty() && ::IsFileExist(GetStr(MKNC_STR_PERLSCRIPT), TRUE, FALSE) ) {
			TCHAR	szPath[_MAX_PATH], szFile[_MAX_PATH];
			::GetTempPath(_MAX_PATH, szPath);
			::GetTempFileName(szPath, AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3)/*ncd*/,
				0, szFile);
			strWriteFile = s_strNCtemp = szFile;
		}
		else {
			strWriteFile = strNCFile;
			s_strNCtemp.Empty();
		}
	}
	else {
		strWriteFile = s_strNCtemp.IsEmpty() ? strNCFile : s_strNCtemp;
	}

	// ̧�ُo��
	try {
		UINT	nOpenFlg = CFile::modeCreate | CFile::modeWrite |
			CFile::shareExclusive | CFile::typeText | CFile::osSequentialScan;
		if ( g_wBindOperator & TH_APPEND )
			nOpenFlg |= CFile::modeNoTruncate;
		CStdioFile	fp(strWriteFile, nOpenFlg);
		if ( g_wBindOperator & TH_APPEND )
			fp.SeekToEnd();
		for ( INT_PTR i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
			g_obMakeData[i]->WriteGcode(fp);
			_SetProgressPos64(i);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}
	_SetProgressPos(g_obMakeData.GetSize());

	// �o�͌�̏���
	if ( g_wBindOperator & TH_FOOTER && !s_strNCtemp.IsEmpty() ) {
		CString	strArgv("\""+GetStr(MKNC_STR_PERLSCRIPT)+"\" \""+s_strNCtemp+"\" \""+strNCFile+"\"");
		// Perl�����ċN��
		if ( !AfxGetNCVCMainWnd()->CreateOutsideProcess("perl.exe", strArgv, FALSE, TRUE) ) {
			AfxMessageBox(IDS_ERR_PERLSCRIPT, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
	}

	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC����Ҳݽگ��
//////////////////////////////////////////////////////////////////////

BOOL MakeNCD_MainFunc(CLayerData* pLayer)
{
#ifdef _DEBUG
	printf("MakeNCD_MainFunc() Start\n");
#endif
	CString	strLayer;
	if ( pLayer )
		strLayer = pLayer->GetLayerName();

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
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFchain*	pChain;
	INT_PTR		i, j, nLoop, nMapCnt = 0;

	if ( !g_pDoc->IsDocFlag(DXFDOC_SHAPE) ) {
		// �`��F��������p���Đ}�`�W�����쐬
		if ( !CreateShapeThread() )
			return FALSE;
	}

	// ���H�J�n�ʒu�w���ް��̾��
	if ( !g_bData && !g_obStartData.IsEmpty() )
		CDXFdata::ms_pData = g_obStartData.GetTail();

	// ̪���1
	switch ( GetNum(MKNC_NUM_DRILLPROCESS) ) {
	case 0:		// ��Ɍ����H
	case 2:		// �����H�̂�
		if ( !CallMakeDrill(NULL, CString()) )
			return FALSE;
		break;
	}
	if ( GetNum(MKNC_NUM_DRILLPROCESS) == 2 )
		return MakeLoopAddLastMove();	// �����H�݂̂Ȃ�A�����ŏI��

	// ̪���2
	// ���_������ϯ�߂̐����׸ނ�ر�A�`��W��ϯ�߂ɓo�^
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		nLoop  = pLayer->GetShapeSize();
		for ( j=0; j<nLoop && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			pChain = pShape->GetShapeChain();
			if ( pChain && pChain->GetCount()==1 && pChain->GetHead()->GetMakeType()==DXFPOINTDATA )
				pShape->SetShapeFlag(DXFMAPFLG_MAKE|DXFMAPFLG_SEARCH);
			else {
				pShape->OrgTuning();
				pShape->ClearMakeFlg();
			}
		}
		nMapCnt += nLoop;
	}
	if ( !IsThread() )
		return FALSE;
	if ( nMapCnt <= 0 )
		return TRUE;

	if ( !g_bData ) {
		// G����ͯ��(�J�n����)
		if ( g_wBindOperator & TH_HEADER )
			AddCustomMillCode(GetStr(MKNC_STR_HEADER), NULL);
		// ̧�ٖ��̺���
		if ( g_pDoc->IsDocFlag(DXFDOC_BIND) && AfxGetNCVCApp()->GetDXFOption()->GetDxfOptFlg(DXFOPT_FILECOMMENT) ) {
			CString	strBuf;
			strBuf.Format(IDS_MAKENCD_BINDFILE, g_pDoc->GetTitle());
			_AddMakeGdataStr(strBuf);
		}
		// ���H�J�n�ʒu�w��ڲԏ���
		AddMakeStart();
	}
	// ��]��
	CString	strSpindle( CNCMakeMill::MakeSpindle(DXFLINEDATA) );
	if ( !strSpindle.IsEmpty() )
		_AddMakeGdataStr(strSpindle);
	if ( !g_bData ) {
		// ���H�O�ړ��w���̐���
		MakeLoopAddFirstMove(MAKECUTTER);
	}

	// ���_�ɋ߂����Wϯ�߂�����
	pShape = GetNearPointShape(CDXFdata::ms_pData->GetEndCutterPoint()+CDXFdata::ms_ptOrg);
	if ( !pShape )
		return MakeLoopAddLastMove();

	// �ް�����
	g_bData = TRUE;
	SendFaseMessage(nMapCnt);

	// NC����ٰ��
	if ( !MakeLoopShape(pShape) )
		return FALSE;

	if ( GetNum(MKNC_NUM_DRILLPROCESS) == 1 ) {
		// ��Ō����H
		if ( !CallMakeDrill(NULL, CString()) )
			return FALSE;
	}

	// �؍��ް��I����̈ړ��w��ڲ�����
	return MakeLoopAddLastMove();
}

BOOL CreateShapeThread(void)
{
	NCVCTHREADPARAM	param;
	param.pParent = NULL;
	param.pDoc    = g_pDoc;
	param.wParam  = NULL;
	param.lParam  = NULL;

	// �`��F�������̽گ�ސ���
	CWinThread*	pThread = AfxBeginThread(ShapeSearch_Thread, &param,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if ( !pThread )
		return FALSE;
	pThread->m_bAutoDelete = FALSE;
	pThread->ResumeThread();
	::WaitForSingleObject(pThread->m_hThread, INFINITE);
	delete	pThread;

	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC����Ҳݽگ�ޕ⏕�֐��Q
//////////////////////////////////////////////////////////////////////

BOOL CallMakeDrill(CLayerData* pLayer, CString& strLayer)
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
	_AddMakeGdataCycleCancel();

	return TRUE;
}

BOOL CallMakeLoop(ENMAKETYPE enMake, CLayerData* pLayer, CString& strLayer)
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
		if ( g_wBindOperator & TH_HEADER )
			AddCustomMillCode(GetStr(MKNC_STR_HEADER), pData);
		// ̧�ٖ��̺���(-- BindMode�̎��́A���Ԃ񂱂��ɂ��Ȃ� --)
		if ( g_pDoc->IsDocFlag(DXFDOC_BIND) && AfxGetNCVCApp()->GetDXFOption()->GetDxfOptFlg(DXFOPT_FILECOMMENT) ) {
			CString	strBuf;
			strBuf.Format(IDS_MAKENCD_BINDFILE, g_pDoc->GetTitle());
			_AddMakeGdataStr(strBuf);
		}
		// ���H�J�n�ʒu�w��ڲԏ���
		AddMakeStart();
	}
	else {
		// �����H�ȊO�őO�̐؍�I���ʒu�Ǝ��̐؍�J�n�ʒu���Ⴄ�Ȃ�
		if ( CDXFdata::ms_pData->GetMakeType() != DXFPOINTDATA &&
				CDXFdata::ms_pData->GetEndMakePoint() != pData->GetStartMakePoint() ) {
			// Z���̏㏸
			_AddMoveGdataZ(0, g_dZReturn, -1.0f);
		}
	}

	// ڲԂ��Ƃ̺��ĂƏo�ͺ���
	if ( pLayer ) {
		// Multi Mode
		if ( !strLayer.IsEmpty() ) {
			// ���ď���
			if ( GetFlg(MKNC_FLG_LAYERCOMMENT) ) {
				strBuf = pLayer->GetLayerComment();
				if ( strBuf.IsEmpty() )
					strBuf.Format(IDS_MAKENCD_BREAKLAYER, strLayer);
				else
					strBuf = '(' + strBuf + ')';
				_AddMakeGdataStr(strBuf);
			}
			// �o�ͺ���
			strBuf = pLayer->GetLayerOutputCode();
			if ( !strBuf.IsEmpty() )
				_AddMakeGdataStr(strBuf);
			// �������Ă����Ȃ��悤��ڲԖ���ر
			strLayer.Empty();
		}
	}
	else if ( GetFlg(MKNC_FLG_LAYERCOMMENT) ) {
		// Single Mode
		for ( INT_PTR i=0; i<g_pDoc->GetLayerCnt(); i++ ) {
			pLayer = g_pDoc->GetLayerData(i);
			if ( pLayer->IsMakeTarget() ) {
				strBuf = pLayer->GetLayerName();
				strBuf.Format(IDS_MAKENCD_BREAKLAYER, strBuf);
				_AddMakeGdataStr(strBuf);
			}
		}
		pLayer = NULL;	// �ݸ�ق�NULL�ɂ��Ȃ��ƃo�O��
	}

	// ��]��
	strBuf = CNCMakeMill::MakeSpindle(pData->GetMakeType());
	if ( !strBuf.IsEmpty() )
		_AddMakeGdataStr(strBuf);

	// ���H�O�ړ��w���̐���
	if ( !bMatch && enMake!=MAKEDRILLCIRCLE ) {
		// OrgTuning()�Ō��݈ʒu�Ɠ�����W��������Ȃ�����
		CDXFdata* pDataMove = MakeLoopAddFirstMove(enMake);
		if ( pDataMove ) {
			// �ړ���ɋ߂��؍��ް����Č���
			if ( enMake == MAKECUTTER )
				pData = GetNearPointCutter(pLayer, pDataMove);
			else if ( enMake==MAKEDRILLPOINT && GetNum(MKNC_NUM_OPTIMAIZEDRILL)==0 )
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
				_AddMakeGdataStr(strBuf);
			}
		}
		break;
	}

	return bResult;
}

tuple<CDXFdata*, BOOL> OrgTuningCutter(const CLayerData* pLayerTarget)
{
#ifdef _DEBUG
	printf("OrgTuningCutter()\n");
	int			nDbg = -1;
	CLayerData*	pLayerDbg = NULL;
#endif
	INT_PTR	i, nCnt = 0, nLayerLoop, nDataLoop;
	BOOL	bMatch = FALSE, bCalc = !g_bData,
			bRound = GetNum(MKNC_NUM_CIRCLECODE) == 0 ? FALSE : TRUE;
	float	dGap, dGapMin = FLT_MAX;	// ���_�܂ł̋���
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	CDXFellipse*	pEllipse;
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
		pLayer = pLayerTarget ? const_cast<CLayerData *>(pLayerTarget) : g_pDoc->GetLayerData(nLayerLoop);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++, nCnt++ ) {
			pData = pLayer->GetDxfData(i);
			if ( pData->GetMakeType()!=DXFPOINTDATA && pData->IsMakeTarget() ) {	// �����̒Z���ް��͂����ŏ���
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
				// �ȉ~�ް��̒���
				if ( pData->GetMakeType() == DXFELLIPSEDATA ) {
					pEllipse = static_cast<CDXFellipse*>(pData);
					if ( !pEllipse->IsArc() ) {
						// �ȉ~�̐؍������ݒ�
						pEllipse->SetRoundFixed(bRound);
					}
				}
			}
			_SetProgressPos64(nCnt);
		}
	}

#ifdef _DEBUG
	if ( pLayerDbg ) {
		printf("FirstPoint Layer=%s Cnt=%d Gap=%f\n",
			LPCTSTR(pLayerDbg->GetLayerName()), nDbg, dGapMin);
	}
	else {
		printf("FirstPoint Cnt=%d Gap=%f\n", nDbg, dGapMin);
	}
#endif
	_SetProgressPos(nDataLoop);

	return make_tuple(pDataResult, bMatch);
}

tuple<CDXFdata*, BOOL> OrgTuningDrillPoint(void)
{
#ifdef _DEBUG
	printf("OrgTuningDrillPoint()\n");
#endif
	INT_PTR		i;
	const INT_PTR	nLoop = g_obDrill.GetSize();
	BOOL		bMatch = FALSE, bCalc;
	float		dGap, dGapMin = FLT_MAX;	// ���_�܂ł̋���
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;

	if ( g_obDrill.IsEmpty() )
		return make_tuple(pDataResult, bMatch);

	// ̪���1
	SendFaseMessage(nLoop);

	// ���_�����Ɛ؍�J�n�߲�Č���ٰ��
#ifdef _DEBUG
	int	nDbg = -1;
#endif

	// �؍�J�n�߲�Ă̌v�Z���K�v���ǂ���
	if ( g_bData )
		bCalc = FALSE;
	else
		bCalc = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? TRUE : FALSE;

	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = g_obDrill[i];
		// ���_�����Ƌ����v�Z + NC�����׸ނ̏�����
		dGap = pData->OrgTuning(bCalc);
		// ���_�Ɉ�ԋ߂��߲�Ă�T��
		if ( !bCalc ) {
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
		_SetProgressPos64(i);
	}
#ifdef _DEBUG
	printf("FirstPoint %d Gap=%f\n", nDbg, dGapMin);
#endif

	_SetProgressPos(nLoop);

	if ( !pDataResult )		// !bCalc
		pDataResult = g_obDrill[0];		// dummy

	return make_tuple(pDataResult, bMatch);
}

CDXFdata* OrgTuningDrillCircle(void)
{
#ifdef _DEBUG
	printf("OrgTuningDrillCircle()\n");
#endif
	if ( g_obCircle.IsEmpty() )
		return NULL;

	INT_PTR		i, j;
	const INT_PTR	nLoop = g_obCircle.GetSize();
	CPointF		pt;
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	SendFaseMessage(nLoop);

	// ̪���1 ���_����
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		g_obCircle[i]->OrgTuning(FALSE);	// ���בւ���̂ŋߐڌv�Z�͕s�v
		_SetProgressPos64(i);
	}
	_SetProgressPos(nLoop);
	// ̪���2 ���בւ�
	g_obCircle.Sort( GetNum(MKNC_NUM_DRILLSORT) == 0 ?
		CircleSizeCompareFunc1 : CircleSizeCompareFunc2 );	// �����E�~��

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
			_SetProgressPos64(i);
		}
		_SetProgressPos(nLoop);
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
	BOOL		bMove, bMakeHit, bCust;
	INT_PTR		i, nCnt, nPos = 0, nSetPos = 64;
	CPointF		ptKey;
	CDXFarray*	pobArray;

	i = GetPrimeNumber( g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER)*2 );
	mpEuler.InitHashTable((int)max(17, i));

	// GetNearPoint() �̌��ʂ� NULL �ɂȂ�܂�
	while ( pData && IsThread() ) {
		mpEuler.RemoveAll();
		// ���� pData ����_��
		mpEuler.SetMakePointMap(pData);
		pData->SetSearchFlg();
		// pData �̎n�_�E�I�_�ň�M�����T��
		for ( i=0; i<pData->GetPointNumber() && IsThread(); i++ ) {
			if ( !MakeLoopEulerSearch(pData->GetTunPoint(i), mpEuler) )
				return FALSE;
		}
		// CMap��޼ު�Ă�NC����
		if ( IsThread() ) {
			if ( (nCnt=MakeLoopEulerAdd(&mpEuler)) < 0 )
				return FALSE;
		}
		//
		nPos += nCnt;
		if ( nSetPos < nPos ) {
			_SetProgressPos(nPos);
			while ( nSetPos < nPos )
				nSetPos += nSetPos;
		}

		// �ړ��w��ڲԂ�����
		bCust = TRUE;
		bMove = bMakeHit = FALSE;
		while ( (pDataMove=GetMatchPointMove(CDXFdata::ms_pData)) && IsThread() ) {
			if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPALL)==0 ) {
				// �Ō��Z���ړ���ϰ��������΍폜
				if ( !g_ltDeepData.IsEmpty() && g_ltDeepData.GetTail() == NULL )
					g_ltDeepData.RemoveTail();
				// �ړ��ް��o�^
				g_ltDeepData.AddTail(pDataMove);
				pDataMove->SetMakeFlg();
			}
			else {
				bMove = TRUE;
				// �ړ��ް��O�̶��Ѻ��ޑ}��
				if ( bCust ) {
					g_pfnAddMoveCust_B();
					bCust = FALSE;	// �A�����ē���Ȃ��悤��
				}
				// �w�����ꂽZ�ʒu�ňړ�
				g_pfnAddMoveZ();
				// �ړ��ް��̐���
				_AddMakeMove(pDataMove);
			}
			// �ړ��ް���Ҕ�
			CDXFdata::ms_pData = pDataMove;
			// �ړ��ް��̏I�_�Ő؍��ް���˯Ă����
			ptKey = pDataMove->GetEndCutterPoint();
			if ( g_mpDXFdata.Lookup(ptKey, pobArray) ) {
				bMakeHit = TRUE;
				break;	// �؍��ް��D��̂��߈ړ��ް������𒆒f
			}
		}
		// �ړ��ް���̶��Ѻ��ޑ}��
		if ( bMove && IsThread() ) {
			// �ړ��ް��̏I�_��÷���ް��̐���
			AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
			// ���Ѻ���
			g_pfnAddMoveCust_A();
		}

		if ( bMakeHit ) {
			pData = NULL;
			// pobArray ���玟�̐؍��ް�������
			for ( i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
				pDataMove = pobArray->GetAt(i);
				if ( !pDataMove->IsMakeFlg() ) {
					pData = pDataMove;
					break;
				}
			}
			if ( !pData )
				bMakeHit = FALSE;	// ���̐؍��߲�Č���������
		}
		if ( !bMakeHit ) {		// else �ł̓_��
			// ���̐؍��߲�Č���
			if ( IsThread() )
				pData = GetNearPointCutter(pLayer, CDXFdata::ms_pData);
			// Z���̏㏸
			if ( pData && !GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_TOLERANCE)==0 )
				_AddMoveGdataZup();
		}

	} // End of while

	_SetProgressPos(nPos);

	// �S�̐[���̌㏈��
	if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPALL)==0 && IsThread() ) {
		if ( !MakeLoopDeepAdd() )
			return FALSE;
	}

	return IsThread();
}

BOOL MakeLoopEulerSearch(const CPointF& ptKey, CDXFmap& mpEuler)
{
	int			i, j;
	CPointF		pt;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// ���W�𷰂ɑS�̂�ϯ�ߌ���
	if ( g_mpDXFdata.Lookup(const_cast<CPointF&>(ptKey), pobArray) ) {
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

INT_PTR MakeLoopEulerAdd(const CDXFmap* mpEuler)
{
#ifdef _DEBUG
	printf("MakeLoopEulerAdd()\n");
#endif
	BOOL		bEuler = FALSE;		// ��M�����v���𖞂����Ă��邩
	CPointF		pt, ptNow(CDXFdata::ms_pData->GetEndCutterPoint());
	CDXFdata*	pData;
	CDXFarray*	pStartArray;
	CDXFlist	ltEuler;			// ��M������

#ifdef _DEBUGOLD
	CDumpContext	dc;
	dc.SetDepth(1);
	pEuler->Dump(dc);
#endif

	// ���̍��Wϯ�߂���M�����v���𖞂����Ă��邩
	tie(bEuler, pStartArray, pt) = mpEuler->IsEulerRequirement(ptNow);
	if ( !IsThread() )
		return -1;
#ifdef _DEBUG
	printf("FirstPoint x=%f y=%f EulerFlg=%d Cnt=%d\n", pt.x, pt.y, bEuler, mpEuler->GetCount());
#endif

	// --- ��M�����̐���(�ċA�Ăяo���ɂ��؍\�����)
	ASSERT( pStartArray );
	ASSERT( !pStartArray->IsEmpty() );
	if ( !MakeLoopEulerAdd_with_one_stroke(mpEuler, bEuler, pt, pStartArray, ltEuler) ) {
		if ( bEuler ) {
			// ��M�����ł���n�Y�₯�ǎ��s����������ɘa���Ă�蒼��
			bEuler = FALSE;
			MakeLoopEulerAdd_with_one_stroke(mpEuler, bEuler, pt, pStartArray, ltEuler);
		}
	}

	// --- �؍��ް�����
	ASSERT( !ltEuler.IsEmpty() );
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// �؍��ް�����
		PLIST_FOREACH(pData, &ltEuler)
			pData->SetMakeFlg();
		END_FOREACH
		g_ltDeepData.AddTail(&ltEuler);
		// �[�����S�̂��ۂ�
		if ( GetNum(MKNC_NUM_DEEPALL) == 0 ) {
			g_ltDeepData.AddTail((CDXFdata *)NULL);	// Z���ړ���ϰ��
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
		g_pfnAddMoveGdata(pData);
		// �؍��ް�����
		PLIST_FOREACH(pData, &ltEuler)
			_AddMakeGdataCut(pData);
		END_FOREACH
		// �Ō�ɐ��������ް���Ҕ�
		CDXFdata::ms_pData = pData;
	}

	// ��M�����v���𖞂����Ă��Ȃ��Ƃ�����
	if ( !bEuler )
		mpEuler->AllMapObject_ClearSearchFlg();	// ��M�����ɘR�ꂽ��޼ު�Ă̻���׸ނ�������

	return ltEuler.GetCount();
}

BOOL MakeLoopEulerAdd_with_one_stroke
	(const CDXFmap* mpEuler, BOOL bEuler, 
		const CPointF& ptEdge, const CDXFarray* pArray, CDXFlist& ltEuler)
{
	const INT_PTR	nLoop = pArray->GetSize();
	const CPointF	ptOrg(CDXFdata::ms_ptOrg);
	INT_PTR		i;
	CDXFdata*	pData;
	CDXFarray*	pNextArray;
	CPointF		pt, ptKey;
	POSITION	pos, posTail = ltEuler.GetTailPosition();	// ���̎��_�ł̉��o�^ؽĂ̍Ō�

	// �܂����̍��W�z��̉~(�ɏ�������)�ް������o�^
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() && pData->IsStartEqEnd() ) {
			pData->GetEdgeGap(ptEdge);	// ptEdge�l�ɋ߂������޼ު�Ă̎n�_�ɓ���ւ�
			ltEuler.AddTail( pData );
			pData->SetSearchFlg();
		}
	}

	// �~�ȊO���ް��Ŗ؍\���̎�������
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() ) {
			pData->GetEdgeGap(ptEdge);
			ltEuler.AddTail(pData);
			pData->SetSearchFlg();
			ptKey = pData->GetEndCutterPoint();
			if ( mpEuler->IsNativeKey() )
				ptKey += ptOrg;
			if ( !mpEuler->Lookup(ptKey, pNextArray) ) {
#ifdef _DEBUG
				printf("Name=%s\n", (LPCTSTR)pData->GetParentMap()->GetShapeName());
				printf("LookupKey not found=(%f, %f)\n", ptKey.x, ptKey.y);
				mpEuler->DbgDump();
#endif
				// Lookup() �ň���������Ȃ��ꍇ�́CKey���蓮�őS����
				BOOL	bMatch = FALSE;
				PMAP_FOREACH(pt, pNextArray, mpEuler)
					if ( ptKey.IsMatchPoint(&pt) ) {
						bMatch = TRUE;
						ptKey = pt;
						break;
					}
				END_FOREACH
				if ( !bMatch )
					NCVC_CriticalErrorMsg(__FILE__, __LINE__);	// �{���ɂȂ��H
			}
			// ���̍��W�z�������
			if ( mpEuler->IsNativeKey() )
				ptKey -= ptOrg;
			if ( MakeLoopEulerAdd_with_one_stroke(mpEuler, bEuler, ptKey, pNextArray, ltEuler) )
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
		if ( mpEuler->IsAllSearchFlg() )
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

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopShape(CDXFshape* pShape)
{
#ifdef _DEBUG
	printf("MakeLoopShape()\n");
#endif
	CDXFdata*	pData;
	const	CPointF		ptOrg(CDXFdata::ms_ptOrg);
			CPointF		pt;
	INT_PTR	nCnt, nPos = 0;

	while ( pShape && IsThread() ) {
#ifdef _DEBUG
		printf("ParentMapName=%s\n", LPCTSTR(pShape->GetShapeName()));
#endif
		// �`��W���̓������琶��
		pShape->SetShapeFlag(DXFMAPFLG_SEARCH);	// �e(�O��)�`��W���͌����ΏۊO
		if ( (nCnt=MakeLoopShapeSearch(pShape)) < 0 )
			return FALSE;
		if ( pShape->IsOutlineList() )
			pData = NULL;
		else {
			// �e�`��W�����猻�݈ʒu�ɋ߂���޼ު�Ă�����
			pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
			pShape->GetSelectObjectFromShape(pt, NULL, &pData);	// �߂�l�s�v
		}
		// �e�`��W�����g�̐���
		if ( !MakeLoopShapeAdd(pShape, pData) )
			return FALSE;
		// ��۸�ڽ�ް�̍X�V
		nPos += nCnt+1;
		_SetProgressPos(nPos);
		// ���̌`��W��������
		pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		pShape = GetNearPointShape(pt);		// Ȳè�ލ��W�Ō���
	}

	// �S�̐[���̌㏈��
	if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPALL)==0 && IsThread() ) {
		if ( !MakeLoopDeepAdd() )
			return FALSE;
	}

	return IsThread();
}

INT_PTR MakeLoopShapeSearch(const CDXFshape* pShapeBase)
{
#ifdef _DEBUG
	printf("MakeLoopShapeSearch()\n");
#endif

	INT_PTR	i, j, nCnt = 0;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CShapeArray	obShape;	// �����̌`��W���ꗗ
	CRectF	rc( pShapeBase->GetMaxRect() ),
			rcBase(rc.TopLeft().RoundUp(), rc.BottomRight().RoundUp());
	obShape.SetSize(0, 64);

	// pShapeBase�������̋�`�����`��W��������
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		for ( j=0; j<pLayer->GetShapeSize() && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			rc = pShape->GetMaxRect();
			if ( !pShape->IsMakeFlg() && !pShape->IsSearchFlg() &&
					pShape->GetShapeAssemble()!=DXFSHAPE_EXCLUDE &&
					rcBase.PtInRect(rc.TopLeft().RoundUp()) && rcBase.PtInRect(rc.BottomRight().RoundUp()) ) {
				pShape->SetShapeFlag(DXFMAPFLG_SEARCH);
				obShape.Add(pShape);
				nCnt += MakeLoopShapeSearch(pShape);
			}
		}
	}

	if ( obShape.IsEmpty() || !IsThread() )
		return 0;

	// obShape�ɒ~�ς��ꂽ�ް��̐���
	const	CPointF		ptOrg(CDXFdata::ms_ptOrg);
			CPointF		pt(CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg);
	float		dGap, dGapMin;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	CDXFshape*	pShapeResult;

	pShapeResult = obShape[0];
	j = obShape.GetSize();

	while ( pShapeResult && IsThread() ) {
		dGapMin = FLT_MAX;
		pShapeResult = NULL;
		// pt�Ɉ�ԋ߂�Ȳè�ނ̌`��W��������
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
			// �`��W����NC����
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
#ifdef _DEBUGOLD
	printf("MakeLoopShapeAdd() MapName=%s\n", LPCTSTR(pShape->GetShapeName()));
#endif
	if ( pShape->IsMakeFlg() )	// �����֊s���ŁA���ɐ����ς݂̏ꍇ������
		return TRUE;

	pShape->SetShapeFlag(DXFMAPFLG_MAKE);

	// �����̕���
	CDXFchain*	pChain = pShape->GetShapeChain();
	if ( pChain ) {
		if ( !pShape->IsOutlineList() )
			return MakeLoopShapeAdd_ChainList(pShape, pChain, pData);
	}
	else
		return MakeLoopShapeAdd_EulerMap(pShape);

	// �֊s����
	int		i;
	BOOL	bResult = TRUE;
	COutlineData	obMerge;
	COutlineList*	pOutlineList = pShape->GetOutlineList();
	CDXFworkingOutline*	pOutline;
	obMerge.SetSize(0, 64);

	// �`��ɑ�����S�Ă̗֊s��޼ު�Ă𓝍�
	PLIST_FOREACH(pOutline, pOutlineList)
		for ( i=0; i<pOutline->GetOutlineSize() && IsThread(); i++ ) {
			pChain = pOutline->GetOutlineObject(i);
			pChain->OrgTuning();
			obMerge.Add(pChain);
		}
	END_FOREACH

	// ���������֊s��޼ު�Ă�ʐςŕ��בւ�
	obMerge.Sort(AreaSizeCompareFunc);

	// NC����
	for ( i=0; i<obMerge.GetSize() && bResult && IsThread(); i++ ) {
		pChain  = obMerge[i];
		bResult = MakeLoopShapeAdd_ChainList(pShape, pChain, NULL);
	}

	return bResult;
}

BOOL MakeLoopShapeAdd_ChainList(CDXFshape* pShape, CDXFchain* pChain, CDXFdata* pData)
{
	POSITION	pos1, pos2;
	BOOL		bReverse = FALSE, bNext = FALSE, bDirection = FALSE;

	if ( pData ) {
		// pShape ���֊s��޼ު�Ă������Ȃ��ꍇ
		// -- ���H�w���ɔ����J�n�ʒu��������̐ݒ�
		pos1 = pos2 = pShape->GetFirstChainPosition();
	}
	else {
		CPointF	ptNow;
		// pShape ���֊s��޼ު�Ă����ꍇ
		CDXFworking*	pWork;
		tie(pWork, pData) = pShape->GetDirectionObject();
		if ( pData ) {
			pShape->GetFirstChainPosition();	// pShape �̕����w����������
			bReverse = pShape->GetShapeFlag() & DXFMAPFLG_REVERSE;
			bNext    = pShape->GetShapeFlag() & DXFMAPFLG_NEXTPOS;
			bDirection = TRUE;
		}
		tie(pWork, pData) = pShape->GetStartObject();
		if ( pData ) {
			ptNow = static_cast<CDXFworkingStart*>(pWork)->GetStartPoint() - CDXFdata::ms_ptOrg;
		}
		else {
			ptNow = CDXFdata::ms_pData->GetEndCutterPoint();
		}
		pData = NULL;
		float	dGap1, dGap2;
		if ( pChain->IsLoop() ) {
			if ( pChain->GetCount() == 1 ) {
				pData = pChain->GetHead();
				pData->GetEdgeGap(ptNow);	// �֊s��޼ު�Ă̋ߐڍ��W�v�Z
			}
			else {
				// ���݈ʒu�ɋ߂���޼ު�Ă���J�n
				pChain->GetSelectObjectFromShape(ptNow+CDXFdata::ms_ptOrg, NULL, &pData);
				dGap1 = GAPCALC(pData->GetStartCutterPoint() - ptNow);
				dGap2 = GAPCALC(pData->GetEndCutterPoint()   - ptNow);
				if ( dGap1 > dGap2 ) {
					if ( bDirection )
						bNext = bReverse ? FALSE : TRUE;
					else
						bReverse = TRUE;
				}
			}
		}
		else {
			// �[�_�ɋ߂�������J�n
			dGap1 = GAPCALC(pChain->GetHead()->GetStartCutterPoint() - ptNow);
			dGap2 = GAPCALC(pChain->GetTail()->GetEndCutterPoint()   - ptNow);
			if ( dGap1 > dGap2 ) {
				bReverse = TRUE;
				pData = pChain->GetTail();
			}
			else
				pData = pChain->GetHead();
		}
		// -- ���H�w���ɔ����J�n�ʒu��������̐ݒ�
		ASSERT(pData);
		pos1 = pos2 = pChain->SetLoopFunc(pData, bReverse, bNext);
	}
	ASSERT(pos1);

	// �؍��ް�����
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// �J�n�߼޼�݂���ٰ��
		do {
			pData = pChain->GetSeqData(pos1);
			g_ltDeepData.AddTail(pData);
			if ( !pos1 )
				pos1 = pChain->GetFirstPosition();
		} while ( pos1!=pos2 && IsThread() );
		// �[�����S�̂��ۂ�
		if ( GetNum(MKNC_NUM_DEEPALL) == 0 ) {
			// �Ō�ɐ��������ް���Ҕ�
			CDXFdata::ms_pData = pData;
			// Z���ړ���ϰ��
			g_ltDeepData.AddTail((CDXFdata *)NULL);
		}
		else
			return MakeLoopDeepAdd();
	}
	else {
		if ( bNext ) {
			// pData�̍X�V
			POSITION	pos = pos1;
			pData = pChain->GetSeqData(pos);
		}
		// �؍��ް��܂ł̈ړ�
		g_pfnAddMoveGdata(pData);
		// �J�n�߼޼�݂���ٰ��
		do {
			pData = pChain->GetSeqData(pos1);
			_AddMakeGdataCut(pData);
			if ( !pos1 )
				pos1 = pChain->GetFirstPosition();
		} while ( pos1!=pos2 && IsThread() );
		// �Ō�ɐ��������ް���Ҕ�
		CDXFdata::ms_pData = pData;
		// Z���㏸
		_AddMoveGdataZup();
	}

	return IsThread();
}

BOOL MakeLoopShapeAdd_EulerMap(CDXFshape* pShape)
{
	BOOL		bEuler = FALSE;
	CDXFmap*	mpEuler = pShape->GetShapeMap();
	ASSERT( mpEuler );
	// �P��ڂ̐�������
	if ( !MakeLoopShapeAdd_EulerMap_Make( pShape, mpEuler, bEuler ) )
		return FALSE;
	if ( bEuler )
		return TRUE;	// �P��ڂőS�Đ�������

	// �����R����ް�����
	int			i;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	CDXFarray*	pArray;
	CDXFmap		mpLeak;
	float		dGap, dGapMin;
	CPointF		pt, ptKey;
	while ( IsThread() ) {
		// ���݈ʒu�ɋ߂���޼ު�Č���
		pDataResult = NULL;
		dGapMin = FLT_MAX;
		pt = CDXFdata::ms_pData->GetEndCutterPoint();
		PMAP_FOREACH(ptKey, pArray, mpEuler)
			for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
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
		END_FOREACH
		if ( !pDataResult || !IsThread() )	// ٰ�ߏI������
			break;
		// �����Wϯ�߂𐶐�
		mpLeak.RemoveAll();
		mpLeak.SetPointMap(pDataResult);	// SetMakePointMap() �ł͂Ȃ�
		pDataResult->SetSearchFlg();
		for ( i=0; i<pDataResult->GetPointNumber() && IsThread(); i++ ) {
			pt = pDataResult->GetNativePoint(i);
			if ( !MakeLoopShapeAdd_EulerMap_Search(pt, mpEuler, &mpLeak) )
				return FALSE;
		}
		if ( !IsThread() )
			return FALSE;
		// �Q��ڈȍ~�̐�������
		bEuler = FALSE;
		if ( !MakeLoopShapeAdd_EulerMap_Make( pShape, &mpLeak, bEuler ) )
			return FALSE;
	}

	return IsThread();
}

BOOL MakeLoopShapeAdd_EulerMap_Make(CDXFshape* pShape, CDXFmap* mpEuler, BOOL& bEuler)
{
	// MakeLoopEulerAdd() �Q�l
	BOOL		bReverse = FALSE;
	POSITION	pos;
	CPointF		pt;
	CDXFdata*		pData;
	CDXFdata*		pDataFix;
	CDXFarray*		pArray;
	CDXFworking*	pWork;
	CDXFchain		ltEuler;
	const	CPointF	ptOrg(CDXFdata::ms_ptOrg);

	// �J�n�ʒu�w��
	tie(pWork, pDataFix) = pShape->GetStartObject();
	const	CPointF		ptNow( pDataFix ?
		static_cast<CDXFworkingStart*>(pWork)->GetStartPoint() :	// ���݈ʒu���X�V
		CDXFdata::ms_pData->GetEndCutterPoint()+ptOrg );

	// ���̍��Wϯ�߂���M�����v���𖞂����Ă��邩�A���A
	// ���݈ʒu�i���H�J�n�ʒu�j�ɋ߂��Ƃ��납��ꎞ�W���𐶐�
	tie(bEuler, pArray, pt) = mpEuler->IsEulerRequirement(ptNow);
	if ( !IsThread() )
		return FALSE;
	ASSERT( pArray );
	ASSERT( !pArray->IsEmpty() );

	// --- ��M�����̐���(�ċA�Ăяo���ɂ��؍\�����)
	pt -= ptOrg;	// TunPoint -> MakePoint
	if ( !MakeLoopEulerAdd_with_one_stroke(mpEuler, bEuler, pt, pArray, ltEuler) ) {
		if ( bEuler ) {
			// ��M�����ł���n�Y�₯�ǎ��s����������ɘa���Ă�蒼��
			bEuler = FALSE;
			MakeLoopEulerAdd_with_one_stroke(mpEuler, bEuler, pt, pArray, ltEuler);
		}
	}
	ASSERT( !ltEuler.IsEmpty() );

	// �����w������ѐ�����������
	tie(pWork, pDataFix) = pShape->GetDirectionObject();
	if ( pDataFix ) {
		// �����w����ltEuler�Ɋ܂܂��ꍇ��������
		PLIST_FOREACH(pData, &ltEuler)
			if ( pDataFix == pData ) {
				CPointF	pts( static_cast<CDXFworkingDirection*>(pWork)->GetStartPoint() - ptOrg ),
						pte( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() - ptOrg );
				bReverse = pData->IsDirectionPoint(pts, pte);
				break;
			}
		END_FOREACH
	}

	// ���������̐ݒ�
	ltEuler.SetLoopFunc(NULL, bReverse, FALSE);

	// --- �؍��ް�����
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// �؍��ް�����
		for ( pos=ltEuler.GetFirstPosition(); pos && IsThread(); ) {
			pData = ltEuler.GetSeqData(pos);
			if ( pData )
				pData->SetMakeFlg();
		}
		g_ltDeepData.AddTail(&ltEuler);
		// �[�����S�̂��ۂ�
		if ( GetNum(MKNC_NUM_DEEPALL) == 0 ) {
			// �Ō�ɐ��������ް���Ҕ�
			CDXFdata::ms_pData = g_ltDeepData.GetTail();
			// Z���ړ���ϰ��
			g_ltDeepData.AddTail((CDXFdata *)NULL);
		}
		else
			return MakeLoopDeepAdd();
	}
	else {
		BOOL	bNext = FALSE;
		// �؍��ް��܂ł̈ړ�
		pData = ltEuler.GetFirstData();
		g_pfnAddMoveGdata(pData);
		// �؍��ް�����
		for ( pos=ltEuler.GetFirstPosition(); pos && IsThread(); ) {
			pData = ltEuler.GetSeqData(pos);
			if ( pData ) {
				if ( bNext ) {
					_AddMoveGdataZup();
					g_pfnAddMoveGdata(pData);
					bNext = FALSE;
				}
				_AddMakeGdataCut(pData);
			}
			else
				bNext = TRUE;
		}
		// �Ō�ɐ��������ް���Ҕ�
		CDXFdata::ms_pData = pData;
		// Z���㏸
		_AddMoveGdataZup();
	}

	return IsThread();
}

BOOL MakeLoopShapeAdd_EulerMap_Search
	(const CPointF& ptKey, CDXFmap* pOrgMap, CDXFmap* pResultMap)
{
	int			i, j;
	CPointF		pt;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// ���W�𷰂ɑS�̂�ϯ�ߌ���
	if ( pOrgMap->Lookup(const_cast<CPointF&>(ptKey), pobArray) ) {
		for ( i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() && !pData->IsSearchFlg() ) {
				pResultMap->SetPointMap(pData);		// SetMakePointMap() �ł͂Ȃ�
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
	BOOL		bAction = TRUE,		// �܂��͐�����
				bEndZApproach = GetNum(MKNC_NUM_DEEPROUND)==0;	// �I�_�ł̃A�v���[�`�v�Z��[����]�̂�
	float		dZCut = g_dZCut;	// g_dZCut�ޯ�����
	CDXFdata*	pData;

	// �Ō��Z���ړ�ϰ��͍폜
	if ( g_ltDeepData.GetTail() == NULL )
		g_ltDeepData.RemoveTail();
	if ( g_ltDeepData.IsEmpty() )
		return TRUE;

	// �[������
	if ( GetNum(MKNC_NUM_DEEPALL) == 0 ) {
		// [�S��]
		// İ�ٌ���*�[���ï�߂���۸�ڽ���۰ق̍Đݒ�(�[���ï�߶��Ă͐؂�グ)
		nCnt = (int)(ceil(fabs((g_dDeep - g_dZCut) / GetDbl(MKNC_DBL_ZSTEP)))
			* g_ltDeepData.GetCount());
		SendFaseMessage( nCnt );
	}
	else {
		// [��M]
		// Head��Tail�̃}�[�L���O
		g_ltDeepData.GetHead()->SetDxfFlg(DXFFLG_EDGE);
		g_ltDeepData.GetTail()->SetDxfFlg(DXFFLG_EDGE);
	}

#ifdef _DEBUGOLD
	int	n;
	printf("LayerName=%s\n", LPCTSTR(g_ltDeepData.GetHead()->GetParentLayer()->GetLayerName()));
	PLIST_FOREACH(pData, &g_ltDeepData)
		if ( pData )
			n = pData->GetParentLayer()->IsCutType() ? 1 : 2;
		else
			n = 0;
		printf("ListType=%d\n", n);
	END_FOREACH
#endif

	// ��]��
	CString	strSpindle( CNCMakeMill::MakeSpindle(DXFLINEDATA) );
	if ( !strSpindle.IsEmpty() )
		_AddMakeGdataStr(strSpindle);
	// �؍��ް��܂ł̈ړ�
	g_pfnAddMoveGdata( g_ltDeepData.GetHead() );

	nCnt  = 0;
	pData = g_ltDeepData.GetHead();
	// �[���ŏI�ʒu�܂ŉ��o�^�ް���NC����
	if ( GetNum(MKNC_NUM_DEEPALL)==1 && GetFlg(MKNC_FLG_HELICAL) &&
			g_ltDeepData.GetCount()==1 && pData->GetMakeType()==DXFCIRCLEDATA ) {
		// �~�ް����ضِ؍�
		// g_dZCut==g_dDeep�܂��ضقŗ��Ƃ��̂�
		// ������RoundUp(g_dZCut-g_dDeep)>0��
		while ( RoundUp(g_dZCut-g_dDeep)>0 && IsThread() ) {
			g_dZCut = max(g_dZCut+GetDbl(MKNC_DBL_ZSTEP), g_dDeep);
			_AddMakeGdataHelical(pData);
		}
	}
	else {
		// �[����������
		// g_dZCut > g_dDeep �ł̏����ł͐��l�덷�����������Ƃ�ٰ�ߒE�o���Ȃ�����
		// ������RoundUp(g_dZCut-g_dDeep)>NCMIN
		while ( RoundUp(g_dZCut-g_dDeep)>NCMIN && IsThread() ) {
			pData = g_pfnDeepProc(bAction, FALSE, bEndZApproach);
			CDXFdata::ms_pData = pData;
			// Z���̉��~
			g_dZCut = max(g_dZCut+GetDbl(MKNC_DBL_ZSTEP), g_dDeep);
			if ( bEndZApproach && _IsZApproach(pData) ) {
				// pData�̏I�_��3���؍�
				CPoint3F	pt3d(pData->GetEndMakePoint(), g_dZCut);
				_AddMakeGdata3dCut(pt3d);
			}
			else {
				if ( pData->GetParentLayer()->IsCutType() && RoundUp(g_dZCut-g_dDeep)>NCMIN ) {
					// ����ʍs�؍������
					MakeLoopDeepZDown();
				}
			}
			// ����݂̐؂�ւ�(�����؍�̂�)
			if ( bEndZApproach ) { // GetNum(MKNC_NUM_DEEPROUND) == 0
				bAction = !bAction;
				// �e��޼ު�Ă̎n�_�I�_�����ւ�
				PLIST_FOREACH(pData, &g_ltDeepData)
					if ( pData && pData->GetMakeType()!=DXFCIRCLEDATA ) {
						// �~�ް��ȊO�̍��W����ւ�
						if ( pData->GetMakeType() == DXFELLIPSEDATA ) {
							CDXFellipse* pEllipse = static_cast<CDXFellipse*>(pData);
							if ( pEllipse->IsArc() )
								pEllipse->SwapMakePt(0);
							else
								pEllipse->SetRoundFixed(!pEllipse->GetRound());
						}
						else
							pData->SwapMakePt(0);
					}
				END_FOREACH
			}
			// ��۸�ڽ�ް�̍X�V
			if ( GetNum(MKNC_NUM_DEEPALL) == 0 )
				_SetProgressPos(++nCnt * g_ltDeepData.GetCount());
		}
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
			_AddMakeGdataStr( CNCMakeMill::MakeSpindle(DXFLINEDATA, TRUE) );
			// ��޼ު�Đ؍�ʒu�ֈړ�
			pData = bAction ? g_ltDeepData.GetHead() : g_ltDeepData.GetTail();
			g_pfnAddMoveGdata(pData);
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
	CDXFdata::ms_pData = g_pfnDeepProc(bAction, bFinish, FALSE);

	// �[���؍�ɂ�����Z���̏㏸
	MakeLoopDeepZUp();

	// ��n��
	g_ltDeepData.RemoveAll();
	g_dZCut = dZCut;

	return IsThread();
}

CDXFdata* MakeLoopDeepAdd_Euler(BOOL bAction, BOOL bDeepFin, BOOL bApproach)
{
	BOOL		bIgnoreFirstEdge = FALSE;	// �擪��Edge�͖���
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
	for ( POSITION pos=(g_ltDeepData.*pfnGetPosition)(); pos && IsThread(); ) {
		pData = (g_ltDeepData.*pfnGetData)(pos);
		if ( bApproach && bIgnoreFirstEdge && pData->IsEdgeFlg() && _IsZApproach(pData) ) {
			// �A�v���[�`�I�_�܂Ő؍�
			CPointF	pts(pData->GetStartCutterPoint()),
					pte(pData->GetEndCutterPoint());
			_AddMakeGdataApproach(pte, pts, 1, GetDbl(MKNC_DBL_FEED));
			pData->SetMakeFlg();
		}
		else
			_AddMakeGdataDeep(pData, bDeepFin);
		bIgnoreFirstEdge = TRUE;
	}
/*
	CDXFdata*	pData;
	POSITION	pos;
	function<POSITION ()>				pfnGetPosition;
	function<CDXFdata*& (POSITION&)>	pfnGetData;
	if ( bAction ) {
		pfnGetPosition	= bind(&CDXFlist::GetHeadPosition, &g_ltDeepData);
		// ��GetNext()�ƌ^����v���Ȃ�...�Ȃ�ŁH
		pfnGetData		= bind(&CDXFlist::GetNext, &g_ltDeepData, _1);
	}
	else {
		pfnGetPosition	= bind(&CDXFlist::GetTailPosition, &g_ltDeepData);
//		pfnGetData		= bind(&CDXFlist::GetPrev, &g_ltDeepData, _1);
	}
	// �ް�����ٰ��(���]�t�])
	for ( pos=pfnGetPosition(); pos && IsThread(); ) {
		pData = pfnGetData(pos);
		_AddMakeGdataDeep(pData, bDeep);
	}
*/
	return pData;
}

CDXFdata* MakeLoopDeepAdd_All(BOOL bAction, BOOL bDeepFin, BOOL bApproach)
{
	POSITION	(CDXFlist::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFlist::*pfnGetData)(POSITION&);
	CDXFdata*&	(CDXFlist::*pfnGetEnd)(void);
	if ( bAction ) {
		pfnGetPosition	= &(CDXFlist::GetHeadPosition);
		pfnGetData		= &(CDXFlist::GetNext);
		pfnGetEnd		= &(CDXFlist::GetTail);
	}
	else {
		pfnGetPosition	= &(CDXFlist::GetTailPosition);
		pfnGetData		= &(CDXFlist::GetPrev);
		pfnGetEnd		= &(CDXFlist::GetHead);
	}
	BOOL		bMove = FALSE, bBreak = FALSE;
	CDXFdata*	pData;
	CDXFdata*	pDataEnd = (g_ltDeepData.*pfnGetEnd)();
	CDXFdata*	pDataResult = NULL;

	// �ް�����ٰ��(���]�t�])
	for ( POSITION pos=(g_ltDeepData.*pfnGetPosition)(); pos && IsThread(); ) {
		// ��޼ު�Ď��o��
		pData = (g_ltDeepData.*pfnGetData)(pos);
		if ( pData ) {
			if ( pData->GetParentLayer()->IsCutType() ) {
				// �؍��ް�
				if ( bBreak ) {
					if ( !bMove )	// �ړ��Ȃ����
						MakeLoopDeepZUp();
					// pData�܂ňړ�(Z�����~����)
					g_pfnAddMoveGdata(pData);
				}
				if ( bMove ) {
					// �ړ��ް�������
					_AddMoveGdataZdown();
					bMove = FALSE;
				}
				// ���X�g�̍Ō��Z���i���A�v���[�`���K�v���ǂ���
				if ( bApproach && pData==pDataEnd && _IsZApproach(pData) ) {
					// �A�v���[�`�I�_�܂Ő؍�
					CPointF	pts(pData->GetStartCutterPoint()),
							pte(pData->GetEndCutterPoint());
					_AddMakeGdataApproach(pte, pts, 1, GetDbl(MKNC_DBL_FEED));
					pData->SetMakeFlg();
				}
				else {
					_AddMakeGdataDeep(pData, bDeepFin);
				}
			}
			else {
				// �ړ��ް�
				ASSERT( !bBreak );
				if ( !bMove )
					MakeLoopDeepZUp();
				_AddMakeMove(pData);
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
	const CDXFdata* pDataHead = g_ltDeepData.GetHead();
	const CDXFdata* pDataTail = g_ltDeepData.GetTail();

	// �����؍킩��A�̵�޼ު�Ă���ٰ�߂Ȃ�
	if ( GetNum(MKNC_NUM_DEEPROUND)==0 ||
				pDataHead->GetStartMakePoint()==pDataTail->GetEndMakePoint() ) {
		// ���̐[�����W�ցCZ���̍~���̂�
		_AddMoveGdataZ(1, g_dZCut, GetDbl(MKNC_DBL_ZFEED));
	}
	else {
		// ����ʍs�؍�̏ꍇ
		// �܂�Z���̏㏸
		MakeLoopDeepZUp();
		// �擪��޼ު�ĂɈړ�
		g_pfnAddMoveGdata(pDataHead);
	}
}

void MakeLoopDeepZUp(void)
{
	if ( GetNum(MKNC_NUM_DEEPRETURN) == 0 ) {
		// �������Z�����A
		_AddMoveGdataZup();
	}
	else {
		// R�_�܂Ő؍푗���Z�����A
		_AddMoveGdataZ(1, g_dZG0Stop, GetDbl(MKNC_DBL_MAKEENDFEED));
		// �Ƽ�ٓ_���A�Ȃ�
		if ( GetNum(MKNC_NUM_ZRETURN) == 0 )
			_AddMoveGdataZup();
	}
}

BOOL MakeLoopDrillPoint(CDXFdata* pData)
{
	int		nAxis = GetNum(MKNC_NUM_OPTIMAIZEDRILL);

	if ( nAxis == 0 )	// �D�掲�Ȃ�
		return MakeLoopAddDrill(pData);

	INT_PTR	i, nCnt, nIndex, nMatch, nProgress = 0;
	BOOL	bMatch, bMove, bCust;
	CDrillAxis	obAxis;	// ���W��
	CDXFsort*	pAxis;
	CDXFdata*	pDataDummy;
	CDXFdata*	pDataMove;

	nAxis = nAxis - 1;	// X=0, Y=1
	obAxis.SetSize(0, g_obDrill.GetSize());

	// ���W���̐���
	MakeLoopDrillPoint_MakeAxis(nAxis, obAxis);

	// �؍��ް��̐���
	while ( IsThread() ) {
		bCust = TRUE;
		bMove = FALSE;
		// ���݈ʒu�ɋ߂����W��������
		tie(nIndex, nMatch) = GetNearPointDrillAxis(CDXFdata::ms_pData, 1-nAxis, obAxis);
		if ( nIndex < 0 )
			break;		// �I������
		pAxis = obAxis[nIndex];
		if ( nMatch < 0 ) {
			// ���̊J�n�ʒu�ƊJ�n�����𒲐�
			nMatch = MakeLoopDrillPoint_EdgeChk(nAxis, pAxis);
		}
		else if ( nMatch > 0 ) {	// ��ۂ������s�v
			// ��v������޼ު�Ă̑O��ǂ���ɋ߂���
			nMatch = MakeLoopDrillPoint_SeqChk(nMatch, nAxis, pAxis);
		}
		tie(nCnt, pDataMove) = MakeLoopAddDrillSeq(nProgress, nMatch, pAxis);
		nProgress += nCnt;
		// �ړ��ް�����
		while ( pDataMove && IsThread() ) {
			// �ړ��ް��̂P�ڂ̏I�[�Ɍ����H�ް���Hit
			tie(pDataDummy, bMatch) = GetNearPointDrill(pDataMove);	// g_obDrill�S�̂��猟��
			if ( bMatch ) {
				// �ړ�ڲ��ް��͏����������Ƃ�
				pDataMove->SetMakeFlg();
				CDXFdata::ms_pData = pDataMove;
				break;
			}
			// �Œ軲�ٷ�ݾ�
			if ( !bMove && !GetFlg(MKNC_FLG_L0CYCLE) )
				_AddMakeGdataCycleCancel();
			// �ړ��׸�ON
			bMove = TRUE;
			// �ړ��ް��O�̶��Ѻ��ޑ}��
			if ( bCust ) {
				g_pfnAddMoveCust_B();
				bCust = FALSE;
			}
			if ( GetFlg(MKNC_FLG_L0CYCLE) ) {
				// �ړ��ް��̐���(L0�t��)
				_AddMakeMove(pDataMove, TRUE);
			}
			else {
				// �w�����ꂽZ�ʒu�ňړ�
				g_pfnAddMoveZ();
				// �ړ��ް��̐���
				_AddMakeMove(pDataMove);
			}
			CDXFdata::ms_pData = pDataMove;
			// ���̈ړ��ް�������
			pDataMove = GetMatchPointMove(pDataMove);
		}
		// �ړ��ް��̌㏈��
		if ( bMove && IsThread() ) {
			// �ړ��ް��̏I�_��÷���ް��̐���
			AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
			// ���Ѻ���
			g_pfnAddMoveCust_A();
		}
	}

	// ��ЂÂ�
	for ( i=0; i<obAxis.GetSize(); i++ )
		delete	obAxis[i];

	return IsThread();
}

void MakeLoopDrillPoint_MakeAxis(int nAxis, CDrillAxis& obAxis)
{
	CDXFsort*	pAxis;
	INT_PTR		i = 0;
	const INT_PTR	nLoop = g_obDrill.GetSize();
	float		dBase, dMargin = fabs(GetDbl(MKNC_DBL_DRILLMARGIN)) * 2.0f;
	CDXFsort::PFNCOMPARE	pfnCompare;
	
	// ����ŕ��בւ�
	if ( nAxis == 0 ) {
		g_obDrill.Sort(DrillOptimaizeCompareFuncY);
		pfnCompare = DrillOptimaizeCompareFuncX;
	}
	else {
		g_obDrill.Sort(DrillOptimaizeCompareFuncX);
		pfnCompare = DrillOptimaizeCompareFuncY;
	}

	nAxis = 1 - nAxis;

	// ����ٰ�߂̐���
	while ( i < nLoop && IsThread() ) {
		dBase = g_obDrill[i]->GetEndCutterPoint()[nAxis] + dMargin;
		pAxis = new CDXFsort;
		while ( i < nLoop &&
				g_obDrill[i]->GetEndCutterPoint()[nAxis] <= dBase && IsThread() ) {
			pAxis->Add(g_obDrill[i++]);
		}
		pAxis->Sort(pfnCompare);
		obAxis.Add(pAxis);
	}
}

INT_PTR MakeLoopDrillPoint_EdgeChk(int nAxis, CDXFsort* pAxis)
{
	INT_PTR		i, nFirst = -1, nLast = -1;
	const INT_PTR	nLoop = pAxis->GetSize();
	CPointF		pts, pte,
				ptNow(CDXFdata::ms_pData->GetEndCutterPoint());
	CDXFdata*	pData;

	// �擪�̖������ް�������
	for ( i=0; i<nLoop; i++ ) {
		pData = pAxis->GetAt(i);
		if ( !pData->IsMakeFlg() ) {
			pts = pData->GetEndCutterPoint();
			nFirst = i;
			break;
		}
	}
	ASSERT( nFirst >= 0 );

	// �����̖������ް�������
	for ( i=nLoop-1; i>=0; i-- ) {
		pData = pAxis->GetAt(i);
		if ( !pData->IsMakeFlg() ) {
			pte = pData->GetEndCutterPoint();
			nLast = i;
			break;
		}
	}

	// ���݈ʒu����̂ǂ��炪�߂���(�����ł͂Ȃ������)
	if ( nFirst!=nLast &&
			fabs(pts[nAxis] - ptNow[nAxis]) > fabs(pte[nAxis] - ptNow[nAxis]) ) {
		pAxis->Reverse();
		nFirst = nLoop - nLast - 1;	// �t���ɂ����Ƃ��̐V�����J�n�ʒu
	}
	
	return nFirst;
}

INT_PTR MakeLoopDrillPoint_SeqChk(INT_PTR nMatch, int nAxis, CDXFsort* pAxis)
{
	INT_PTR			i, nFirst = -1, nLast = -1, nCntF = 0, nCntL = 0;
	const INT_PTR	nLoop = pAxis->GetSize();
	CPointF		pts, pte,
				ptNow(pAxis->GetAt(nMatch)->GetEndCutterPoint());
	CDXFdata*	pData;

	// �擪������
	for ( i=nMatch-1; i>=0; i-- ) {
		pData = pAxis->GetAt(i);
		if ( !pData->IsMakeFlg() ) {
			if ( nFirst < 0 ) {
				pts = pData->GetEndCutterPoint();
				nFirst = i;
			}
			nCntF++;	// �擪�����ւ̎c�茏��
		}
	}
	if ( nFirst < 0 )
		return nMatch;		// ��������ւ��s�v

	// ����������
	for ( i=nMatch+1; i<nLoop; i++ ) {
		pData = pAxis->GetAt(i);
		if ( !pData->IsMakeFlg() ) {
			if ( nLast < 0 ) {
				pte = pData->GetEndCutterPoint();
				nLast = i;
			}
			nCntL++;	// ���������ւ̎c�茏��
		}
	}
	if ( nLast < 0 ) {
		pAxis->Reverse();	// �����������ް�����
		nMatch = nLoop - nMatch - 1;	// �t���ɂ����Ƃ��̐V�����J�n�ʒu
		return nMatch;
	}

	float	dGap = fabs(pts[nAxis]-ptNow[nAxis]) - fabs(pte[nAxis]-ptNow[nAxis]);

	if ( fabs(dGap) < NCMIN ) {
		// �O��̋������������̂Ŏc�茏���̑�������
		if ( nCntF > nCntL ) {
			pAxis->Reverse();
			nMatch = nLoop - nMatch - 1;
		}
	}
	else if ( dGap < 0 ) {	// pts < pte
		// �擪�����̕����߂�
		pAxis->Reverse();
		nMatch = nLoop - nMatch - 1;
	}

	return nMatch;
}

BOOL MakeLoopDrillCircle(void)
{
	INT_PTR			i;
	const INT_PTR	nLoop = g_obCircle.GetSize();
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	BOOL		bMatch;
	float		r, dGap, dGapMin;
	CString		strBreak;

	// ٰ�ߊJ�n
	g_obDrill.SetSize(0, nLoop);
	for ( i=0; i<nLoop && IsThread(); ) {
		g_obDrill.RemoveAll();
		// �~���ٰ��(���a)���Ƃɏ���
		r = g_obCircle[i]->GetMakeR();
		bMatch = FALSE;
		pDataResult = NULL;
		dGapMin = FLT_MAX;
		for ( ; i<nLoop && r==g_obCircle[i]->GetMakeR() && IsThread(); i++ ) {
			pData = g_obCircle[i];
			if ( !pData->IsMakeFlg() ) {
				g_obDrill.Add(pData);
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
				strBreak.Format(IDS_MAKENCD_CIRCLEBREAK,
					static_cast<CDXFcircle*>(pDataResult)->GetMakeR());
				_AddMakeGdataStr(strBreak);
			}
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

	// �������Ă����Ȃ��ƁA�u�~�ް����Ɂv�̂Ƃ���
	// ���̎��_�����H���ް������������
	g_obDrill.RemoveAll();

	return IsThread();
}

BOOL MakeLoopAddDrill(CDXFdata* pData)
{
	CDXFdata*	pDataMove;
	INT_PTR		nPos = 0;
	BOOL		bMatch,
				bMove = FALSE,		// �ړ�ڲ�Hit
				bCust = TRUE;		// �ړ��ް��O�̶��Ѻ��ޑ}��

	// �ް�����
	_AddMakeGdataDrill(pData);
	CDXFdata::ms_pData = pData;

	// GetNearPoint() �̌��ʂ� NULL �ɂȂ�܂�
	while ( IsThread() ) {
		// ���̗v�f�Ɉ�ԋ߂��v�f
		tie(pData, bMatch) = GetNearPointDrill(CDXFdata::ms_pData);
		if ( !pData )
			break;
		if ( !bMatch ) {
			// �ړ��w��ڲԂ�����
			if ( (pDataMove=GetMatchPointMove(CDXFdata::ms_pData)) ) {
				// �ړ�ڲԂP�ڂ̏I�[�Ɍ����H�ް���Hit�����
				tie(pData, bMatch) = GetNearPointDrill(pDataMove);
				if ( bMatch ) {
					// �ړ�ڲ��ް��͏����������Ƃɂ��āC���̌����H�ް��𐶐�
					pDataMove->SetMakeFlg();
				}
				else {
					bMove = TRUE;
					// �Œ軲�ٷ�ݾ�
					if ( !GetFlg(MKNC_FLG_L0CYCLE) )
						_AddMakeGdataCycleCancel();
					// �ړ��ް��O�̶��Ѻ��ޑ}��
					if ( bCust ) {
						g_pfnAddMoveCust_B();
						bCust = FALSE;
					}
					if ( GetFlg(MKNC_FLG_L0CYCLE) ) {
						// �ړ��ް��̐���(L0�t��)
						_AddMakeMove(pDataMove, TRUE);
					}
					else {
						// �w�����ꂽZ�ʒu�ňړ�
						g_pfnAddMoveZ();
						// �ړ��ް��̐���
						_AddMakeMove(pDataMove);
					}
					CDXFdata::ms_pData = pDataMove;
					continue;	// �ĒT��
				}
			}
		}
		bCust = TRUE;
		// �ړ��ް���̶��Ѻ��ޑ}��
		if ( bMove && IsThread() ) {
			// �ړ��ް��̏I�_��÷���ް��̐���
			AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
			// ���Ѻ���
			g_pfnAddMoveCust_A();
			bMove = FALSE;
		}
		// �ް�����
		_AddMakeGdataDrill(pData);
		CDXFdata::ms_pData = pData;
		_SetProgressPos64(++nPos);
	} // End of while

	_SetProgressPos(nPos);

	return IsThread();
}

tuple<INT_PTR, CDXFdata*> MakeLoopAddDrillSeq(INT_PTR nProgress, INT_PTR nStart, CDXFsort* pAxis)
{
	INT_PTR		i, nCnt = 0;
	CDXFdata*	pData;
	CDXFdata*	pDataMove = NULL;

	for ( i=nStart; i<pAxis->GetSize() && IsThread(); i++ ) {
		pData = pAxis->GetAt(i);
		if ( pData->IsMakeFlg() )
			continue;
		// �ް�����
		_AddMakeGdataDrill(pData);
		_SetProgressPos64(i+nProgress);
		nCnt++;
		// �ړ�ڲԂ�����
		if ( (pDataMove=GetMatchPointMove(pData)) )
			break;
	}
	// �Ō�ɐ��������ް���Ҕ�
	CDXFdata::ms_pData = pData;

	return make_tuple(nCnt, pDataMove);
}

CDXFdata* MakeLoopAddFirstMove(ENMAKETYPE enType)
{
	CDXFarray*	pobArray;
	CDXFdata*	pDataMove;
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	BOOL		bMatch, bCust = FALSE;
	CPointF		pt;

	// �ړ��w��ڲԂ�����
	while ( (pDataMove=GetMatchPointMove(CDXFdata::ms_pData)) && IsThread() ) {
		if ( !bCust ) {
			// �ړ��ް��O�̶��Ѻ��ޑ}��(�P�񂾂�)
			g_pfnAddMoveCust_B();
			bCust = TRUE;
		}
		// �w�����ꂽZ�ʒu�ňړ�
		g_pfnAddMoveZ();
		// �ړ��ް��̐���
		_AddMakeMove(pDataMove);
		CDXFdata::ms_pData = pDataResult = pDataMove;
		// ������W�Ő؍��ް���������� break
		if ( enType == MAKECUTTER ) {
			pt = pDataMove->GetEndCutterPoint();
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
			tie(pData, bMatch) = GetNearPointDrill(pDataMove);
			if ( bMatch ) {
				pDataResult = pData;
				break;
			}
		}
	}
	// �ړ��ް���̶��Ѻ��ޑ}��
	if ( bCust && IsThread() ) {
		// �ړ��ް��̏I�_��÷���ް��̐���
		AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
		// ���Ѻ���
		g_pfnAddMoveCust_A();
	}

	return pDataResult;
}

BOOL MakeLoopAddLastMove(void)
{
	CDXFdata*	pDataMove;
	BOOL		bCust = FALSE;

	// �I�_���W�ł̺��Đ���
	CPointF	pt( CDXFdata::ms_pData->GetEndCutterPoint() );
	AddCutterTextIntegrated(pt);	// �؍�ڲ�
	AddMoveTextIntegrated(pt);		// �ړ�ڲ�

	// �Ō�̈ړ����ނ�����
	while ( (pDataMove=GetMatchPointMove(CDXFdata::ms_pData)) && IsThread() ) {
		if ( !bCust ) {
			g_pfnAddMoveCust_B();
			bCust = TRUE;
		}
		g_pfnAddMoveZ();
		_AddMakeMove(pDataMove);
		CDXFdata::ms_pData = pDataMove;
	}
	if ( bCust && IsThread() ) {
		AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
		g_pfnAddMoveCust_A();
	}

	return IsThread();
}

CDXFdata* GetNearPointCutter(const CLayerData* pLayerTarget, const CDXFdata* pDataTarget)
{
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	CLayerData*	pLayer;
	INT_PTR		i, nLayerLoop = pLayerTarget ? 1 : g_pDoc->GetLayerCnt();
	float		dGap, dGapMin = FLT_MAX;

	while ( nLayerLoop-- > 0 && IsThread() ) {
		pLayer = pLayerTarget ? const_cast<CLayerData*>(pLayerTarget) : g_pDoc->GetLayerData(nLayerLoop);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++ ) {
			pData = pLayer->GetDxfData(i);
			if ( !pData->IsMakeFlg() && pData->GetMakeType()!=DXFPOINTDATA && pData->IsMakeTarget() ) {
				dGap = pData->GetEdgeGap(pDataTarget);
				if ( dGap < dGapMin ) {
					pDataResult = pData;
					dGapMin = dGap;
				}
			}
		}
	}

	return pDataResult;
}

CDXFshape* GetNearPointShape(const CPointF& pt)
{
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFshape*	pShapeResult = NULL;
	INT_PTR		i, j, nLoop1 = g_pDoc->GetLayerCnt(), nLoop2;
	float		dGap, dGapMin = FLT_MAX;

	for ( i=0; i<nLoop1 && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		nLoop2 = pLayer->GetShapeSize();
		for ( j=0; j<nLoop2 && IsThread(); j++ ) {
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

tuple<CDXFdata*, BOOL> GetNearPointDrill(const CDXFdata* pDataTarget)
{
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;
	BOOL	bMatch = FALSE;
	float	dGap, dGapMin = FLT_MAX;	// �w��_�Ƃ̋���

	// ���݈ʒu�Ɠ������C�܂��͋߂��v�f������
	for ( int i=0; i<g_obDrill.GetSize() && IsThread(); i++ ) {
		pData = g_obDrill[i];
		if ( pData->IsMakeFlg() )
			continue;
		// �������f
		if ( pData->IsMakeMatchObject(pDataTarget) ) {
			pDataResult = pData;
			bMatch = TRUE;
			break;
		}
		// ���݈ʒu�Ƃ̋����v�Z
		dGap = pData->GetEdgeGap(pDataTarget);
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

tuple<int, int> GetNearPointDrillAxis(const CDXFdata* pDataTarget, int nAxis, CDrillAxis& obAxis)
{
	CDXFsort*	pAxis;
	CDXFdata*	pData;
	int		i, j, nMatch = -1, nResult = -1;
	float	dGap, dGapMin = FLT_MAX;
	CPointF	pt1(pDataTarget->GetEndMakePoint()), pt2;

	for ( i=0; i<obAxis.GetSize() && nMatch<0 && IsThread(); i++ ) {
		pAxis = obAxis[i];
		for ( j=0; j<pAxis->GetSize(); j++ ) {
			pData = pAxis->GetAt(j);
			if ( pData->IsMakeFlg() )
				continue;
			if ( pData->IsMakeMatchPoint(pt1) ) {	// pData->IsMakeMatchObject(pDataTarget)
				nResult = i;
				nMatch  = j;		// �O��ٰ�߂�break
				break;
			}
			pt2 = pData->GetEndMakePoint();
			dGap = fabs(pt2[nAxis] - pt1[nAxis]);	// �w�莲�̋����Ŕ��f
			if ( dGap < dGapMin ) {
				nResult = i;
				dGapMin = dGap;
			}
		}
	}

	return make_tuple(nResult, nMatch);
}

CDXFdata* GetMatchPointMove(const CDXFdata* pDataTarget)
{
	// ���݈ʒu�Ɠ������v�f����������
	CPointF	pt( pDataTarget->GetEndCutterPoint() );
	CDXFarray*	pobArray;
	CDXFdata*	pDataResult = NULL;
	
	if ( g_mpDXFmove.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pData->GetEdgeGap(pt);	// �n�_�𒲐�
				pDataResult = pData;
				break;
			}
		}
	}

	return pDataResult;
}

//	AddCustomMillCode() ����Ăяo��
class CMakeCustomCode_Mill : public CMakeCustomCode	// MakeCustomCode.h
{
public:
	CMakeCustomCode_Mill(const CDXFdata* pData) :
				CMakeCustomCode(g_pDoc, pData, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "G92_INITIAL", "G92X", "G92Y", "G92Z",
			"SPINDLE", "G0XY_INITIAL"
		};
		// ���ް�ǉ�
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
	}

	CString	ReplaceCustomCode(const string& str) {
		extern	const	DWORD	g_dwSetValFlags[];
		int		nTestCode;
		float	dValue[VALUESIZE];
		CString	strResult;

		// ���׽�Ăяo��
		tie(nTestCode, strResult) = CMakeCustomCode::ReplaceCustomCode(str);
		if ( !strResult.IsEmpty() )
			return strResult;

		// �h��replace
		switch ( nTestCode ) {
		case 0:		// ProgNo
			if ( GetFlg(MKNC_FLG_PROG) )
				strResult.Format(IDS_MAKENCD_PROG,
					GetFlg(MKNC_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKNC_NUM_PROG));
			break;
		case 1:		// G90orG91
			strResult = CNCMakeBase::MakeCustomString(GetNum(MKNC_NUM_G90)+90);
			break;
		case 2:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			dValue[NCA_Z] = GetDbl(MKNC_DBL_G92Z);
			strResult = CNCMakeBase::MakeCustomString(92, NCD_X|NCD_Y|NCD_Z, dValue, FALSE);
			break;
		case 3:		// G92X
		case 4:		// G92Y
		case 5:		// G92Z
			nTestCode -= 3;
			dValue[nTestCode] = GetDbl(MKNC_DBL_G92X+nTestCode);
			strResult = CNCMakeBase::MakeCustomString(-1, g_dwSetValFlags[nTestCode], dValue, FALSE);
			break;
		case 6:		// SPINDLE
			if ( m_pData )					// Header
				strResult = CNCMakeMill::MakeSpindle(m_pData->GetMakeType());
			else if ( CDXFdata::ms_pData )	// Footer
				strResult = CNCMakeMill::MakeSpindle(CDXFdata::ms_pData->GetMakeType());
			break;
		case 7:		// G0XY_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			strResult = CNCMakeBase::MakeCustomString(0, NCD_X|NCD_Y, dValue);
			break;
		default:
			strResult = str.c_str();
		}

		return strResult;
	}
};

void AddCustomMillCode(const CString& strFileName, const CDXFdata* pData)
{
	CString	strBuf, strResult;
	CMakeCustomCode_Mill	custom(pData);
	string	str, strTok;
	tokenizer<custom_separator>	tokens(str);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) && IsThread() ) {
			str = strBuf;
			tokens.assign(str);
			strResult.Empty();
			BOOST_FOREACH(strTok, tokens) {
				strResult += custom.ReplaceCustomCode(strTok);
			}
			if ( !strResult.IsEmpty() )
				_AddMakeGdataStr(strResult);
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// ���̴װ�͐�������(�x���̂�)
	}
}

// ���H�J�n�w���ް��̐���
void AddMakeStart(void)
{
	// �@�B���_�ł�÷���ް�����
	AddStartTextIntegrated(CPointF());	// (0, 0)
	
	CDXFdata*	pData = NULL;
	CNCMakeMill*	pNCD;
	// ������������Ă΂�Ȃ��̂� IsMakeFlg() �̔��f�͕K�v�Ȃ�
	for ( int i=0; i<g_obStartData.GetSize() && IsThread(); i++ ) {
		pData = g_obStartData[i];
		// �J�n�ʒu�Ɠ�����÷���ް��̐���
		AddStartTextIntegrated(pData->GetStartCutterPoint());
		// �ړ��w��
		pNCD = new CNCMakeMill(pData, FALSE);
		ASSERT( pNCD );
		g_obMakeData.Add(pNCD);
	}
	if ( pData ) {
		// �Ō�̐����ް�
		CDXFdata::ms_pData = pData;
		// �I���ʒu�Ɠ�����÷���ް��̐���
		AddStartTextIntegrated(pData->GetEndCutterPoint());
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
	if ( CNCMakeMill::ms_xyz[NCA_Z] < g_dZG0Stop )
		_AddMoveGdataZ(0, g_dZG0Stop, -1);
}

void AddMoveZ_Initial(void)
{
	// Z���̌��݈ʒu���Ƽ�ٓ_��菬����(�Ⴂ)�Ȃ�
	if ( CNCMakeMill::ms_xyz[NCA_Z] < g_dZInitial )
		_AddMoveGdataZ(0, g_dZInitial, -1);
}

void AddMoveCust_B(void)
{
	_AddMakeGdataStr(GetStr(MKNC_STR_CUSTMOVE_B));
}

void AddMoveCust_A(void)
{
	_AddMakeGdataStr(GetStr(MKNC_STR_CUSTMOVE_A));
}

void AddMoveGdataG0(const CDXFdata* pData)
{
	// G0�ړ��ް��̐���
	_AddMoveGdata(0, pData, 0);
	// Z���̉��~
	_AddMoveGdataZdown();
}

void AddMoveGdataG1(const CDXFdata* pData)
{
	// Z���̉��~
	_AddMoveGdataZdown();
	// G1�ړ��ް��̐���
	_AddMoveGdata(1, pData, GetDbl(MKNC_DBL_FEED));
}

void AddMoveGdataApproach(const CDXFdata* pData)
{
	if ( !_IsZApproach(pData) )
		return AddMoveGdataG0(pData);

	// �A�v���[�`�̊J�n�ʒu��G00�ړ�
	CPointF	pts(pData->GetStartCutterPoint()), 
			pte(pData->GetEndCutterPoint());
	_AddMakeGdataApproach(pts, pte, 0);
	// Z���̌��݈ʒu��R�_���傫��(����)�Ȃ�R�_�܂ő�����
	if ( CNCMakeMill::ms_xyz[NCA_Z] > g_dZG0Stop )
		_AddMoveGdataZ(0, g_dZG0Stop, -1.0f);
	// ���H�ςݐ[���ւ�Z���؍�ړ�
	float	dZValue = _GetZValue();
	if ( CNCMakeMill::ms_xyz[NCA_Z] > dZValue )
		_AddMoveGdataZ(1, dZValue, GetDbl(MKNC_DBL_MAKEENDFEED));
	// pData�̎n�_��3���؍�
	CPoint3F	pt3d(pData->GetStartMakePoint(), g_dZCut);
	_AddMakeGdata3dCut(pt3d);
}

// ÷�ď��̐���
void AddCommentText(const CPointF& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFcomment.Lookup(const_cast<CPointF&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				_AddMakeGdataStr( '(' + static_cast<CDXFtext*>(pData)->GetStrValue() + ')' );
				pData->SetMakeFlg();
				break;
			}
		}
	}
}

void AddStartTextIntegrated(const CPointF& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFstarttext.Lookup(const_cast<CPointF&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				_AddMakeText(pData);
				return;
			}
		}
	}
}

void AddMoveTextIntegrated(const CPointF& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFmovetext.Lookup(const_cast<CPointF&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				_AddMakeText(pData);
				return;
			}
		}
	}
}

void AddCutterTextIntegrated(const CPointF& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFtext.Lookup(const_cast<CPointF&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				_AddMakeText(pData);
				return;
			}
		}
	}
}

// ̪��ޏo��
void SendFaseMessage
	(INT_PTR nRange/*=-1*/, int nMsgID/*=-1*/, LPCTSTR lpszMsg/*=NULL*/)
{
#ifdef _DEBUG
	printf("MakeNCD_Thread() Phase%d Start\n", g_nFase);
#endif
	if ( nRange > 0 )
		g_pParent->m_ctReadProgress.SetRange32(0, (int)nRange);

	CString	strMsg;
	if ( nMsgID > 0 )
		VERIFY(strMsg.LoadString(nMsgID));
	else
		strMsg.Format(IDS_MAKENCD_FASE, g_nFase);
	g_pParent->SetFaseMessage(strMsg, lpszMsg);
	g_nFase++;
}

//////////////////////////////////////////////////////////////////////
// ���בւ��⏕�֐�
//////////////////////////////////////////////////////////////////////

int CircleSizeCompareFunc1(CDXFcircle* pFirst, CDXFcircle* pSecond)
{
	int		nResult;
	float	dResult = pFirst->GetMakeR() - pSecond->GetMakeR();
	if ( dResult == 0.0f )
		nResult = 0;
	else if ( dResult > 0.0f )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int CircleSizeCompareFunc2(CDXFcircle* pFirst, CDXFcircle* pSecond)
{
	int		nResult;
	float	dResult = pSecond->GetMakeR() - pFirst->GetMakeR();
	if ( dResult == 0.0f )
		nResult = 0;
	else if ( dResult > 0.0f )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int DrillOptimaizeCompareFuncX(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	float	dResult = pFirst->GetEndCutterPoint().x - pSecond->GetEndCutterPoint().x;
	if ( dResult == 0.0f )
		nResult = 0;
	else if ( dResult > 0.0f )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int DrillOptimaizeCompareFuncY(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	float	dResult = pFirst->GetEndCutterPoint().y - pSecond->GetEndCutterPoint().y;
	if ( dResult == 0.0f )
		nResult = 0;
	else if ( dResult > 0.0f )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int AreaSizeCompareFunc(CDXFchain* pFirst, CDXFchain* pSecond)
{
	int		nResult;
	CRectF	rc1(pFirst->GetMaxRect()), rc2(pSecond->GetMaxRect());
	float	dResult = rc1.Width() * rc1.Height() - rc2.Width() * rc2.Height();
	if ( dResult == 0.0f )
		nResult = 0;
	else if ( dResult > 0.0f )
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
	printf("MakeNCD_AfterThread() Start\n");
#endif
	for ( int i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	g_obMakeData.RemoveAll();
	g_mpDXFdata.RemoveAll();
	g_mpDXFstarttext.RemoveAll();
	g_mpDXFmove.RemoveAll();
	g_mpDXFmovetext.RemoveAll();
	g_mpDXFtext.RemoveAll();
	g_mpDXFcomment.RemoveAll();
	g_obStartData.RemoveAll();
	g_ltDeepData.RemoveAll();
	g_obDrill.RemoveAll();
	g_obCircle.RemoveAll();

	g_csMakeAfter.Unlock();

	return 0;
}
