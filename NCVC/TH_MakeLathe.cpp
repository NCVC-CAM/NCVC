// TH_MakeNCD.cpp
// DXF->���՗pNC����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCdata.h"
#include "NCMakeLatheOpt.h"
#include "NCMakeLathe.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

/*
!!!ATTENTION!!!
�������Ԃ̕\���F�����ذ��ł͊O���̂�Y�ꂸ��
#define	_DBG_NCMAKE_TIME
*/

using namespace std;
using namespace boost;

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	SetProgressPos(a)	g_pParent->m_ctReadProgress.SetPos(a)

// ��۰��ٕϐ���`
static	CThreadDlg*			g_pParent;
static	CDXFDoc*			g_pDoc;
static	CNCMakeLatheOpt*	g_pMakeOpt;

// NC�����ɕK�v���ް��Q
static	CShapeArray	g_obShape;
static	CDXFarray	g_obOutsideTemp;
static	CDXFarray	g_obLathePass;
static	CTypedPtrArrayEx<CPtrArray, CNCMakeLathe*>	g_obMakeGdata;	// ���H�ް�

// ��ފ֐�
static	void	InitialVariable(void);		// �ϐ�������
static	void	SetStaticOption(void);		// �ÓI�ϐ��̏�����
static	BOOL	MakeLathe_MainFunc(void);	// NC������Ҳ�ٰ��
static	BOOL	CreateShapeThread(void);	// �`��F������
static	void	InitialShapeData(void);		// �`��F���̏�����
static	BOOL	CreateOutsidePitch(void);	// �O�a�̾�Ă𒆐S�܂Ő���
static	BOOL	CreateRoughPass(void);		// �r���H�ް��̐���
static	BOOL	MakeLatheCode(void);		// NC���ނ̐���
static	void	MoveLatheCode(const CDXFdata*, double, double);
static	BOOL	OutputNCcode(void);			// NC���ނ̏o��

// ͯ�ް,̯�ް���̽�߼�ٺ��ސ���
static	void	AddCustomCode(const CString&);

// �C���ް��̐���
inline	void	AddMakeLatheStr(const CString& strData)
{
	CNCMakeLathe*	mkNCD = new CNCMakeLathe(strData);
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
}

// ̪��ލX�V
static	int		g_nFase;			// ̪��އ�
static	void	SendFaseMessage(int = -1, int = -1, LPCTSTR = NULL);

// ��޽گ�ފ֐�
static	CCriticalSection	g_csMakeAfter;	// MakeLathe_AfterThread()�گ��ۯ���޼ު��
static	UINT	MakeLathe_AfterThread(LPVOID);	// ��n���گ��

//////////////////////////////////////////////////////////////////////
// ���՗pNC�����گ��
//////////////////////////////////////////////////////////////////////

UINT MakeLathe_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLathe_Thread()\nStart", DBG_GREEN);
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
		BOOL	bResult = FALSE;
		// NC������߼�ݵ�޼ު�Ă̐����Ƶ�߼�݂̓ǂݍ���
		g_pMakeOpt = new CNCMakeLatheOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetLatheInitList()->GetHead());
		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		InitialVariable();
		// �������Ƃɕω��������Ұ���ݒ�
		SetStaticOption();
		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
		// �������蓖��
		g_obMakeGdata.SetSize(0, 1024);		// �����ް������͖��m��
		// �����J�n
		bResult = MakeLathe_MainFunc();
		if ( bResult )
			bResult = OutputNCcode();

		// �߂�l���
		if ( bResult && IsThread() )
			nResult = IDOK;
#ifdef _DEBUG
		dbg.printf("MakeLathe_Thread All Over!!!");
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
	for ( int i=0; i<g_obShape.GetSize(); i++ )
		g_obShape[i]->CrearScanLine_Lathe();
	g_obShape.RemoveAll();

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
	// �[�ʂ̏I�_�����_
	CDXFdata::ms_ptOrg = g_pDoc->GetLatheLine(1)->GetNativePoint(1);
	// ORIGIN�ް����H����ʒu
	CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	// �e��޼ު�Ă̌��_����
	g_pDoc->GetCircleObject()->OrgTuning(FALSE);
	g_pDoc->GetLatheLine(0)->OrgTuning(FALSE);
	g_pDoc->GetLatheLine(1)->OrgTuning(FALSE);
	// ������߼�݂̏�����
	CNCMakeLathe::InitialVariable();
}

