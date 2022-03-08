// TH_MakeWire.cpp
//		���C�����d���H�@�pNC����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFOption.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCMakeWireOpt.h"
#include "NCMakeWire.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DBG_NCMAKE_TIME	//	�������Ԃ̕\��
#endif

using std::string;
using namespace boost;

// ��۰��ٕϐ���`
static	CThreadDlg*			g_pParent;
static	CDXFDoc*			g_pDoc;
static	CNCMakeWireOpt*		g_pMakeOpt;
static	int					g_nFase;	// ̪��އ�

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)

// NC�����ɕK�v���ް��Q
static	CSortArray<CObArray, CDXFcircle*>
					g_obAWFinside, g_obAWFoutside;	// AWF�ް�(CDXFcircle*)
static	CDXFmap		g_mpPause;		// �ꎞ��~�w���_
static	CTypedPtrArrayEx<CPtrArray, CNCMakeWire*>	g_obMakeData;	// ���H�ް�

static	WORD	g_wBindOperator;	// Bind����̧�ُo�͎w��
static	BOOL	g_bAWF;				// AWF�ڑ���

// ��ފ֐�
static	void	InitialVariable(void);		// �ϐ�������
static	void	SetStaticOption(void);		// �ÓI�ϐ��̏�����
static	BOOL	MakeWire_MainFunc(void);	// NC������Ҳ�ٰ��
static	BOOL	MakeLoopWire(CDXFshape*);
static	INT_PTR	MakeLoopWireSearch(CDXFshape*, int);
static	BOOL	MakeLoopWireAdd(CDXFshape*, CDXFshape*, BOOL);
static	BOOL	MakeLoopWireAdd_Hetero(CDXFshape*, CDXFshape*);
static	BOOL	MakeLoopWireAdd_ChainList(CDXFshape*, CDXFchain*);
static	BOOL	MakeLoopWireAdd_EulerMap(CDXFshape*);
static	BOOL	MakeLoopWireAdd_EulerMap_Make(CDXFshape*, CDXFmap*, BOOL&);
static	BOOL	MakeLoopWireAdd_EulerMap_Search(const CPointF&, CDXFmap*, CDXFmap*);
static	BOOL	MakeLoopWireAdd_with_one_stroke(const CDXFmap*, BOOL, BOOL, const CPointF&, const CDXFarray*, CDXFlist&);
static	CDXFcircle*	SetAWFandPAUSEdata(void);	// AWF�ƈꎞ��~�߲�Ă̓o�^����
static	BOOL	CreateShapeThread(void);	// �`��F������
static	BOOL	OutputWireCode(void);		// NC���ނ̏o��

// �����Ŏw�肵����޼ު�ĂɈ�ԋ߂���޼ު�Ă�Ԃ�
static	CDXFshape*	GetNearPointWire(const CPointF&);
static	CDXFcircle*	GetInsideAWF(const CDXFshape*);
static	CDXFcircle*	GetOutsideAWF(void);

// ͯ�ް,̯�ް���̽�߼�ٺ��ސ���
static	void	AddCustomWireCode(const CString&);

// �C���ް��̐���
static inline	void	_AddMakeWireStr(const CString& strData)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(strData);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// AWF����
static inline	void	_AddAWFconnect(void)
{
	if ( !g_bAWF ) {
		_AddMakeWireStr(GetStr(MKWI_STR_AWFCNT));
		g_bAWF = TRUE;
	}
}
// AWF�ؒf
static inline	void	_AddAWFcut(void)
{
	if ( g_bAWF ) {
		_AddMakeWireStr(GetStr(MKWI_STR_AWFCUT));
		g_bAWF = FALSE;
	}
}
// AWF�߲�Ăւ̈ړ�(G00)
static inline	void	_AddMoveAWFpoint(CDXFcircle* pCircle)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(0, pCircle->GetMakeCenter(),
			0.0f, GetDbl(MKWI_DBL_TAPER));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pCircle->SetMakeFlg();
	_AddAWFconnect();
}
// �؍��ް��ւ̈ړ�(G01)
static inline	void	_AddMoveGdata(const CDXFdata* pData)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(1, pData->GetStartMakePoint(),
			GetDbl(MKWI_DBL_FEED), GetDbl(MKWI_DBL_TAPER));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// �؍��ް��ւ̈ړ�(G01)
