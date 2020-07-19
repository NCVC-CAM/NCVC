// TH_MakeLathe.cpp
//		���՗pNC����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCDoc.h"		// g_szNCcomment[]
#include "NCMakeLatheOpt.h"
#include "NCMakeLathe.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DBG_NCMAKE_TIME	//	�������Ԃ̕\��
#endif

using std::string;
using namespace boost;

extern	LPCTSTR	g_szNCcomment[];

// ��۰��ٕϐ���`
static	CThreadDlg*			g_pParent;
static	CDXFDoc*			g_pDoc;
static	CNCMakeLatheOpt*	g_pMakeOpt;

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	MAXLAYER	3	// ���a�E�O�a�E�ː�
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)

// NC�����ɕK�v���ް��Q
static	CDXFshape*	g_pShape[2];	// 0:Inside, 1:Outside
static	CLayerData*	g_pGrooveLayer;
static	CDXFarray	g_obLineTemp[2];
static	CDXFarray	g_obMakeLine[2];
static	CTypedPtrArrayEx<CPtrArray, CNCMakeLathe*>	g_obMakeData;	// ���H�ް�

// ��ފ֐�
static	void	InitialVariable(void);			// �ϐ�������
static	void	SetStaticOption(void);			// �ÓI�ϐ��̏�����
static	BOOL	MakeLathe_MainFunc(void);		// NC������Ҳ�ٰ��
static	BOOL	CreateShapeThread(void);		// �`��F������
static	void	InitialShapeData(void);			// �`��F���̏�����
static	BOOL	CreateInsidePitch(float&);		// ���a�̾�Ă𐶐�
static	BOOL	CreateOutsidePitch(void);		// �O�a�̾�Ă𐶐�
static	BOOL	CreateRoughPass(int);			// �r���H�ް��̐���(���O����)
static	BOOL	MakeInsideCode(const CPointF&);	// NC���ނ̐���
static	BOOL	MakeOutsideCode(const CPointF&);
static	BOOL	MakeGrooveCode(const CPointF&);
static	BOOL	CheckXZMove(int, const CPointF&, const CPointF&);
static	CPointF	MoveInsideCode(const CDXFchain*, const CPointF&, const CPointF&);
static	void	MoveOutsideCode(const CDXFdata*, const CPointF&, const CPointF&);
static	BOOL	OutputLatheCode(void);			// NC���ނ̏o��

// ͯ�ް,̯�ް���̽�߼�ٺ��ސ���
static	void	AddCustomLatheCode(const CString&);

// �C���ް��̐���
static inline	void	AddMakeLatheStr(const CString& strData)
{
	CNCMakeLathe*	pNCD = new CNCMakeLathe(strData);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

// ̪��ލX�V
static	int		g_nFase;			// ̪��އ�
static	void	SendFaseMessage(INT_PTR = -1, int = -1, LPCTSTR = NULL);
static	inline	void	SetProgressPos(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetPos((int)n);
}

// ���בւ��⏕�֐�
static	int		ShapeSortFunc(CDXFshape*, CDXFshape*);	// �`��F���ް��̕��בւ�
static	int		GrooveSortFunc(CDXFdata*, CDXFdata*);	// �˂��؂���H���ް����בւ�

// ��޽گ�ފ֐�
static	CCriticalSection	g_csMakeAfter;	// MakeLathe_AfterThread()�گ��ۯ���޼ު��
static	UINT	MakeLathe_AfterThread(LPVOID);	// ��n���گ��

//////////////////////////////////////////////////////////////////////
// ���՗pNC�����گ��
//////////////////////////////////////////////////////////////////////

UINT MakeLathe_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeLathe_Thread() Start\n");
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

	// �������\��
	g_nFase = 0;
	SendFaseMessage(-1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		// NC������߼�ݵ�޼ު�Ă̐����Ƶ�߼�݂̓ǂݍ���
		g_pMakeOpt = new CNCMakeLatheOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKELATHE)->GetHead());
		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		InitialVariable();
		// �������Ƃɕω��������Ұ���ݒ�
		SetStaticOption();
		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
		// �������蓖��
		g_obMakeData.SetSize(0, 1024);		// �����ް������͖��m��
		// �����J�n
		BOOL bResult = MakeLathe_MainFunc();
		if ( bResult )
			bResult = OutputLatheCode();

		// �߂�l���
		if ( bResult && IsThread() )
			nResult = IDOK;
#ifdef _DEBUG
		printf("MakeLathe_Thread All Over!!!\n");
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

	// �����̏��ł͂Ȃ��̂ŏI���O�ɐ�ɏ������Ă���
	// MakeLathe_AfterThread�گ�ޓ��ł̏�����NG
	for ( int i=0; i<SIZEOF(g_pShape); i++ ) {
		if ( g_pShape[i] )
			g_pShape[i]->CrearScanLine_Lathe();
	}

	// �I������
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��
	// ��������NC���ނ̏����گ��(�D��x��������)
	AfxBeginThread(MakeLathe_AfterThread, NULL,
//		THREAD_PRIORITY_LOWEST);
		THREAD_PRIORITY_IDLE);
//		THREAD_PRIORITY_BELOW_NORMAL;

	// ������޼ު�č폜
	if ( g_pMakeOpt )
		delete	g_pMakeOpt;

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	ZEROCLR(g_pShape);
	g_pGrooveLayer = NULL;
	// �[�ʂ̏I�_�����_
	CDXFdata::ms_ptOrg = g_pDoc->GetLatheLine(1)->GetNativePoint(1);
	// ORIGIN�ް����H����ʒu
	CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	// �e��޼ު�Ă̌��_����
	g_pDoc->GetCircleObject()->OrgTuning(FALSE);
	g_pDoc->GetLatheLine(0)->OrgTuning(FALSE);
	g_pDoc->GetLatheLine(1)->OrgTuning(FALSE);
	// ������߼�݂̏�����
	CNCMakeLathe::InitialVariable();		// CNCMakeBase::InitialVariable()
}