void SetStaticOption(void)
{
	// CDXFdata�̐ÓI�ϐ�������
	CDXFdata::ms_fXRev = CDXFdata::ms_fYRev = FALSE;

	// CNCMakeMill�̐ÓI�ϐ�������
	CPointD	ptOrg(g_pDoc->GetCircleObject()->GetStartMakePoint());
	CNCMakeLathe::ms_xyz[NCA_X] = ptOrg.y;
	CNCMakeLathe::ms_xyz[NCA_Y] = 0;
	CNCMakeLathe::ms_xyz[NCA_Z] = ptOrg.x;

	// ������߼�݂ɂ��ÓI�ϐ��̏�����
	CNCMakeLathe::SetStaticOption(g_pMakeOpt);
}

BOOL OutputNCcode(void)
{
	CString	strPath, strFile,
			strNCFile(g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_obMakeGdata.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		CStdioFile	fp(strNCFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
#ifdef _DEBUG
		for ( int i=0; i<g_obMakeGdata.GetSize(); i++ ) {
#else
		for ( int i=0; i<g_obMakeGdata.GetSize() && IsThread(); i++ ) {
#endif
			g_obMakeGdata[i]->WriteGcode(fp);
			SetProgressPos(i+1);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	SetProgressPos(g_obMakeGdata.GetSize());
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
	// �O�a�ް�����؍폀���ް����쐬
	if ( !CreateOutsidePitch() )
		return FALSE;

	// ̪���1 : �`��̵̾�ĂƊO�a�̵̾�Ă̌�_���v�Z���r���H���ް��𐶐�
	SendFaseMessage(g_obOutsideTemp.GetSize());
	if ( !CreateRoughPass() )
		return FALSE;

	// ̪���2 : NC���ނ̐���
	SendFaseMessage(g_obLathePass.GetSize()+GetNum(MKLA_NUM_MARGIN));
	if ( !MakeLatheCode() )
		return FALSE;

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

BOOL CreateShapeThread(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateShapeThread()\nStart");
	CDXFchain*	pChainDbg;
	CDXFdata*	pDataDbg;
	POSITION	posdbg;
	CPointD		ptsd, pted;
#endif
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

	// ���Ր����ł���W�������o���ꂽ���H
	int		i, j, nLoop;
	const int	nLayerLoop = g_pDoc->GetLayerCnt();
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFmap*	pMap;

	for ( i=0; i<nLayerLoop && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutType() )
			continue;
#ifdef _DEBUG
		dbg.printf(" Layer=%s", pLayer->GetStrLayer());
#endif
		nLoop = pLayer->GetShapeSize();
		for ( j=0; j<nLoop && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			pMap   = pShape->GetShapeMap();
			if ( !pMap || !(pShape->GetShapeFlag()&DXFMAPFLG_EDGE) )
				continue;	// ���̎��_��CDXFchain�Ȃ米�
			// CDXFmap -> CDXFchain �ϊ�
			if ( pShape->ChangeCreate_MapToChain() ) {
#ifdef _DEBUG
				pChainDbg = pShape->GetShapeChain();
				dbg.printf("  ShapeNo.%d ChainCnt=%d", i, pChainDbg->GetCount());
				for ( posdbg=pChainDbg->GetHeadPosition(); posdbg; ) {
					pDataDbg = pChainDbg->GetNext(posdbg);
					ptsd = pDataDbg->GetNativePoint(0);
					pted = pDataDbg->GetNativePoint(1);
					dbg.printf("%d (%.3f, %.3f)-(%.3f, %.3f)", pDataDbg->GetType(),
						ptsd.x, ptsd.y, pted.x, pted.y);
				}
#endif
				delete	pMap;
				// �����Ώی`���ۑ�
				g_obShape.Add(pShape);
			}
		}
	}

	return !g_obShape.IsEmpty();
}

void InitialShapeData(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("InitialShapeData()\nStart");
#endif
	int			i;
	POSITION	pos;
	CPointD		pts, pte;
	CDXFchain	ltTemp;
	CDXFchain*	pChain;

	for ( i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		pChain = g_obShape[i]->GetShapeChain();
		ASSERT(pChain);
		pts = pChain->GetHead()->GetNativePoint(0);
		pte = pChain->GetTail()->GetNativePoint(1);
		// Z�l(X)���傫������擪���ނ�
		if ( pts.x < pte.x ) {
#ifdef _DEBUG
			dbg.printf("(pts.x < pte.x) Object Reverse");
#endif
			ltTemp.RemoveAll();
			for ( pos=pChain->GetHeadPosition(); pos && IsThread(); )
				ltTemp.AddHead(pChain->GetNext(pos));
			pChain->RemoveAll();
			pChain->AddTail(&ltTemp);
			pChain->ReverseNativePt();
		}
		// �����𐳂������Ă��猴�_����
		pChain->OrgTuning();
	}
}

BOOL CreateOutsidePitch(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateOutsidePitch()\nStart");
	CPointD		ptsd, pted;
	int			nCntDbg = 0;
#endif
	int			i, j,
				n = GetNum(MKLA_NUM_MARGIN);
	double		d = GetDbl(MKLA_DBL_MARGIN);
	POSITION	pos;
	COutlineData*	pOutline;
	CDXFchain*	pChain;
	CDXFdata*	pData;
	CRectD		rc;
	double		dLimit = DBL_MAX,
				cutD  = fabs(GetDbl(MKLA_DBL_CUT));

	// �`��̾�Ĉ�ԊO���̉����l�����߂�
	for ( i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		// �`��̵̾�Ă𐶐� + ��ԊO���̵̾�Ă��擾
		g_obShape[i]->CreateScanLine_Lathe(n, d);
		pOutline = g_obShape[i]->GetLatheList();
		// n==0�̑Ώ�
		pChain = pOutline->IsEmpty() ?
			g_obShape[i]->GetShapeChain() : pOutline->GetHead();
		for ( pos=pChain->GetHeadPosition(); pos && IsThread(); ) {
			pData = pChain->GetNext(pos);
#ifdef _DEBUG
			ptsd = pData->GetNativePoint(0);
			pted = pData->GetNativePoint(1);
			rc = pData->GetMaxRect();
			dbg.printf("%d (%.3f, %.3f)-(%.3f, %.3f)", pData->GetType(),
				ptsd.x, ptsd.y, pted.x, pted.y);
#else
			rc = pData->GetMaxRect();
#endif
			if ( dLimit > rc.top )
				dLimit = rc.top;
		}
		// �������ނ̌��_����
		for ( j=0; j<pOutline->GetSize() && IsThread(); j++ ) {
			pChain = pOutline->GetAt(j);
			ASSERT(pChain);
			pChain->OrgTuning();
		}
	}
#ifdef _DEBUG
	dbg.printf("Limit=%f", dLimit);
#endif

	// �O�a�̾�Ă𒆐S�܂Ő���
	CDXFline*	pLine = g_pDoc->GetLatheLine(0);	// �O�a��޼ު��
	CPointD	pts(pLine->GetNativePoint(0)),
			pte(pLine->GetNativePoint(1));
	pts.x += GetDbl(MKLA_DBL_PULL_Z);		// �����㕪
	pts.y -= cutD;
	pte.y -= cutD;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// ���_(zero)�܂�X����(Y�l)��؂荞��
	while ( pts.y>dLimit && pte.y>dLimit && IsThread() ) {
		dxfLine.s = pts;
		dxfLine.e = pte;
		pLine = new CDXFline(&dxfLine);
		g_obOutsideTemp.Add(pLine);		// OrgTuning()�͕s�v
		pts.y -= cutD;
		pte.y -= cutD;
#ifdef _DEBUG
		nCntDbg++;
#endif
	}
#ifdef _DEBUG
	dbg.printf("OffsetCnt=%d", nCntDbg);
#endif

	return IsThread();
}

BOOL CreateRoughPass(void)
{
	int			i, j,
				nResult;
	BOOL		bCreate, bInter;
	ENDXFTYPE	enType;
	POSITION	pos;
	double		q, qq;
	CPointD		ptChk[4];
	optional<CPointD>	pts;
	CDXFdata*	pData;
	CDXFdata*	pDataChain;
	CDXFdata*	pDataNew;
	CDXFchain*	pChain;
	COutlineData*	pOutline;
	CDXFarc*	pArc;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// �O�a����̌X�����v�Z
	if ( !g_obOutsideTemp.IsEmpty() ) {
		pData = g_obOutsideTemp[0];
		ptChk[0] = pData->GetNativePoint(1) - pData->GetNativePoint(0);
		if ( (qq=atan2(ptChk[0].y, ptChk[0].x)) < 0 )
			qq += 360.0*RAD;
	}

	// �O�a�����ް���ٰ�߂����r���H�ް����쐬
	for ( i=0; i<g_obOutsideTemp.GetSize() && IsThread(); i++ ) {
		SetProgressPos(i+1);
		pData = g_obOutsideTemp[i];
		pts = pData->GetNativePoint(0);
		pDataChain = NULL;
		bInter = FALSE;		// ��x�ł���_�������TRUE
		for ( j=0; j<g_obShape.GetSize() && IsThread(); j++ ) {
			// ��ԊO���̌`��̾�Ăƌ�_����
			pOutline = g_obShape[j]->GetLatheList();
			pChain = pOutline->IsEmpty() ?
				g_obShape[j]->GetShapeChain() : pOutline->GetHead();
			for ( pos=pChain->GetHeadPosition(); pos && IsThread(); ) {
				bCreate = FALSE;
				pDataChain = pChain->GetNext(pos);
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
					g_obLathePass.Add(pDataNew);
					bInter = TRUE;
				}
			}			// End of Chain Loop
		}				// End of Shape Loop
		// ��_�[������
		if ( pts ) {
			if ( !bInter ) {
				// ��_���Ȃ���ΊO�a�I�_�܂Ő���
				dxfLine.s = *pts;
				dxfLine.e = pData->GetNativePoint(1);
				pDataNew = new CDXFline(&dxfLine);
				pDataNew->OrgTuning(FALSE);
				g_obLathePass.Add(pDataNew);
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
				if ( (q=atan2(ptChk[0].y, ptChk[0].x)) < 0 )
					q += 360.0*RAD;
				if ( q >= qq ) {
					// ������X�����傫���Ƃ�����
					// �[����_����O�a�I�_�܂Ő���
					dxfLine.s = *pts;
					dxfLine.e = pData->GetNativePoint(1);
					pDataNew = new CDXFline(&dxfLine);
					pDataNew->OrgTuning(FALSE);
					g_obLathePass.Add(pDataNew);
				}
			}
		}
	}	// End of g_obOutsideTemp

	return IsThread();
}

BOOL MakeLatheCode(void)
{
	int			i, j, nLoop = g_obLathePass.GetSize();
	POSITION	pos;
	double		dCutX;
	CPointD		pt;
	CDXFchain*	pChain;
	CDXFdata*	pData;
	COutlineData*	pOutline;
	CNCMakeLathe*	mkNCD;

	// �[�ʂƊO�a�̍ő�l���擾
	double		dMaxZ = g_pDoc->GetLatheLine(0)->GetStartMakePoint().x,
				dMaxX = g_pDoc->GetLatheLine(1)->GetStartMakePoint().y;

	// �擪�ް��̎n�_�Ɉړ�
	if ( nLoop > 0 ) {
		// G����ͯ��(�J�n����)
		AddCustomCode(g_pMakeOpt->GetStr(MKLA_STR_HEADER));
		//
		pData = g_obLathePass[0];
		pt = pData->GetStartMakePoint();
		if ( pt.x < CNCMakeLathe::ms_xyz[NCA_Z] ) {
			// �H����ʒu���[�ʂ̉E��
			// ���w��ʒu�܂Œ����ړ�
			mkNCD = new CNCMakeLathe(pt);
		}
		else {
			// �H����ʒu���[�ʂ̍���
			// ��Z���ړ���AX���ړ�
			mkNCD = new CNCMakeLathe(ZXMOVE, pt);
		}
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		// �擪�ް��̐؍��߽
		mkNCD = new CNCMakeLathe(pData);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
	}

	// �r���H�ٰ߽��
	for ( i=1; i<nLoop && IsThread(); i++ ) {
		SetProgressPos(i+1);
		pData = g_obLathePass[i];
		pt = pData->GetStartMakePoint();
		dCutX = pt.y;
		// pData�̎n�_�����݈ʒu�̂ǂ���ɂ��邩�ň������ς���
		if ( CNCMakeLathe::ms_xyz[NCA_Z] < pt.x ) {
			// ���̎n�_���E�� �� ���݈ʒu����
			pt.y = CNCMakeLathe::ms_xyz[NCA_X];
		}
		else {
			// ���̎n�_������ �� �O�a����
			pt.y = dMaxX; 
		}
		pt.y += GetDbl(MKLA_DBL_PULL_X);	// �����㕪�����Ĉړ�
		mkNCD = new CNCMakeLathe(XZMOVE, pt);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		// �n�_�ւ̐؂荞��
		if ( dMaxZ < pt.x ) {
			// �[�ʂ����E�� �� �������X�������Ɉړ�
			mkNCD = new CNCMakeLathe(0, NCA_X, dCutX);
		}
		else {
			// �[�ʂ������� �� �؍푗���X�������ɐ؂荞��
			mkNCD = new CNCMakeLathe(1, NCA_X, dCutX);
		}
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		// �؍��߽
		mkNCD = new CNCMakeLathe(pData);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
	}

	// �d�グ�����ٰ߽��
	for ( i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		pOutline = g_obShape[i]->GetLatheList();
		for ( j=0; j<pOutline->GetSize() && IsThread(); j++ ) {
			pChain = pOutline->GetAt(j);
			// �ړ��߽
			if ( !pChain->IsEmpty() )
				MoveLatheCode(pChain->GetHead(), dMaxZ, dMaxX);
			// �؍��߽
			for ( pos=pChain->GetHeadPosition(); pos && IsThread(); ) {
#ifdef _DEBUG
				pData = pChain->GetNext(pos);
				mkNCD = new CNCMakeLathe(pData);
#else
				mkNCD = new CNCMakeLathe(pChain->GetNext(pos));
#endif
				ASSERT(mkNCD);
				g_obMakeGdata.Add(mkNCD);
			}
		}
	}

	// �d�グ�ٰ߽��
	for ( i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		pChain = g_obShape[i]->GetShapeChain();
		ASSERT(pChain);
		// �ړ��߽
		if ( !pChain->IsEmpty() )
			MoveLatheCode(pChain->GetHead(), dMaxZ, dMaxX);
		// �؍��߽
		for ( pos=pChain->GetHeadPosition(); pos && IsThread(); ) {
#ifdef _DEBUG
			pData = pChain->GetNext(pos);
			mkNCD = new CNCMakeLathe(pData);
#else
			mkNCD = new CNCMakeLathe(pChain->GetNext(pos));
#endif
			ASSERT(mkNCD);
			g_obMakeGdata.Add(mkNCD);
		}
	}

	if ( !g_obMakeGdata.IsEmpty() ) {
		// �H����ʒu�֕��A
		dCutX = dMaxX + GetDbl(MKLA_DBL_PULL_X);
		pt = g_pDoc->GetCircleObject()->GetStartMakePoint();
		mkNCD = new CNCMakeLathe(1, NCA_X, dCutX);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		if ( dCutX < pt.y ) {
			mkNCD = new CNCMakeLathe(0, NCA_X, pt.y);
			ASSERT(mkNCD);
			g_obMakeGdata.Add(mkNCD);
		}
		mkNCD = new CNCMakeLathe(0, NCA_Z, pt.x);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		if ( pt.y < dCutX ) {
			mkNCD = new CNCMakeLathe(0, NCA_X, pt.y);
			ASSERT(mkNCD);
			g_obMakeGdata.Add(mkNCD);
		}
		// G����̯��(�I������)
		AddCustomCode(g_pMakeOpt->GetStr(MKLA_STR_FOOTER));
	}

	return IsThread();
}

void MoveLatheCode(const CDXFdata* pData, double dMaxZ, double dMaxX)
{
	CNCMakeLathe*	mkNCD;
	CPointD		pts(pData->GetStartMakePoint()),
				pt(pts);

	// X��(Y)�����̗��E��Z��(X)�����̈ړ�
	if ( CNCMakeLathe::ms_xyz[NCA_Z] < pt.x ) {
		pt.x = dMaxZ + GetDbl(MKLA_DBL_PULL_Z);	// �[�ʁ{������
		pt.y = CNCMakeLathe::ms_xyz[NCA_X];
	}
	else
		pt.y = dMaxX;	// Z���͵�޼ު�Ă̊J�n�ʒu, X���͊O�a
	pt.y += GetDbl(MKLA_DBL_PULL_X);	// ������
	mkNCD = new CNCMakeLathe(XZMOVE, pt);
	ASSERT(mkNCD);
	g_obMakeGdata.Add(mkNCD);
	// X���n�_�Ɉړ�
	mkNCD = new CNCMakeLathe(dMaxZ < pt.x ? 0 : 1, NCA_X, pts.y);
	ASSERT(mkNCD);
	g_obMakeGdata.Add(mkNCD);
	if ( dMaxZ < pt.x ) {
		// ��޼ު�Ă̎n�_�Ɉړ�
		mkNCD = new CNCMakeLathe(1, NCA_Z, pts.x);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
	}
}

//////////////////////////////////////////////////////////////////////

// ̪��ޏo��
void SendFaseMessage
	(int nRange/*=-1*/, int nMsgID/*=-1*/, LPCTSTR lpszMsg/*=NULL*/)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLathe_Thread()", DBG_GREEN);
	dbg.printf("Phase%d Start", g_nFase);
#endif
	if ( nRange > 0 )
		g_pParent->m_ctReadProgress.SetRange32(0, nRange);

	CString	strMsg;
	if ( nMsgID > 0 )
		VERIFY(strMsg.LoadString(nMsgID));
	else
		strMsg.Format(IDS_MAKENCD_FASE, g_nFase);
	g_pParent->SetFaseMessage(strMsg, lpszMsg);
	g_nFase++;
}

//	AddCustomCode() parse()�֐�����Ăяo��
class CMakeCustomCode_Lathe : public CMakeCustomCode	// MakeCustomCode.h
{
public:
	CMakeCustomCode_Lathe(CString& r) :
				CMakeCustomCode(r, g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "SPINDLE",
			"LatheDiameter", "LatheZmax", "LatheZmin",
			"ToolPosX", "ToolPosZ"
		};
		// ���ް�ǉ�
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
	}

	void	operator()(const char* s, const char* e) const {
		CString	strTmp;

		// ���׽�Ăяo��
		int		nTestCode = CMakeCustomCode::ReplaceCustomCode(s, e);

		// replace
		switch ( nTestCode ) {
		case 0:		// ProgNo
			if ( GetFlg(MKLA_FLG_PROG) ) {
				strTmp.Format(IDS_MAKENCD_PROG,
					GetFlg(MKLA_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKLA_NUM_PROG));
				m_strResult += strTmp;
			}
			break;
		case 1:		// G90orG91
			m_strResult += CNCMakeBase::MakeCustomString(GetNum(MKLA_NUM_G90)+90);
			break;
		case 2:		// SPINDLE
			m_strResult += CNCMakeLathe::MakeSpindle();
			break;
		case 3:		// LatheDiameter
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(1)->GetStartMakePoint().y * 2.0);
			m_strResult += strTmp;
			break;
		case 4:		// LatheZmax
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(0)->GetStartMakePoint().x);
			m_strResult += strTmp;
			break;
		case 5:		// LatheZmin
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(0)->GetEndMakePoint().x);
			m_strResult += strTmp;
			break;
		case 6:		// ToolPosX
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetCircleObject()->GetStartMakePoint().y * 2.0);
			m_strResult += strTmp;
			break;
		case 7:		// ToolPosZ
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetCircleObject()->GetStartMakePoint().x);
			m_strResult += strTmp;
			break;
		}
	}
};