static inline	void	_AddMoveGdata(const CDXFdata* pDataXY, const CDXFdata* pDataUV)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(
			pDataXY->GetStartMakePoint(), pDataUV->GetStartMakePoint(),
			GetDbl(MKWI_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// �؍��ް�
static inline	void	_AddMakeGdata(CDXFdata* pData)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(pData, GetDbl(MKWI_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pData->SetMakeFlg();
}
// �؍��ް��i�㉺�ٌ`��j
static inline	void	_AddMakeGdata(CDXFdata* pDataXY, CDXFdata* pDataUV)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(pDataXY, pDataUV, GetDbl(MKWI_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pDataXY->SetMakeFlg();
	pDataUV->SetMakeFlg();
}
// �؍��ް��i�㉺�ٌ`����א����j
static inline	void	_AddMakeGdata(const CVPointF& vptXY, const CVPointF& vptUV)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(vptXY, vptUV, GetDbl(MKWI_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

// ��޽گ�ފ֐�
static	CCriticalSection	g_csMakeAfter;	// MakeWire_AfterThread()�گ��ۯ���޼ު��
static	UINT	MakeWire_AfterThread(LPVOID);	// ��n���گ��

//////////////////////////////////////////////////////////////////////
// ���C�����d���H�@�pNC�����گ��
//////////////////////////////////////////////////////////////////////

UINT MakeWire_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeWire_Thread() Start\n");
#endif
#ifdef _DBG_NCMAKE_TIME
	// ���ݎ������擾
	CTime t1 = CTime::GetCurrentTime();
#endif
	int		nResult = IDCANCEL;

	// ��۰��ٕϐ�������
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);
	int i = (int)(pParam->lParam);
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
	SendFaseMessage(g_pParent, g_nFase, -1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		// NC������߼�ݵ�޼ު�Ă̐����Ƶ�߼�݂̓ǂݍ���
		g_pMakeOpt = new CNCMakeWireOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKEWIRE)->GetHead());
		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		{
			optional<CPointF>	ptResult = g_pDoc->GetCutterOrigin();
			CDXFdata::ms_ptOrg = ptResult ? *ptResult : 0.0f;
		}
		g_pDoc->GetCircleObject()->OrgTuning(FALSE);	// ���ʓI�Ɍ��_����ۂɂȂ�
		InitialVariable();
		// �������Ƃɕω��������Ұ���ݒ�
		SetStaticOption();
		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
		// �������蓖��
		g_obMakeData.SetSize(0, 1024);
		// �����J�n
		BOOL bResult = MakeWire_MainFunc();
		if ( bResult )
			bResult = OutputWireCode();

		// �߂�l���
		if ( bResult && IsThread() )
			nResult = IDOK;
#ifdef _DEBUG
		printf("MakeWire_Thread All Over!!!\n");
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
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��

	// ��������NC���ނ̏����گ��(�D��x��������)
	AfxBeginThread(MakeWire_AfterThread, NULL,
		THREAD_PRIORITY_IDLE);

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	if ( !(g_wBindOperator & TH_APPEND) )
		CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	CNCMakeWire::InitialVariable();		// CNCMakeBase::InitialVariable()
}

void SetStaticOption(void)
{
	g_bAWF = GetFlg(MKWI_FLG_AWFSTART);

	// CDXFdata�̐ÓI�ϐ�������
	CDXFdata::ms_fXRev = CDXFdata::ms_fYRev = FALSE;

	// CNCMakeWire�̐ÓI�ϐ�������
	CNCMakeWire::ms_xyz[NCA_X] = 0.0f;
	CNCMakeWire::ms_xyz[NCA_Y] = 0.0f;
	CNCMakeWire::ms_xyz[NCA_Z] = 0.0f;

	// ������߼�݂ɂ��ÓI�ϐ��̏�����
	CNCMakeWire::SetStaticOption(g_pMakeOpt);
}

BOOL OutputWireCode(void)
{
	CString	strPath, strFile,
			strNCFile(g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_pParent, g_nFase, g_obMakeData.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		UINT	nOpenFlg = CFile::modeCreate | CFile::modeWrite |
			CFile::shareExclusive | CFile::typeText | CFile::osSequentialScan;
		if ( g_wBindOperator & TH_APPEND )
			nOpenFlg |= CFile::modeNoTruncate;
		CStdioFile	fp(strNCFile, nOpenFlg);
		if ( g_wBindOperator & TH_APPEND )
			fp.SeekToEnd();
		for ( INT_PTR i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
			g_obMakeData[i]->WriteGcode(fp);
			SetProgressPos(g_pParent, i+1);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	SetProgressPos(g_pParent, g_obMakeData.GetSize());
	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC����Ҳݽگ��
//////////////////////////////////////////////////////////////////////

BOOL MakeWire_MainFunc(void)
{
	if ( !g_pDoc->IsDocFlag(DXFDOC_SHAPE) ) {
		// �`��F��������p���Đ}�`�W�����쐬
		if ( !CreateShapeThread() )
			return FALSE;
	}

	INT_PTR		i, j, nLoop;
	CDXFcircle*	pCircle;
	CLayerData*	pLayer;
	CDXFshape*	pShape;

	// ���_������ϯ�߂̐����׸ނ�ر
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		nLoop = pLayer->GetShapeSize();
		for ( j=0; j<nLoop && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			pShape->OrgTuning();
			pShape->ClearMakeFlg();
		}
	}

	// AWF�߲��
	pCircle = SetAWFandPAUSEdata();

	// AWF�ɋ߂����Wϯ�߂�����
	CPointF	pt( pCircle ? pCircle->GetCenter() :
			(CDXFdata::ms_pData->GetEndCutterPoint() + CDXFdata::ms_ptOrg) );
	pShape = GetNearPointWire(pt);

	// NC����ٰ��
	if ( !MakeLoopWire(pShape) )
		return FALSE;

	if ( pShape ) {
		// AWF�ؒf
		if ( GetFlg(MKWI_FLG_AWFEND) )
			_AddAWFcut();
		// G����̯��(�I������)
		if ( g_wBindOperator & TH_FOOTER )
			AddCustomWireCode(GetStr(MKWI_STR_FOOTER));
	}

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopWire(CDXFshape* pShape)
{
#ifdef _DEBUG
	printf("MakeLoopWire()\n");
#endif
	INT_PTR		nCnt, nPos = 0;

	while ( pShape && IsThread() ) {
#ifdef _DEBUG
		printf("ParentMapName=%s\n", LPCTSTR(pShape->GetShapeName()));
#endif
		// �`��W���̓������琶��
		pShape->SetShapeFlag(DXFMAPFLG_SEARCH);	// �e(�O��)�`��W���͌����ΏۊO
		if ( (nCnt=MakeLoopWireSearch(pShape, 0)) < 0 )
			return FALSE;
		// �e�`��W�����g�̐���
		if ( !MakeLoopWireAdd(pShape, NULL, TRUE) )
			return FALSE;
		// ��۸�ڽ�ް�̍X�V
		nPos += nCnt+1;
		SetProgressPos(g_pParent, nPos);
		// ���̌`��W��������
		pShape = GetNearPointWire(CDXFdata::ms_pData->GetEndCutterPoint() + CDXFdata::ms_ptOrg);
	}

	return IsThread();
}

INT_PTR MakeLoopWireSearch(CDXFshape* pShapeBase, int nRef)
{
#ifdef _DEBUG
	printf("MakeLoopWireSearch()\n");
#endif

	INT_PTR		i, j, nCnt = 0;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CShapeArray	obShape;	// �����̌`��W���ꗗ
	CRectF		rcBase(pShapeBase->GetMaxRect() ), rc;
	rcBase.InflateRect(rcBase.Width()*0.05f, rcBase.Height()*0.05f);	// �v�Z�덷�ɘa�̂���10%�g��
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
				nCnt += MakeLoopWireSearch(pShape, nRef+1);
			}
		}
	}

	if ( obShape.IsEmpty() || !IsThread() )
		return 0;

	// obShape�ɒ~�ς��ꂽ�ް��̐���
	CDXFshape*	pShapeResult = obShape[0];
	j = obShape.GetSize();

	if ( j == 1 ) {
		// ������޼ު�Ă�1��
		BOOL	bResult;
		if ( pShapeBase->GetShapeType()==0 && pShapeResult->GetShapeType()==0 ) {
			// �����Ƃ��֊s�W���̏ꍇ�̂�
			CString	strLayer1(pShapeBase->GetParentLayer()->GetLayerName()),
					strLayer2(pShapeResult->GetParentLayer()->GetLayerName());
			int		nCmp = strLayer1.CompareNoCase(strLayer2);
			if ( nCmp > 0 ) {
				// �㉺�ٌ`�󐶐�����(ڲԖ��̑傫�������2����UV������)
				bResult =MakeLoopWireAdd(pShapeResult, pShapeBase, nRef>0 ? FALSE : TRUE);
			}
			else if ( nCmp < 0 ) {
				bResult =MakeLoopWireAdd(pShapeBase, pShapeResult, nRef>0 ? FALSE : TRUE);
			}
			else {
				// �ʏ폈��
				bResult = MakeLoopWireAdd(pShapeResult, NULL, FALSE);
			}
		}
		else {
			// �ʏ폈��
			bResult = MakeLoopWireAdd(pShapeResult, NULL, FALSE);
		}
		return bResult ? nCnt : -1;
	}

	// �����ɕ����̵�޼ު��
	const	CPointF		ptOrg(CDXFdata::ms_ptOrg);
			CPointF		pt(CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg);
	float	dGap, dGapMin;

	while ( pShapeResult && IsThread() ) {
		dGapMin = FLT_MAX;
		pShapeResult = NULL;
		// pt�Ɉ�ԋ߂�Ȳè�ނ̌`��W��������
		for ( i=0; i<j && IsThread(); i++ ) {
			pShape = obShape[i];
			if ( !pShape->IsMakeFlg() ) {
				dGap = pShape->GetSelectObjectFromShape(pt);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pShapeResult = pShape;
				}
			}
		}
		if ( pShapeResult ) {
			// �`��W����NC����
			if ( !MakeLoopWireAdd(pShapeResult, NULL, FALSE) )
				return -1;
			// pt���W�̍X�V
			pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		}
	}

	return nCnt;
}

BOOL MakeLoopWireAdd(CDXFshape* pShapeXY, CDXFshape* pShapeUV, BOOL bParent)
{
#ifdef _DEBUG
	printf("MakeLoopWireAdd() MapName=%s\n", LPCTSTR(pShapeXY->GetShapeName()));
	if ( pShapeUV )
		printf(" +MapName(UV)=%s\n", LPCTSTR(pShapeUV->GetShapeName()));
#endif
	if ( pShapeXY->IsMakeFlg() )	// �����֊s���ŁA���ɐ����ς݂̏ꍇ������
		return TRUE;
	pShapeXY->SetShapeFlag(DXFMAPFLG_MAKE);

	if ( pShapeUV ) {
		if ( pShapeUV->IsMakeFlg() )
			pShapeUV = NULL;
		else
			pShapeUV->SetShapeFlag(DXFMAPFLG_MAKE);
	}

	// G����ͯ��(�J�n����)
	if ( g_obMakeData.IsEmpty() ) {
		// G����ͯ��(�J�n����)
		if ( g_wBindOperator & TH_HEADER )
			AddCustomWireCode(GetStr(MKWI_STR_HEADER));
		// ̧�ٖ��̺���
		if ( g_pDoc->IsDocFlag(DXFDOC_BIND) && AfxGetNCVCApp()->GetDXFOption()->GetDxfOptFlg(DXFOPT_FILECOMMENT) ) {
			CString	strBuf;
			strBuf.Format(IDS_MAKENCD_BINDFILE, g_pDoc->GetTitle());
			_AddMakeWireStr(strBuf);
		}
	}

	// AWF�߲�Ă̌���
	CDXFcircle* pCircle = bParent ? GetOutsideAWF() : GetInsideAWF(pShapeXY);
	if ( pCircle ) {
		// AWF�ؒf
		_AddAWFcut();
		// AWF�߲�Ă܂ł̈ړ��ƌ���
		_AddMoveAWFpoint(pCircle);
		CDXFdata::ms_pData = pCircle;
	}
	else {
		// AWF����
		_AddAWFconnect();
	}

	// �����̕���
	if ( pShapeUV )
		return MakeLoopWireAdd_Hetero(pShapeXY, pShapeUV);
	else {
		CDXFchain*	pChain = pShapeXY->GetShapeChain();
		return pChain ?
			MakeLoopWireAdd_ChainList(pShapeXY, pChain) :
			MakeLoopWireAdd_EulerMap(pShapeXY);
	}
}

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopWireAdd_Hetero(CDXFshape* pShapeXY, CDXFshape* pShapeUV)
{
	POSITION	posXY, posUV, posXYb, posUVb;
	CDXFchain*	pChainXY = pShapeXY->GetShapeChain();
	CDXFchain*	pChainUV = pShapeUV->GetShapeChain();
	CDXFdata*	pDataXY;
	CDXFdata*	pDataUV;
	CVPointF	vptXY, vptUV;
	size_t		nCntXY = pChainXY->GetObjectCount(),
				nCntUV = pChainUV->GetObjectCount();
	int			nXYUV;		// 1:XY�����ŏI��, 2:UV�����ŏI��, 0:��������

	ASSERT(pChainXY);	ASSERT(pChainUV);
	posXY = posXYb = pShapeXY->GetFirstChainPosition();
	posUV = posUVb = pShapeUV->GetFirstChainPosition();
	ASSERT(posXY);		ASSERT(posUV);

	pDataXY = pChainXY->GetAt(posXY);
	pDataUV = pChainUV->GetAt(posUV);
	_AddMoveGdata(pDataXY, pDataUV);

	if ( !g_mpPause.IsEmpty() ) {
		// XY,UV�A�g �ꎞ��~Ӱ�ށi��޼ު�Đ����������A�g�ł��邱�Ƃ������j
		CDXFarray*	pobArray;
		CPointF		ptsXY, ptsUV;
		size_t		i, nCnt;
		BOOL		bSeqXY = TRUE, bSeqUV = TRUE;
		do {
			if ( bSeqXY ) {
				pDataXY = pChainXY->GetSeqData(posXY);
				if ( !posXY )
					posXY = pChainXY->GetFirstPosition();
				ptsXY = pDataXY->GetStartCutterPoint();
			}
			if ( bSeqUV ) {
				pDataUV = pChainUV->GetSeqData(posUV);
				if ( !posUV )
					posUV = pChainUV->GetFirstPosition();
				ptsUV = pDataUV->GetStartCutterPoint();
			}
			// �ꎞ��~�_����
			if ( bSeqUV && g_mpPause.Lookup(ptsUV, pobArray) ) {
				// XY���ō��W��������
				nCnt = pDataXY->SetVectorPoint(vptXY, GetDbl(MKWI_DBL_ELLIPSE));
				// ���̐�����UV���W��o�^
				ptsUV = pDataUV->GetStartMakePoint();
				for ( i=0; i<nCnt; i++ )
					vptUV.push_back(ptsUV);
				_AddMakeGdata(vptXY, vptUV);
				pDataXY->SetMakeFlg();
				// �����ް��ǂݍ���
				bSeqXY = posXY==posXYb ? FALSE : TRUE;	// �ǂ�
				bSeqUV = FALSE;							// �ǂ܂Ȃ�
				nXYUV = 1;
			}
			else if ( bSeqXY && g_mpPause.Lookup(ptsXY, pobArray) ) {
				nCnt = pDataUV->SetVectorPoint(vptUV, GetDbl(MKWI_DBL_ELLIPSE));
				ptsXY = pDataXY->GetStartMakePoint();
				for ( i=0; i<nCnt; i++ )
					vptXY.push_back(ptsXY);
				_AddMakeGdata(vptXY, vptUV);
				pDataUV->SetMakeFlg();
				bSeqXY = FALSE;
				bSeqUV = posUV==posUVb ? FALSE : TRUE;
				nXYUV = 2;
			}
			else {
				bSeqXY = posXY==posXYb ? FALSE : TRUE;
				bSeqUV = posUV==posUVb ? FALSE : TRUE;
				// ��޼ު�Đ��������Ɖ��肵���㉺�ٌ`��̍��W�o�^
				pDataXY->SetWireHeteroData(pDataUV, vptXY, vptUV, GetDbl(MKWI_DBL_ELLIPSE));
				if ( vptXY.empty() ) {
					// ���W�����K�v�Ȃ� -> ��޼ު�č��W���璼�ڐ���
					_AddMakeGdata(pDataXY, pDataUV);
				}
				else {
					// �������W����G01����
					_AddMakeGdata(vptXY, vptUV);
					pDataXY->SetMakeFlg();
					pDataUV->SetMakeFlg();
				}
				nXYUV = 0;
			}
			vptXY.clear();
			vptUV.clear();
		} while ( (bSeqXY || bSeqUV) && IsThread() );
		// �I�_����
		switch ( nXYUV ) {
		case 1:
			// �c����UV�ް�������	
			nCnt = pDataUV->SetVectorPoint(vptUV, GetDbl(MKWI_DBL_ELLIPSE));
			ptsXY = pDataXY->GetEndMakePoint();		// �I�_
			for ( i=0; i<nCnt; i++ )
				vptXY.push_back(ptsXY);
			_AddMakeGdata(vptXY, vptUV);
			pDataUV->SetMakeFlg();
			break;
		case 2:
			// �c����XY�ް�������
			nCnt = pDataXY->SetVectorPoint(vptXY, GetDbl(MKWI_DBL_ELLIPSE));
			ptsUV = pDataUV->GetEndMakePoint();
			for ( i=0; i<nCnt; i++ )
				vptUV.push_back(ptsUV);
			_AddMakeGdata(vptXY, vptUV);
			pDataXY->SetMakeFlg();
			break;
		}
	}
	else if ( nCntXY == nCntUV ) {
		do {
			pDataXY = pChainXY->GetSeqData(posXY);
			pDataUV = pChainUV->GetSeqData(posUV);
			if ( !posXY )
				posXY = pChainXY->GetFirstPosition();
			if ( !posUV )
				posUV = pChainUV->GetFirstPosition();
			// �㉺�ٌ`��̍��W�o�^
			pDataXY->SetWireHeteroData(pDataUV, vptXY, vptUV, GetDbl(MKWI_DBL_ELLIPSE));
			if ( vptXY.empty() ) {
				// ���W�����K�v�Ȃ� -> ��޼ު�č��W���璼�ڐ���
				_AddMakeGdata(pDataXY, pDataUV);
			}
			else {
				// �������W����G01����
				_AddMakeGdata(vptXY, vptUV);
				pDataXY->SetMakeFlg();
				pDataUV->SetMakeFlg();
			}
			vptXY.clear();
			vptUV.clear();
		} while ( posXY!=posXYb && IsThread() );
	}
	else if ( nCntXY==1 && pChainXY->GetHead()->GetMakeType()==DXFCIRCLEDATA ) {
		CDXFcircle*	pCircle = static_cast<CDXFcircle*>(pChainXY->GetHead());
		// �������̗\�z�l
		size_t	n = (size_t)ceil(PI2*pCircle->GetR() / GetDbl(MKWI_DBL_ELLIPSE));
		// �����\�z�����޼ު�Ē����ŋϓ���
		pChainUV->SetVectorPoint(posUV, vptUV, n);
		// �ϓ����œ���ꂽ�������ŉ~�𕪊�
		n = vptUV.size();		// �[�����o��̂ŕ��������Ď擾
		pChainXY->SetVectorPoint(posXY, vptXY, n);
		// ���W����
		_AddMakeGdata(vptXY, vptUV);
		// �����ς��׸�
		pChainXY->SetMakeFlags();
		pChainUV->SetMakeFlags();
	}
	else if ( nCntUV==1 && pChainUV->GetHead()->GetMakeType()==DXFCIRCLEDATA ) {
		CDXFcircle*	pCircle = static_cast<CDXFcircle*>(pChainUV->GetHead());
		size_t	n = (size_t)ceil(PI2*pCircle->GetR() / GetDbl(MKWI_DBL_ELLIPSE));
		pChainXY->SetVectorPoint(posXY, vptXY, n);
		n = vptXY.size();
		pChainUV->SetVectorPoint(posUV, vptUV, n);
		_AddMakeGdata(vptXY, vptUV);
		pChainXY->SetMakeFlags();
		pChainUV->SetMakeFlags();
	}
	else if ( nCntXY > nCntUV ) {
		// ��޼ު�Ă̐��������i���G�Ȍ`���z��j����ɕ������s��
		pChainXY->SetVectorPoint(posXY, vptXY, GetDbl(MKWI_DBL_ELLIPSE));
		pChainUV->SetVectorPoint(posUV, vptUV, vptXY.size());
		_AddMakeGdata(vptXY, vptUV);
		pChainXY->SetMakeFlags();
		pChainUV->SetMakeFlags();
	}
	else {
		pChainUV->SetVectorPoint(posUV, vptUV, GetDbl(MKWI_DBL_ELLIPSE));
		pChainXY->SetVectorPoint(posXY, vptXY, vptUV.size());
		_AddMakeGdata(vptXY, vptUV);
		pChainXY->SetMakeFlags();
		pChainUV->SetMakeFlags();
	}

	return IsThread();
}

BOOL MakeLoopWireAdd_ChainList(CDXFshape* pShape, CDXFchain* pChain)
{
	POSITION	pos1, pos2;
	CDXFdata*	pData;

	// ���H�w���ɔ����J�n�ʒu��������̐ݒ�
	pos1 = pos2 = pShape->GetFirstChainPosition();
	ASSERT(pos1);
	pData = pChain->GetAt(pos1);

	// �؍��ް�����
	_AddMoveGdata(pData);
	// �J�n�߼޼�݂���ٰ��
	do {
		pData = pChain->GetSeqData(pos1);
		_AddMakeGdata(pData);
		if ( !pos1 )
			pos1 = pChain->GetFirstPosition();
	} while ( pos1!=pos2 && IsThread() ); 
	// �Ō�ɐ��������ް���Ҕ�
	CDXFdata::ms_pData = pData;

	return IsThread();
}

BOOL MakeLoopWireAdd_EulerMap(CDXFshape* pShape)
{
	BOOL		bEuler = FALSE;
	CDXFmap*	pEuler = pShape->GetShapeMap();
	ASSERT( pEuler );
	// �P��ڂ̐�������
	if ( !MakeLoopWireAdd_EulerMap_Make( pShape, pEuler, bEuler ) )
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
		PMAP_FOREACH(ptKey, pArray, pEuler)
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
			if ( !MakeLoopWireAdd_EulerMap_Search(pt, pEuler, &mpLeak) )
				return FALSE;
		}
		if ( !IsThread() )
			return FALSE;
		// �Q��ڈȍ~�̐�������
		bEuler = FALSE;
		if ( !MakeLoopWireAdd_EulerMap_Make( pShape, &mpLeak, bEuler ) )
			return FALSE;
	}

	return IsThread();
}