void SetStaticOption(void)
{
	// CDXFdata�̐ÓI�ϐ�������
	CDXFdata::ms_fXRev = CDXFdata::ms_fYRev = FALSE;

	// CNCMakeLathe�̐ÓI�ϐ�������
	CPointF	ptOrg(g_pDoc->GetCircleObject()->GetStartMakePoint());
	CNCMakeLathe::ms_xyz[NCA_X] = ptOrg.y;
	CNCMakeLathe::ms_xyz[NCA_Y] = 0.0f;
	CNCMakeLathe::ms_xyz[NCA_Z] = ptOrg.x;

	// ������߼�݂ɂ��ÓI�ϐ��̏�����
	CNCMakeLathe::SetStaticOption(g_pMakeOpt);
}

BOOL OutputLatheCode(void)
{
	CString	strPath, strFile,
			strNCFile(g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_obMakeData.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		CStdioFile	fp(strNCFile,
			CFile::modeCreate | CFile::modeWrite |
			CFile::shareExclusive | CFile::typeText | CFile::osSequentialScan);
		for ( INT_PTR i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
			g_obMakeData[i]->WriteGcode(fp);
			SetProgressPos(i+1);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	SetProgressPos(g_obMakeData.GetSize());
	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC����Ҳݽگ��
//////////////////////////////////////////////////////////////////////

BOOL MakeLathe_MainFunc(void)
{
	// �`��F��������p���Đ}�`�W�����쐬
	if ( !CreateShapeThread() )
		return FALSE;
	// ���Չ��H�`��̌��_������
	InitialShapeData();

	// ���a�ƊO�a�̐؍폀���ް����쐬
	float	dHole = GetDbl(MKLA_DBL_HOLE);	// ������������
	if ( g_pShape[0] ) {
		if ( !CreateInsidePitch(dHole) )	// dHole�X�V
			return FALSE;
	}
	if ( g_pShape[1] ) {
		if ( !CreateOutsidePitch() )
			return FALSE;
	}

	// ̪���1 : �`��̵̾�ĂƓ��O�a�̵̾�Ă̌�_���v�Z���r���H���ް��𐶐�
	SendFaseMessage(g_obLineTemp[0].GetSize()+g_obLineTemp[1].GetSize());
	for ( int i=0; i<SIZEOF(g_obLineTemp); i++ ) {
		if ( !g_obLineTemp[i].IsEmpty() ) {
			if ( !CreateRoughPass(i) )
				return FALSE;
		}
	}

	// �[�ʂƊO�a�̍ő�l���擾
	CPointF		ptMax(g_pDoc->GetLatheLine(0)->GetStartMakePoint().x,	// Z��
					  g_pDoc->GetLatheLine(1)->GetStartMakePoint().y);	// X��

	// G����ͯ��(�J�n����)
	AddCustomLatheCode(GetStr(MKLA_STR_HEADER));

	// �[�ʏ���
	if ( GetFlg(MKLA_FLG_ENDFACE) ) {
		CNCMakeLathe* pNCD = new CNCMakeLathe;
		ASSERT(pNCD);
		pNCD->CreateEndFace(ptMax);
		g_obMakeData.Add(pNCD);
	}

	// ��������
	if ( !GetStr(MKLA_STR_DRILL).IsEmpty() ) {
		CNCMakeLathe* pNCD = new CNCMakeLathe;
		ASSERT(pNCD);
		pNCD->CreatePilotHole();
		g_obMakeData.Add(pNCD);
	}

	// ̪���2 : NC���ނ̐���
	SendFaseMessage(
		g_obMakeLine[0].GetSize()+GetNum(MKLA_NUM_I_MARGIN)+
		g_obMakeLine[1].GetSize()+GetNum(MKLA_NUM_O_MARGIN)
	);
	if ( g_pShape[0] && dHole>0.0f ) {
		if ( !MakeInsideCode(ptMax) )
			return FALSE;
	}
	if ( g_pShape[1] ) {
		if ( !MakeOutsideCode(ptMax) )
			return FALSE;
	}
	if ( g_pGrooveLayer ) {
		if ( !MakeGrooveCode(ptMax) )
			return FALSE;
	}

	// G����̯��(�I������)
	AddCustomLatheCode(GetStr(MKLA_STR_FOOTER));

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

BOOL CreateShapeThread(void)
{
#ifdef _DEBUG
	printf("CreateShapeThread() for TH_MakeLathe Start\n");
	CDXFchain*	pChainDbg;
	CDXFdata*	pDataDbg;
	CPointF		ptsd, pted;
#endif
	const INT_PTR	nLayerLoop = g_pDoc->GetLayerCnt();
	if ( nLayerLoop > MAXLAYER ) {
		AfxMessageBox(IDS_ERR_LATHE_LAYER, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	NCVCTHREADPARAM	param;
	param.pParent = NULL;
	param.pDoc    = g_pDoc;
	param.wParam  = NULL;
	param.lParam  = NULL;

	// �`��F�������̽گ�ސ���
	CWinThread*	pThread = AfxBeginThread(ShapeSearch_Thread, &param,	// TH_ShapeSearch.cpp
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if ( !pThread )
		return FALSE;
	pThread->m_bAutoDelete = FALSE;
	pThread->ResumeThread();
	::WaitForSingleObject(pThread->m_hThread, INFINITE);
	delete	pThread;

	// ���Ր����ł���W�������o���ꂽ���H
	INT_PTR		i, j, n, nLoop;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFmap*	pMap;
	CString		strInside(INSIDE_S),	// g_szNCcomment[LATHEINSIDE]
				strGroove("GROOVE"),
				strLayer;
	strInside.MakeUpper();

	for ( i=0; i<nLayerLoop && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		strLayer = pLayer->GetLayerName();
#ifdef _DEBUG
		printf(" Layer=%s\n", LPCTSTR(strLayer));
#endif
		strLayer.MakeUpper();
		if ( strLayer.Find(strGroove) >= 0 ) {
			// �˂��؂�a���H�f�[�^
			g_pGrooveLayer = pLayer;
			continue;	// �`�󏈗����Ȃ�
		}
		nLoop = pLayer->GetShapeSize();
		// [��|�O]�a�f�[�^�͐}�`�W�����P�Ɍ���
		if ( nLoop > 1 ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_LATHE_SHAPE, pLayer->GetLayerName());
			AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
		// �������O�����̔���
		n = strLayer.Find(strInside) >= 0 ? 0 : 1; 
		//
		for ( j=0; j<nLoop && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			pMap   = pShape->GetShapeMap();
			if ( !pMap || !(pShape->GetShapeFlag()&DXFMAPFLG_EDGE) )
				continue;	// ���̎��_��CDXFchain�Ȃ米�
			// CDXFmap -> CDXFchain �ϊ�
			if ( pShape->ChangeCreate_MapToChain() ) {
#ifdef _DEBUG
				pChainDbg = pShape->GetShapeChain();
				printf("  ShapeNo.%d ChainCnt=%d\n", i, pChainDbg->GetCount());
				PLIST_FOREACH(pDataDbg, pChainDbg)
					ptsd = pDataDbg->GetNativePoint(0);
					pted = pDataDbg->GetNativePoint(1);
					printf("%d (%.3f, %.3f)-(%.3f, %.3f)\n", pDataDbg->GetType(),
						ptsd.x, ptsd.y, pted.x, pted.y);
				END_FOREACH
#endif
				delete	pMap;
				// �����Ώی`���ۑ�
				g_pShape[n] = pShape;
			}
		}
	}

	return TRUE;
}

void InitialShapeData(void)
{
#ifdef _DEBUG
	printf("InitialShapeData() Start\n");
#endif
	CPointF		pts, pte;
	CDXFchain*	pChain;

	for ( int i=0; i<SIZEOF(g_pShape) && IsThread(); i++ ) {
		if ( !g_pShape[i] )
			continue;
		pChain = g_pShape[i]->GetShapeChain();
		ASSERT(pChain);
		pts = pChain->GetHead()->GetNativePoint(0);
		pte = pChain->GetTail()->GetNativePoint(1);
		// Z�l(X)���傫������擪���ނ�
		if ( pts.x < pte.x ) {
#ifdef _DEBUG
			printf("(pts.x < pte.x) Object Reverse\n");
#endif
			pChain->Reverse();
			pChain->ReverseNativePt();
		}
		// �����𐳂������Ă��猴�_����
		pChain->OrgTuning();
	}
}

BOOL CreateInsidePitch(float& dHole)
{
#ifdef _DEBUG
	printf("CreateInsidePitch() Start\n");
//	CPointF		ptsd, pted;
	int			nCntDbg = 0;
#endif
	int			i,
				n = GetNum(MKLA_NUM_I_MARGIN);
	float		d = GetDbl(MKLA_DBL_I_MARGIN),
			cutD  = fabs(GetDbl(MKLA_DBL_I_CUT)),
			dLimit;
	COutlineData*	pOutline;
	CDXFchain*	pChain;
	CDXFdata*	pData;
	VLATHEDRILLINFO	v;

	if ( g_pMakeOpt->GetDrillInfo(v) ) {
		if ( dHole < v.back().d )	// �Ō�̃h�����T�C�Y���̗p
			dHole = v.back().d;		// �i���בւ��̕K�v�Ȃ��Ǝv�����ǁj
	}
	if ( dHole <= 0.0f ) {
		AfxMessageBox(IDS_ERR_LATHE_INSIDE, MB_OK|MB_ICONEXCLAMATION);
		return TRUE;
	}
	dHole /= 2.0f;

	// --- �`��̾�ē����̏���l�����߂�
	// �`��̵̾�Ă𐶐� + ��ԊO���̵̾�Ă��擾
	g_pShape[0]->CreateScanLine_Lathe(n, -d);
	pOutline = g_pShape[0]->GetLatheList();
	// n==0�̑Ώ�
	pChain = pOutline->IsEmpty() ?
		g_pShape[0]->GetShapeChain() : pOutline->GetHead();
	CRectF rc = pChain->GetMaxRect();
	dLimit = max(rc.top, rc.bottom);
#ifdef _DEBUG
	printf("Limit(tuning)=%f\n", dLimit - CDXFdata::ms_ptOrg.y);
#endif

	// �������ނ̌��_����
	for ( i=0; i<pOutline->GetSize() && IsThread(); i++ ) {
		pChain = pOutline->GetAt(i);
		ASSERT(pChain);
		pChain->OrgTuning();
	}

	// ���a�̾�Ă̐�������
	CPointF	pts, pte;
	pts.x = rc.right + GetDbl(MKLA_DBL_I_PULLZ);	// �����㕪
	pte.x = rc.left;
	pts.y = pte.y = dHole + cutD + CDXFdata::ms_ptOrg.y;	// Y�l�͊����������ނ����ٌa����(Naitive���W)
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// dLimit�܂�X����(Y�l)��؂荞��
	while ( pts.y<dLimit && IsThread() ) {
		dxfLine.s = pts;
		dxfLine.e = pte;
		pData = new CDXFline(&dxfLine);
		g_obLineTemp[0].Add(pData);		// OrgTuning()�͕s�v
		pts.y += cutD;
		pte.y += cutD;
#ifdef _DEBUG
		nCntDbg++;
#endif
	}
#ifdef _DEBUG
	printf("OffsetCnt=%d\n", nCntDbg);
#endif

	return IsThread();
}

BOOL CreateOutsidePitch(void)
{
#ifdef _DEBUG
	printf("CreateOutsidePitch() Start\n");
	int			nCntDbg = 0;
#endif
	int			i,
				n = GetNum(MKLA_NUM_O_MARGIN);
	float		d = GetDbl(MKLA_DBL_O_MARGIN),
			cutD  = fabs(GetDbl(MKLA_DBL_O_CUT)),
			dLimit;
	COutlineData*	pOutline;
	CDXFchain*	pChain;
	CDXFdata*	pData;

	// --- �`��̾�ĊO���̉����l�����߂�
	// �`��̵̾�Ă𐶐� + ��ԊO���̵̾�Ă��擾
	g_pShape[1]->CreateScanLine_Lathe(n, d);
	pOutline = g_pShape[1]->GetLatheList();
	// n==0�̑Ώ�
	pChain = pOutline->IsEmpty() ?
		g_pShape[1]->GetShapeChain() : pOutline->GetHead();
	CRectF rc = pChain->GetMaxRect();
	dLimit = min(rc.top, rc.bottom);
#ifdef _DEBUG
	printf("Limit(tuning)=%f\n", dLimit - CDXFdata::ms_ptOrg.y);
#endif

	// �������ނ̌��_����
	for ( i=0; i<pOutline->GetSize() && IsThread(); i++ ) {
		pChain = pOutline->GetAt(i);
		ASSERT(pChain);
		pChain->OrgTuning();
	}

	// �O�a�̾�Ă̐�������
	pData = g_pDoc->GetLatheLine(0);	// �O�a��޼ު��
	CPointF	pts(pData->GetNativePoint(0)),
			pte(pData->GetNativePoint(1));
	pts.x += GetDbl(MKLA_DBL_O_PULLZ);	// �����㕪
	pts.y -= cutD;
	pte.y -= cutD;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// dLimit�܂�X����(Y�l)��؂荞��
	while ( pts.y>dLimit && pte.y>dLimit && IsThread() ) {
		dxfLine.s = pts;
		dxfLine.e = pte;
		pData = new CDXFline(&dxfLine);
		g_obLineTemp[1].Add(pData);		// OrgTuning()�͕s�v
		pts.y -= cutD;
		pte.y -= cutD;
#ifdef _DEBUG
		nCntDbg++;
#endif
	}
#ifdef _DEBUG
	printf("OffsetCnt=%d\n", nCntDbg);
#endif

	return IsThread();
}

BOOL CreateRoughPass(int io)
{
	INT_PTR		i, iPosBase = io==0 ? 0 : g_obLineTemp[1].GetSize(),
				nResult;
	BOOL		bCreate, bInter;
	ENDXFTYPE	enType;
	float		q, qq;
	CPointF		ptChk[4];
	optional<CPointF>	pts;
	CDXFdata*	pData;
	CDXFdata*	pDataChain;
	CDXFdata*	pDataNew;
	CDXFchain*	pChain;
	COutlineData*	pOutline;
	CDXFarc*	pArc;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// �O�a����̌X�����v�Z
	pData = g_obLineTemp[io][0];
	ptChk[0] = pData->GetNativePoint(1) - pData->GetNativePoint(0);
	if ( (qq=ptChk[0].arctan()) < 0.0f )
		qq += PI2;

	// �O�a�����ް���ٰ�߂����r���H�ް����쐬
	for ( i=0; i<g_obLineTemp[io].GetSize() && IsThread(); i++ ) {
		SetProgressPos(i+iPosBase+1);
		pData = g_obLineTemp[io][i];
		pts = pData->GetNativePoint(0);
		pDataChain = NULL;
		bInter = FALSE;		// ��x�ł���_�������TRUE
		// ��ԊO���̌`��̾�Ăƌ�_����
		pOutline = g_pShape[io]->GetLatheList();
		pChain = pOutline->IsEmpty() ?
			g_pShape[io]->GetShapeChain() : pOutline->GetHead();
		PLIST_FOREACH(pDataChain, pChain)
			bCreate = FALSE;
			enType = pDataChain->GetType();
			nResult = pData->GetIntersectionPoint(pDataChain, ptChk, FALSE);
			if ( nResult > 1 ) {
				// ��_���Q�ȏ�
				if ( ptChk[0].x < ptChk[1].x )
					swap(ptChk[0], ptChk[1]);
				if ( enType==DXFARCDATA || enType==DXFELLIPSEDATA ) {
					pArc = static_cast<CDXFarc*>(pDataChain);
					if ( pArc->GetRound() ) {
						// �����v���
						if ( pts ) {
							dxfLine.s = *pts;
							dxfLine.e = ptChk[0];
							bCreate = TRUE;
						}
						pts = ptChk[1];
					}
					else {
						// ���v���
						dxfLine.s = ptChk[0];
						dxfLine.e = ptChk[1];
						pts.reset();
						bCreate = TRUE;
					}
				}
				else {
					// �~�ʈȊO�Ō�_�Q�ȏ�͂��肦�Ȃ����A
					// �Q�ڂŎ��̏�����
					if ( pts ) {
						dxfLine.s = *pts;
						dxfLine.e = ptChk[0];
						bCreate = TRUE;
					}
					pts = ptChk[1];
				}
			}
			else if ( nResult > 0 ) {
				// ��_���P��
				if ( ptChk[0] == pDataChain->GetNativePoint(0) ) {
					// ��޼ު�Ă̎n�_�Ɠ������ꍇ�́A
					// ���O�̵�޼ު�Ăŏ����ς�
					pts.reset();
				}
				else {
					if ( pts ) {
						if ( ptChk[0] != (*pts) ) {
							// �O��̌�_�ƈႤ����������
							dxfLine.s = *pts;
							dxfLine.e = ptChk[0];
							pts.reset();
							bCreate = TRUE;
						}
					}
					else {
						pts = ptChk[0];	// ���̏�����
					}
				}
			}
			// �r���H�ް�����
			if ( bCreate ) {
				pDataNew = new CDXFline(&dxfLine);
				pDataNew->OrgTuning(FALSE);
				g_obMakeLine[io].Add(pDataNew);
				bInter = TRUE;
			}
		END_FOREACH	// End of Chain Loop

		// ��_�[������
		if ( pts ) {
			if ( !bInter ) {
				// ��_���Ȃ���ΊO�a�I�_�܂Ő���
				dxfLine.s = *pts;
				dxfLine.e = pData->GetNativePoint(1);
				pDataNew = new CDXFline(&dxfLine);
				pDataNew->OrgTuning(FALSE);
				g_obMakeLine[io].Add(pDataNew);
			}
			else if ( pDataChain ) {
				// �Ō�̵�޼ު�Ă̌X��������
				if ( enType==DXFARCDATA || enType==DXFELLIPSEDATA ) {
					pArc = static_cast<CDXFarc*>(pDataChain);
					// ��_�����_�ɉ~�̒��S��90����]
					ptChk[1] = pArc->GetCenter() - (*pts);
					if ( pArc->GetRound() ) {
						// �����v���(-90��)
						ptChk[0].x =  ptChk[1].y;
						ptChk[0].y = -ptChk[1].x;
					}
					else {
						// ���v���(+90��)
						ptChk[0].x = -ptChk[1].y;
						ptChk[0].y =  ptChk[1].x;
					}
				}
				else {
					// ����
					ptChk[0] = pDataChain->GetNativePoint(1) -
								pDataChain->GetNativePoint(0);
				}
				if ( (q=ptChk[0].arctan()) < 0.0f )
					q += PI2;
				if ( q >= qq ) {
					// ������X�����傫���Ƃ�����
					// �[����_����O�a�I�_�܂Ő���
					dxfLine.s = *pts;
					dxfLine.e = pData->GetNativePoint(1);
					pDataNew = new CDXFline(&dxfLine);
					pDataNew->OrgTuning(FALSE);
					g_obMakeLine[io].Add(pDataNew);
				}
			}
		}
	}	// End of g_obLineTemp

	return IsThread();
}

BOOL MakeInsideCode(const CPointF& ptMax)
{
	INT_PTR		i, nLoop = g_obMakeLine[0].GetSize();
	float		dCutX;
	CPointF		pt, pts, pte, ptPull, ptMov;
	CDXFdata*	pData;
	CDXFchain*	pChain;
	COutlineData*	pOutline;
	CNCMakeLathe*	pNCD;

	// �����ݒ�
	CString	strCode( CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_I_SPINDLE)) );
	if ( !strCode.IsEmpty() )
		AddMakeLatheStr(strCode);
	ptPull.x = GetDbl(MKLA_DBL_I_PULLZ);
	ptPull.y = GetDbl(MKLA_DBL_I_PULLX);

	// ���Ѻ���
	if ( !GetStr(MKLA_STR_I_CUSTOM).IsEmpty() )
		AddMakeLatheStr(GetStr(MKLA_STR_I_CUSTOM));
	// NCVC�̓��a�؍�w��
	AddMakeLatheStr( '('+CString(INSIDE_S)+')' );

	// �擪�ް��̎n�_�Ɉړ�
	if ( nLoop > 0 ) {
		pData = g_obMakeLine[0][0];
		pt = pData->GetStartMakePoint();
		if ( pt.x < CNCMakeLathe::ms_xyz[NCA_Z] ) {
			// �H����ʒu���[�ʂ̉E��
			// ���w��ʒu�܂Œ����ړ�
			pNCD = new CNCMakeLathe(0, pt, 0.0f);
		}
		else {
			// �H����ʒu���[�ʂ̍���
			// ��Z���ړ���AX���ړ�
			pNCD = new CNCMakeLathe(ZXMOVE, pt, GetDbl(MKLA_DBL_I_FEED));
		}
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// �擪�ް��̐؍��߽
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_I_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// �r���H�ٰ߽��
	for ( i=1; i<nLoop && IsThread(); i++ ) {
		SetProgressPos(i+1);
		pData = g_obMakeLine[0][i];
		pt = pte = pData->GetStartMakePoint();
		dCutX = pt.y;
		pts.x = CNCMakeLathe::ms_xyz[NCA_Z];
		pts.y = CNCMakeLathe::ms_xyz[NCA_X];
		pts += CDXFdata::ms_ptOrg;		// ���݈ʒu
		pte += CDXFdata::ms_ptOrg;		// ���̈ړ��ʒu
		pte.y += ptPull.y;
		// pData�̎n�_�����݈ʒu�̂ǂ���ɂ��邩�ň������ς���
		if ( CNCMakeLathe::ms_xyz[NCA_Z]<pt.x && !CheckXZMove(0, pts, pte) ) {
			// ���̎n�_���E���A���A�֊s�̾�ĂɏՓ˂��Ȃ� �� ���݈ʒu����
			pt.y = CNCMakeLathe::ms_xyz[NCA_X];
		}
		pt.y -= ptPull.y;				// �����㕪�����Ĉړ�
		pNCD = new CNCMakeLathe(XZMOVE, pt, GetDbl(MKLA_DBL_I_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// �n�_�ւ̐؂荞��
		if ( ptMax.x < pt.x ) {
			// �[�ʂ����E�� �� �������X�������Ɉړ�
			pNCD = new CNCMakeLathe(0, NCA_X, dCutX, 0.0f);
		}
		else {
			// �[�ʂ������� �� �؍푗���X�������ɐ؂荞��
			pNCD = new CNCMakeLathe(1, NCA_X, dCutX, GetDbl(MKLA_DBL_I_FEEDX));
		}
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// �؍��߽
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_I_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// �d�グ�����ٰ߽��
	pOutline = g_pShape[0]->GetLatheList();
	for ( i=0; i<pOutline->GetSize() && IsThread(); i++ ) {
		pChain = pOutline->GetAt(i);
		// �ړ��߽
		if ( !pChain->IsEmpty() )
			MoveInsideCode(pChain, ptMax, ptPull);
		// �؍��߽
		PLIST_FOREACH(pData, pChain)
			pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_I_FEED));
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		END_FOREACH
	}

	// �`��d�グ
	pChain = g_pShape[0]->GetShapeChain();
	ASSERT(pChain);
	if ( !pChain->IsEmpty() )
		ptMov = MoveInsideCode(pChain, ptMax, ptPull);
	PLIST_FOREACH(pData, pChain)
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_I_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	END_FOREACH

	// �H����ʒu�֕��A
	if ( !pChain->IsEmpty() ) {
		// X��(Y)�����̗��E��Z��(X)�����̈ړ�
		pNCD = new CNCMakeLathe(XZMOVE, ptMov, GetDbl(MKLA_DBL_I_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		pNCD = new CNCMakeLathe(0, g_pDoc->GetCircleObject()->GetStartMakePoint(), 0.0f);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// ���a�؍�I������
	AddMakeLatheStr( '('+CString(ENDINSIDE_S)+')' );

	return IsThread();
}

BOOL MakeOutsideCode(const CPointF& ptMax)
{
	INT_PTR		i, nLoop = g_obMakeLine[1].GetSize(),
				iPosBase = g_obMakeLine[0].GetSize();
	float		dCutX;
	CPointF		pt, pts, pte, ptPull;
	CDXFdata*	pData;
	CDXFchain*	pChain;
	COutlineData*	pOutline;
	CNCMakeLathe*	pNCD;

	// �����ݒ�
	CString	strCode( CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_O_SPINDLE)) );
	if ( !strCode.IsEmpty() )
		AddMakeLatheStr(strCode);
	ptPull.x = GetDbl(MKLA_DBL_O_PULLZ);
	ptPull.y = GetDbl(MKLA_DBL_O_PULLX);

	// ���Ѻ���
	if ( !GetStr(MKLA_STR_O_CUSTOM).IsEmpty() )
		AddMakeLatheStr(GetStr(MKLA_STR_O_CUSTOM));

	// �擪�ް��̎n�_�Ɉړ�
	if ( nLoop > 0 ) {
		if ( CNCMakeLathe::ms_xyz[NCA_X] < ptMax.y ) {
			// �[�ʂ̉���
			if ( CNCMakeLathe::ms_xyz[NCA_Z] < ptMax.x ) {
				// �[�ʂ̍���(���������a���z��)
				pNCD = new CNCMakeLathe(0, NCA_Z, ptMax.x+GetDbl(MKLA_DBL_O_PULLZ), 0.0f);	// Z���ޔ�
				ASSERT(pNCD);
				g_obMakeData.Add(pNCD);
			}
			pNCD = new CNCMakeLathe(0, NCA_X, ptMax.y+GetDbl(MKLA_DBL_O_PULLX), 0.0f);		// X���ޔ�
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
		pData = g_obMakeLine[1][0];
		pts = pData->GetStartMakePoint();
		pNCD = new CNCMakeLathe(0, NCA_Z, pts.x, 0.0f);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		if ( CNCMakeLathe::ms_xyz[NCA_X] > ptMax.y ) {
			pNCD = new CNCMakeLathe(0, NCA_X, ptMax.y+GetDbl(MKLA_DBL_O_PULLX), 0.0f);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
		pNCD = new CNCMakeLathe(1, NCA_X, pts.y, GetDbl(MKLA_DBL_O_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// �擪�ް��̐؍��߽
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_O_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// �r���H�ٰ߽��
	for ( i=1; i<nLoop && IsThread(); i++ ) {
		SetProgressPos(i+iPosBase+1);
		pData = g_obMakeLine[1][i];
		pt = pte = pData->GetStartMakePoint();
		dCutX = pt.y;
		pts.x = CNCMakeLathe::ms_xyz[NCA_Z];
		pts.y = CNCMakeLathe::ms_xyz[NCA_X];
		pts += CDXFdata::ms_ptOrg;		// ���݈ʒu
		pte += CDXFdata::ms_ptOrg;		// ���̈ړ��ʒu
		pte.y += ptPull.y;
		// pData�̎n�_�����݈ʒu�̂ǂ���ɂ��邩�ň������ς���
		if ( CNCMakeLathe::ms_xyz[NCA_Z]<pt.x && !CheckXZMove(1, pts, pte) ) {
			// ���̎n�_���E���A���A�֊s�̾�ĂɏՓ˂��Ȃ� �� ���݈ʒu����
			pt.y = CNCMakeLathe::ms_xyz[NCA_X];
		}
		else {
			// ���̎n�_������ �� �O�a����
			pt.y = ptMax.y; 
		}
		pt.y += ptPull.y;				// �����㕪�����Ĉړ�
		pNCD = new CNCMakeLathe(XZMOVE, pt, GetDbl(MKLA_DBL_O_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// �n�_�ւ̐؂荞��
		if ( ptMax.x < pt.x ) {
			// �[�ʂ����E�� �� �������X�������Ɉړ�
			pNCD = new CNCMakeLathe(0, NCA_X, dCutX, 0.0f);
		}
		else {
			// �[�ʂ������� �� �؍푗���X�������ɐ؂荞��
			pNCD = new CNCMakeLathe(1, NCA_X, dCutX, GetDbl(MKLA_DBL_O_FEEDX));
		}
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// �؍��߽
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_O_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// �d�グ�����ٰ߽��
	pOutline = g_pShape[1]->GetLatheList();
	for ( i=0; i<pOutline->GetSize() && IsThread(); i++ ) {
		pChain = pOutline->GetAt(i);
		// �ړ��߽
		if ( !pChain->IsEmpty() )
			MoveOutsideCode(pChain->GetHead(), ptMax, ptPull);
		// �؍��߽
		PLIST_FOREACH(pData, pChain)
			pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_O_FEED));
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		END_FOREACH
	}

	// �`��d�グ
	pChain = g_pShape[1]->GetShapeChain();
	ASSERT(pChain);
	if ( !pChain->IsEmpty() )
		MoveOutsideCode(pChain->GetHead(), ptMax, ptPull);
	PLIST_FOREACH(pData, pChain)
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_O_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	END_FOREACH

	// �H����ʒu�֕��A
	if ( !pChain->IsEmpty() ) {
		dCutX = ptMax.y + ptPull.y;
		pt = g_pDoc->GetCircleObject()->GetStartMakePoint();
		pNCD = new CNCMakeLathe(1, NCA_X, dCutX, GetDbl(MKLA_DBL_O_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		if ( dCutX < pt.y ) {
			pNCD = new CNCMakeLathe(0, NCA_X, pt.y, 0.0f);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
		pNCD = new CNCMakeLathe(0, NCA_Z, pt.x, 0.0f);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		if ( pt.y < dCutX ) {
			pNCD = new CNCMakeLathe(0, NCA_X, pt.y, 0.0f);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
	}

	return IsThread();
}

BOOL MakeGrooveCode(const CPointF& ptMax)
{
	INT_PTR		i;
	CDXFsort	arDXFdata;
	CDXFdata*	pData;
	CNCMakeLathe*	pNCD;
	CPointF		pt, pts, pte;
	float		dPullX = ptMax.y + GetDbl(MKLA_DBL_G_PULLX),
				dWidth = GetDbl(MKLA_DBL_GROOVEWIDTH);

	// �f�[�^����
	for ( i=0; i<g_pGrooveLayer->GetDxfSize(); i++ ) {
		pData = g_pGrooveLayer->GetDxfData(i);
		if ( pData->GetType() == DXFLINEDATA ) {
			pData->OrgTuning(FALSE);
			arDXFdata.Add(pData);
		}
	}
	if ( arDXFdata.IsEmpty() )
		return IsThread();
	arDXFdata.Sort(GrooveSortFunc);	// Z��(X�l)�̑傫�����ɕ��בւ�

	// �����ݒ�
	CString	strCode( CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_G_SPINDLE)) );
	if ( !strCode.IsEmpty() )
		AddMakeLatheStr(strCode);

	// ���Ѻ���
	if ( !GetStr(MKLA_STR_G_CUSTOM).IsEmpty() )
		AddMakeLatheStr(GetStr(MKLA_STR_G_CUSTOM));
	// NCVC�̓˂��؂�؍�w��
	CString	strTool;
	strTool.Format(IDS_MAKENCD_FORMAT, GetDbl(MKLA_DBL_GROOVEWIDTH));
	switch ( GetNum(MKLA_NUM_GROOVETOOL) ) {
	case 1:		// �H���_����
		strTool = 'C'+strTool;
		break;
	case 2:		// �E
		strTool = 'R'+strTool;
		break;
	}
	AddMakeLatheStr( '('+CString(GROOVE_S)+'='+strTool+')' );

	// �˂��؂�f�[�^����
	for ( i=0; i<arDXFdata.GetSize(); i++ ) {
		pData = arDXFdata[i];
		pts = pData->GetStartMakePoint();
		pte = pData->GetEndMakePoint();
		if ( pts.x < pte.x )
			swap(pts, pte);
		// X���̒����ɂ���Đ������@��ς���
		if ( dWidth < pts.x-pte.x ) {
			// Z���X���C�h
			switch ( GetNum(MKLA_NUM_GROOVETOOL) ) {
			case 1:
				pts.x -= dWidth / 2.0f;
				pte.x += dWidth / 2.0f;
				break;
			case 2:
				pte.x += dWidth;
				break;
			default:
				pts.x -= dWidth;
			}
			pNCD = new CNCMakeLathe;
			ASSERT(pNCD);
			pNCD->CreateGroove(pts, pte, dPullX);
			g_obMakeData.Add(pNCD);
		}
		else {
			// X���̂�(�a)
			switch ( GetNum(MKLA_NUM_GROOVETOOL) ) {
			case 1:
				pt.x = (pts.x - pte.x) / 2.0f + pte.x;
				pt.y = (pts.y - pte.y) / 2.0f + pte.y;
				break;
			case 2:
				pt = pts;
				break;
			default:
				pt = pte;
			}
			pNCD = new CNCMakeLathe;
			ASSERT(pNCD);
			pNCD->CreateGroove(pt, dPullX);
			g_obMakeData.Add(pNCD);
		}
	}

	// �H����ʒu�֕��A
	pt = g_pDoc->GetCircleObject()->GetStartMakePoint();
	pNCD = new CNCMakeLathe(0, NCA_X, pt.y, 0.0f);
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);
	pNCD = new CNCMakeLathe(0, NCA_Z, pt.x, 0.0f);
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);

	// �˂��؂�؍�I������
	AddMakeLatheStr( '('+CString(ENDGROOVE_S)+')' );

	return IsThread();
}

BOOL CheckXZMove(int io, const CPointF& pts, const CPointF& pte)
{
	INT_PTR		i, nLoop;
	BOOL		bResult = FALSE;
	CPointF		pt[4];
	COutlineData*	pOutline;
	CDXFchain*		pChain;
	CDXFdata*		pData;

	// �֊s�̾�ĂƂ̌�_������
	if ( g_pShape[io] ) {
		pOutline = g_pShape[io]->GetLatheList();
		nLoop = pOutline->IsEmpty() ? 1 : pOutline->GetSize();
		for ( i=0; i<nLoop && !bResult && IsThread(); i++ ) {
			pChain = pOutline->IsEmpty() ? g_pShape[io]->GetShapeChain() : pOutline->GetAt(i);
			PLIST_FOREACH(pData, pChain)
				if ( pData->GetIntersectionPoint(pts, pte, pt, FALSE) > 0 ) {
					bResult = TRUE;
					break;
				}
			END_FOREACH
		}
	}

	return bResult;
}

CPointF MoveInsideCode(const CDXFchain* pChain, const CPointF& ptMax, const CPointF& ptPull)
{
	CNCMakeLathe*	pNCD;

	// �`��̈�ԉ����{������̍��W�v�Z
	CRect3F	rc( pChain->GetMaxRect() );
	CPointF	pt1(rc.TopLeft()), pt2(rc.BottomRight()), pt;
	pt1 -= CDXFdata::ms_ptOrg;
	pt2 -= CDXFdata::ms_ptOrg;
	pt.x = ptMax.x + ptPull.x;
	pt.y = min(pt1.y, pt2.y) - ptPull.y;

	// X��(Y)�����̗��E��Z��(X)�����̈ړ�
	pNCD = new CNCMakeLathe(XZMOVE, pt, GetDbl(MKLA_DBL_I_FEEDX));
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);

	// �擪�f�[�^��X���Ɉړ�
	CDXFdata*	pData = pChain->GetHead();
	CPointF	pts(pData->GetStartMakePoint());
	pNCD = new CNCMakeLathe(1, NCA_X, pts.y, 0.0f);
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);

	return pt;
}

void MoveOutsideCode(const CDXFdata* pData, const CPointF& ptMax, const CPointF& ptPull)
{
	CNCMakeLathe*	pNCD;
	CPointF		pts(pData->GetStartMakePoint()),
				pt(pts);

	// X��(Y)�����̗��E��Z��(X)�����̈ړ�
	if ( CNCMakeLathe::ms_xyz[NCA_Z] <= pt.x )
		pt.x = ptMax.x + ptPull.x;		// �[�ʁ{������
	pt.y = ptMax.y + ptPull.y;	// Z���͵�޼ު�Ă̊J�n�ʒu, X���͊O�a
	pNCD = new CNCMakeLathe(XZMOVE, pt, GetDbl(MKLA_DBL_O_FEEDX));
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);

	// X���n�_�Ɉړ�
	pNCD = new CNCMakeLathe(ptMax.x < pt.x ? 0 : 1, NCA_X, pts.y, GetDbl(MKLA_DBL_O_FEEDX));
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);
	if ( ptMax.x < pt.x ) {
		// ��޼ު�Ă̎n�_�Ɉړ�
		pNCD = new CNCMakeLathe(1, NCA_Z, pts.x, GetDbl(MKLA_DBL_O_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}
}

//////////////////////////////////////////////////////////////////////

// ̪��ޏo��
void SendFaseMessage
	(INT_PTR nRange/*=-1*/, int nMsgID/*=-1*/, LPCTSTR lpszMsg/*=NULL*/)
{
#ifdef _DEBUG
	printf("MakeLathe_Thread() Phase%d Start\n", g_nFase);
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

//	AddCustomLatheCode() ����Ăяo��
class CMakeCustomCode_Lathe : public CMakeCustomCode	// MakeCustomCode.h
{
public:
	CMakeCustomCode_Lathe(void) :
				CMakeCustomCode(g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "SPINDLE",
			"LatheDiameter", "LatheZmax", "LatheZmin",
			"ToolPosX", "ToolPosZ"
		};
		// ���ް�ǉ�
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
	}

	CString	ReplaceCustomCode(const string& str) {
		int		nTestCode;
		CString	strResult;

		// ���׽�Ăяo��
		tie(nTestCode, strResult) = CMakeCustomCode::ReplaceCustomCode(str);
		if ( !strResult.IsEmpty() )
			return strResult;

		// �h��replace
		switch ( nTestCode ) {
		case 0:		// ProgNo
			if ( GetFlg(MKLA_FLG_PROG) )
				strResult.Format(IDS_MAKENCD_PROG,
					GetFlg(MKLA_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKLA_NUM_PROG));
			break;
		case 1:		// G90orG91
			strResult = CNCMakeBase::MakeCustomString(GetNum(MKLA_NUM_G90)+90);
			break;
		case 2:		// SPINDLE
			// -- �����H���̉�]���Ő���
			if ( GetFlg(MKLA_FLG_ENDFACE) ) {
				// �[�ʉ�]��
				strResult = CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_E_SPINDLE));
			}
			else if ( !GetStr(MKLA_STR_DRILL).IsEmpty() ) {
				// ������]��
				VLATHEDRILLINFO	v;
				if ( g_pMakeOpt->GetDrillInfo(v) )
					strResult = CNCMakeLathe::MakeSpindle(v[0].s);
			}
			else if ( g_pShape[0] ) {
				// ���a��]��
				strResult = CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_I_SPINDLE));
			}
			else if ( g_pShape[1] ) {	// �ŏ���ͯ�ްٰ�߂Ȃ̂� else if ��OK
				// �O�a��]��
				strResult = CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_O_SPINDLE));
			}
			else if ( g_pGrooveLayer ) {
				// �ː؉�]��
				strResult = CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_G_SPINDLE));
			}
			break;
		case 3:		// LatheDiameter
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(1)->GetStartMakePoint().y * 2.0);
			break;
		case 4:		// LatheZmax
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(0)->GetStartMakePoint().x);
			break;
		case 5:		// LatheZmin
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(0)->GetEndMakePoint().x);
			break;
		case 6:		// ToolPosX
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetCircleObject()->GetStartMakePoint().y * 2.0);
			break;
		case 7:		// ToolPosZ
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetCircleObject()->GetStartMakePoint().x);
			break;
		default:
			strResult = str.c_str();
		}

		return strResult;
	}
};