void AddCustomCode(const CString& strFileName)
{
	using namespace boost::spirit::classic;

	CString	strBuf;
	CString	strResult;
	CMakeCustomCode_Lathe	custom(strResult);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) && IsThread() ) {
			// �\�����
			strResult.Empty();
			if ( parse((LPCTSTR)strBuf, *( *(anychar_p - '{')[custom] >> comment_p('{', '}')[custom] ) ).hit ) {
				if ( !strResult.IsEmpty() )
					AddMakeLatheStr( strResult );
			}
			else
				AddMakeLatheStr( strBuf );
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// ���̴װ�͐�������(�x���̂�)
	}
}

//////////////////////////////////////////////////////////////////////

// NC�����̸�۰��ٕϐ�������(��n��)�گ��
UINT MakeLathe_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

	int			i;
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLathe_AfterThread()\nStart", TRUE, DBG_RED);
#endif

	for ( i=0; i<g_obOutsideTemp.GetSize(); i++ )
		delete	g_obOutsideTemp[i];
	for ( i=0; i<g_obLathePass.GetSize(); i++ )
		delete	g_obLathePass[i];
	for ( i=0; i<g_obMakeGdata.GetSize(); i++ )
		delete	g_obMakeGdata[i];
	for ( i=0; i<g_pDoc->GetLayerCnt(); i++ )
		g_pDoc->GetLayerData(i)->RemoveAllShape();

	g_obOutsideTemp.RemoveAll();
	g_obLathePass.RemoveAll();
	g_obMakeGdata.RemoveAll();

	g_csMakeAfter.Unlock();

	return 0;
}