BOOL MakeLoopWireAdd_EulerMap_Make(CDXFshape* pShape, CDXFmap* pEuler, BOOL& bEuler)
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
	const CPointF	ptOrg(CDXFdata::ms_ptOrg);

	// �J�n�ʒu�w��
	tie(pWork, pDataFix) = pShape->GetStartObject();
	const	CPointF		ptNow( pDataFix ?
		static_cast<CDXFworkingStart*>(pWork)->GetStartPoint() :	// ���݈ʒu���X�V
		CDXFdata::ms_pData->GetEndCutterPoint()+ptOrg );

	// ���̍��Wϯ�߂���M�����v���𖞂����Ă��邩�A���A
	// ���݈ʒu�i���H�J�n�ʒu�j�ɋ߂��Ƃ��납��ꎞ�W���𐶐�
	tie(bEuler, pArray, pt) = pEuler->IsEulerRequirement(ptNow);
	if ( !IsThread() )
		return FALSE;
	pt -= ptOrg;
	ASSERT( pArray );
	ASSERT( !pArray->IsEmpty() );

	// --- ��M�����̐���(�ċA�Ăяo���ɂ��؍\�����)
	if ( !MakeLoopWireAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler) ) {
		// ��M�����ł���n�Y�₯�ǎ��s����������ɘa���Ă�蒼��
		bEuler = FALSE;
		MakeLoopWireAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler);
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

	BOOL	bNext = FALSE;
	// �؍��ް��܂ł̈ړ�
	pData = ltEuler.GetFirstData();
	_AddMoveGdata(pData);
	// �؍��ް�����
	for ( pos=ltEuler.GetFirstPosition(); pos && IsThread(); ) {
		pData = ltEuler.GetSeqData(pos);
		if ( pData ) {
			if ( bNext ) {
				_AddMoveGdata(pData);
				bNext = FALSE;
			}
			_AddMakeGdata(pData);
		}
		else
			bNext = TRUE;
	}
	// �Ō�ɐ��������ް���Ҕ�
	CDXFdata::ms_pData = pData;

	return IsThread();
}