void AddCustomLatheCode(const CString& strFileName)
{
	CString	strBuf, strResult;
	CMakeCustomCode_Lathe	custom;
	string	str, strTok;
	tokenizer<custom_separator>	tokens(str);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText | CFile::osSequentialScan);
		while ( fp.ReadString(strBuf) && IsThread() ) {
			str = strBuf;
			tokens.assign(str);
			strResult.Empty();
			BOOST_FOREACH(strTok, tokens) {
				strResult += custom.ReplaceCustomCode(strTok);
			}
			if ( !strResult.IsEmpty() )
				AddMakeLatheStr(strResult);
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// ���̴װ�͐�������(�x���̂�)
	}
}

int ShapeSortFunc(CDXFshape* pFirst, CDXFshape* pSecond)
{
	int		nResult;
	CPointF	pt1( pFirst->GetShapeChain()->GetHead()->GetNativePoint(0) ),
			pt2( pSecond->GetShapeChain()->GetHead()->GetNativePoint(0) );
	float	x = RoundUp(pt1.x - pt2.x);
	if ( x == 0.0f )
		nResult = 0;
	else if ( x > 0.0f )
		nResult = -1;
	else
		nResult = 1;
	return nResult;
}

int GrooveSortFunc(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	CPointF	pt1s( pFirst->GetStartMakePoint() ),  pt1e( pFirst->GetEndMakePoint() ),
			pt2s( pSecond->GetStartMakePoint() ), pt2e( pSecond->GetEndMakePoint() );
	float	sx = pt1s.x > pt1e.x ? pt1s.x : pt1e.x,
			ex = pt2s.x > pt2e.x ? pt2s.x : pt2e.x;
	if ( sx == ex )
		nResult = 0;
	else if ( sx > ex )
		nResult = -1;
	else
		nResult = 1;
	return nResult;
}

//////////////////////////////////////////////////////////////////////

// NC�����̸�۰��ٕϐ�������(��n��)�گ��
UINT MakeLathe_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

	int			i, j;
#ifdef _DEBUG
	printf("MakeLathe_AfterThread() Start\n");
#endif

	for ( j=0; j<SIZEOF(g_obLineTemp); j++ ) {
		for ( i=0; i<g_obLineTemp[j].GetSize(); i++ )
			delete	g_obLineTemp[j][i];
		g_obLineTemp[j].RemoveAll();
	}
	for ( j=0; j<SIZEOF(g_obMakeLine); j++ ) {
		for ( i=0; i<g_obMakeLine[j].GetSize(); i++ )
			delete	g_obMakeLine[j][i];
		g_obMakeLine[j].RemoveAll();
	}
	for ( i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	g_obMakeData.RemoveAll();
	for ( i=0; i<g_pDoc->GetLayerCnt(); i++ )
		g_pDoc->GetLayerData(i)->RemoveAllShape();

	g_csMakeAfter.Unlock();

	return 0;
}