BOOL MakeLoopWireAdd_EulerMap_Search
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
						if ( !MakeLoopWireAdd_EulerMap_Search(pt, pOrgMap, pResultMap) )
							break;
					}
				}
			}
		}
	}

	return IsThread();
}

BOOL MakeLoopWireAdd_with_one_stroke
	(const CDXFmap* pEuler, BOOL bEuler, BOOL bMakeShape, 
		const CPointF& pt, const CDXFarray* pArray, CDXFlist& ltEuler)
{
	INT_PTR		i;
	const INT_PTR	nLoop = pArray->GetSize();
	CDXFdata*	pData;
	CDXFarray*	pNextArray;
	CPointF		ptNext;
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
			if ( MakeLoopWireAdd_with_one_stroke(pEuler, bEuler, bMakeShape, ptNext, pNextArray, ltEuler) )
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

//////////////////////////////////////////////////////////////////////

//	AddCustomWireCode() ����Ăяo��
class CMakeCustomWireCode : public CMakeCustomCode	// MakeCustomCode.h
{
public:
	CMakeCustomWireCode() :
				CMakeCustomCode(g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", "WorkDepth", "TaperMode",
			"G90orG91", "G92_INITIAL", "G92X", "G92Y",
			"G0XY_INITIAL"
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
			if ( GetFlg(MKWI_FLG_PROG) )
				strResult.Format(IDS_MAKENCD_PROG,
					GetFlg(MKWI_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKWI_NUM_PROG));
			break;
		case 1:		// WorkDepth
			strResult.Format(IDS_MAKENCD_FORMAT, GetDbl(MKWI_DBL_DEPTH));
			break;
		case 2:		// TaperMode
			strResult = GetStr(MKWI_STR_TAPERMODE);
			break;
		case 3:		// G90orG91
			strResult = CNCMakeBase::MakeCustomString(GetNum(MKWI_NUM_G90)+90);
			break;
		case 4:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKWI_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKWI_DBL_G92Y);
			strResult = CNCMakeBase::MakeCustomString(92, NCD_X|NCD_Y, dValue, FALSE);
			break;
		case 5:		// G92X
		case 6:		// G92Y
			nTestCode -= 3;
			dValue[nTestCode] = GetDbl(MKWI_DBL_G92X+nTestCode);
			strResult = CNCMakeBase::MakeCustomString(-1, g_dwSetValFlags[nTestCode], dValue, FALSE);
			break;
		case 7:		// G0XY_INITIAL
			dValue[NCA_X] = GetDbl(MKWI_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKWI_DBL_G92Y);
			strResult = CNCMakeBase::MakeCustomString(0, NCD_X|NCD_Y, dValue);
			break;
		default:
			strResult = str.c_str();
		}

		return strResult;
	}
};

void AddCustomWireCode(const CString& strFileName)
{
	CString	strBuf, strResult;
	CMakeCustomWireCode			custom;
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
				_AddMakeWireStr(strResult);
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// ���̴װ�͐�������(�x���̂�)
	}
}

//////////////////////////////////////////////////////////////////////

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

CDXFcircle* SetAWFandPAUSEdata(void)
{
	INT_PTR		i, j, k, nLoop1 = g_pDoc->GetLayerCnt(), nLoop2;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFdata*	pData;
	CDXFcircle*	pDataResult = NULL;
	CDXFcircle*	pCircle;
	CPointF		ptOrg(0, 0),				// ���H���_
				pt;
	float		dGap, dGapMin = FLT_MAX;	// �w��_�Ƃ̋���
	CSortArray<CObArray, CDXFcircle*>	obAWFdata;		// AWF�ް����u����

	// AWF�ƈꎞ��~�߲�Ă̌���
	for ( i=0; i<nLoop1 && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		nLoop2 = pLayer->GetDxfSize();
		for ( j=0; j<nLoop2 && IsThread(); j++ ) {
			pData = pLayer->GetDxfData(j);
			switch ( pData->GetMakeType() ) {
			case DXFPOINTDATA:
				pData->OrgTuning(FALSE);
				g_mpPause.SetMakePointMap(pData);
				break;
			case DXFCIRCLEDATA:
				pCircle = static_cast<CDXFcircle*>(pData);
				if ( g_pMakeOpt->IsAWFcircle(pCircle->GetMakeR()) ) {
					// AWF�ް��̒��o
					pData->SetSearchFlg();
					pData->ChangeMakeType(DXFPOINTDATA);
					obAWFdata.Add(pCircle);
					// �e�𐶐��ς�
					pData->GetParentMap()->SetShapeFlag(DXFMAPFLG_MAKE);
				}
				break;
			}
		}
	}
	if ( obAWFdata.IsEmpty() )
		return NULL;

	// �`��̓������O�����̐U�蕪��
	for ( i=0; i<obAWFdata.GetSize() && IsThread(); i++ ) {
		pCircle = obAWFdata[i];
		pt = pCircle->GetCenter();
		for ( j=0; j<nLoop1 && IsThread(); j++ ) {
			pLayer = g_pDoc->GetLayerData(j);
			if ( !pLayer->IsMakeTarget() )
				continue;
			nLoop2 = pLayer->GetShapeSize();
			for ( k=0; k<nLoop2 && IsThread(); k++ ) {
				pShape = pLayer->GetShapeData(k);
				if ( !pShape->IsMakeFlg() && pShape->GetShapeAssemble()!=DXFSHAPE_EXCLUDE &&
							pShape->GetMaxRect().PtInRect(pt) ) {
					g_obAWFinside.Add(pCircle);
					break;
				}
			}
			if ( k >= nLoop2 )
				g_obAWFoutside.Add(pCircle);
		}
	}

	// �O����D��I�ɁA��ԋ߂�AWF�߲�Ă�����
	CSortArray<CObArray, CDXFcircle*>* pArray = g_obAWFoutside.IsEmpty() ?
		&g_obAWFinside : &g_obAWFoutside;
	for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
		pCircle = pArray->GetAt(i);
		dGap = pCircle->GetEdgeGap(ptOrg);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			pDataResult = pCircle;
		}
	}

	return pDataResult;
}

CDXFshape* GetNearPointWire(const CPointF& pt)
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

CDXFcircle*	GetInsideAWF(const CDXFshape* pShape)
{
	CRectF	rc(pShape->GetMaxRect());
	CDXFcircle*	pCircle;

	for ( int i=0; i<g_obAWFinside.GetSize() && IsThread(); i++ ) {
		pCircle = g_obAWFinside[i];
		if ( !pCircle->IsMakeFlg() && rc.PtInRect(pCircle->GetCenter()) )
			return pCircle;
	}

	return NULL;
}

CDXFcircle*	GetOutsideAWF(void)
{
	int		i;
	CPointF	pt(CDXFdata::ms_pData->GetEndCutterPoint());
	float	dGap, dGapMin = FLT_MAX;
	CDXFcircle*	pCircle;
	CDXFcircle*	pDataResult = NULL;

	for ( i=0; i<g_obAWFoutside.GetSize() && IsThread(); i++ ) {
		pCircle = g_obAWFoutside[i];
		if ( !pCircle->IsMakeFlg() ) {
			dGap = pCircle->GetEdgeGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pDataResult = pCircle;
			}
		}
	}

	if ( pDataResult )
		return pDataResult;

	for ( i=0; i<g_obAWFinside.GetSize() && IsThread(); i++ ) {
		pCircle = g_obAWFinside[i];
		if ( !pCircle->IsMakeFlg() ) {
			dGap = pCircle->GetEdgeGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pDataResult = pCircle;
			}
		}
	}

	return pDataResult;
}

//////////////////////////////////////////////////////////////////////

// NC�����̸�۰��ٕϐ�������(��n��)�گ��
UINT MakeWire_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

#ifdef _DEBUG
	printf("MakeWire_AfterThread() Start\n");
#endif
	for ( int i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	g_obMakeData.RemoveAll();
	g_obAWFinside.RemoveAll();
	g_obAWFoutside.RemoveAll();
	g_mpPause.RemoveAll();

	if ( g_pMakeOpt )
		delete	g_pMakeOpt;

	g_csMakeAfter.Unlock();

	return 0;
}
