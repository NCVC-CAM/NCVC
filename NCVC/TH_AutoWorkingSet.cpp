// TH_AutoWorkingSet.cpp
// �������H�w��
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"	// DXFView.h
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "DXFView.h"	// DXFTREETYPE
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::vector;
using namespace boost;

static	CThreadDlg*	g_pParent;
#define	IsThread()	g_pParent->IsThreadContinue()
static	inline	void	_SetProgressRange(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetRange32(0, (int)n);
}
static	inline	void	_SetProgressPos(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetPos((int)n);
}

//	SeparateOutline_Thread
struct	SEPARATEOUTLINETHREADPARAM
{
	CEvent		evStart;			// ٰ�ߊJ�n�����
	CEvent		evEnd;				// �I���҂��m�F
	BOOL		bThread;			// �گ�ނ̌p���׸�
	CDXFworkingOutline*	pOutline;	// �Ώۗ֊s��޼ު�ďW��
	// SEPARATEOUTLINETHREADPARAM::CEvent ���蓮����Ăɂ��邽�߂̺ݽ�׸�
	SEPARATEOUTLINETHREADPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE), bThread(TRUE), pOutline(NULL) {}
};
typedef	SEPARATEOUTLINETHREADPARAM*		LPSEPARATEOUTLINETHREADPARAM;
static	UINT	SeparateOutline_Thread(LPVOID);

static	void	AutoWorkingProc(const CLayerData*, LPVOID);
static	void	AutoRecalcWorking(const CLayerData*, LPVOID);
static	void	CheckStrictOffset(const CLayerData*, LPVOID);

static	BOOL	SelectOutline(const CLayerData*);
static	BOOL	SelectPocket(const CLayerData*);
static	void	SelectPocket_Reflex(const CLayerData*, INT_PTR, int);
static	void	CreateAutoWorking(const CLayerData*, float);
static	BOOL	CreateScanLine(CDXFshape*, LPAUTOWORKINGDATA);
static	void	CheckStrictOffset_forScan(const CLayerData*);
static	BOOL	CheckOffsetIntersection(CDXFchain*, CDXFchain*, BOOL);
static	void	CheckCircleIntersection(const CLayerData*, const CDXFshape*, const CDXFworkingOutline*);
static	CDXFdata*	ChangeCircleToArc(const CDXFcircle*, const CDXFchain*, CPointF&, CPointF&);
static	void	SetAllExcludeData(CDXFchain*);
static	CDXFworkingOutline*	GetOutlineHierarchy(CDXFshape*, INT_PTR);
static	CDXFworkingOutline*	GetOutlineLastObj(CDXFshape*, INT_PTR);

//////////////////////////////////////////////////////////////////////
//	�������H�w���گ��
//////////////////////////////////////////////////////////////////////

UINT AutoWorkingSet_Thread(LPVOID pThread)
{
	LPNCVCTHREADPARAM	pVoid = reinterpret_cast<LPNCVCTHREADPARAM>(pThread);
	g_pParent = pVoid->pParent;
	CDXFDoc*	pDoc = static_cast<CDXFDoc*>(pVoid->pDoc);
	INT_PTR			i;
	const INT_PTR	nLoop = pDoc->GetLayerCnt();
	int				nType = (int)(pVoid->wParam);	// ��������
	DXFTREETYPE*	vSelect = NULL;
	LPVOID			pParam = NULL;
	function<void (const CLayerData*, LPVOID)>	pfnAutoProc;

	switch ( nType ) {
	case AUTOWORKING:
		pfnAutoProc = &AutoWorkingProc;
		pParam = reinterpret_cast<LPVOID>(pVoid->lParam);			// �޲�۸ޏ��
		break;
	case AUTORECALCWORKING:
		pfnAutoProc = &AutoRecalcWorking;
		vSelect = reinterpret_cast<DXFTREETYPE*>(pVoid->lParam);	// �Čv�Z���̌`��W��
		pParam = vSelect;
		break;
	case AUTOSTRICTOFFSET:
		pfnAutoProc = &CheckStrictOffset;
		pParam = reinterpret_cast<LPVOID>(TRUE);		// not NULL (������ٰ�߂��K�v)
		break;
	default:
		g_pParent->PostMessage(WM_USERFINISH, IDCANCEL);
		return 0;
	}

	CString		strMsg;
	CLayerData*	pLayer;

	VERIFY(strMsg.LoadString(IDS_AUTOWORKING));

	// ڲԂ��Ƃɍ��Wϯ�߂�����
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pLayer = pDoc->GetLayerData(i);
		if ( !pLayer->IsCutType() || 
				(vSelect && vSelect->which()==DXFTREETYPE_LAYER && pLayer!=get<CLayerData*>(*vSelect)) )
			continue;
		g_pParent->SetFaseMessage(strMsg, pLayer->GetLayerName());
		// ���ߕʂ̎�������
		pfnAutoProc(pLayer, pParam);
		//
		if ( vSelect && vSelect->which()==DXFTREETYPE_LAYER )
			break;	// ڲԎw�肠��Έȍ~�K�v�Ȃ�
	}

	g_pParent->PostMessage(WM_USERFINISH, IsThread() ? IDOK : IDCANCEL);
	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
//	�������H�w��
//		�֊s�E�߹�Ă̏����I��
void AutoWorkingProc(const CLayerData* pLayer, LPVOID pParam)
{
	int		i;
	LPAUTOWORKINGDATA	pAuto = reinterpret_cast<LPAUTOWORKINGDATA>(pParam);	// �޲�۸ޏ��

	pLayer->AllShapeClearSideFlg();

	// �����ɉ��������O����
	if ( pAuto->nSelect == 0 )
		SelectOutline(pLayer);
	else
		SelectPocket(pLayer);

	// �֊s����
	for ( i=0; i<pAuto->nLoopCnt && IsThread(); i++ ) {
		CreateAutoWorking(pLayer, pAuto->dOffset*(i+1));
		CheckStrictOffset(pLayer, NULL);
	}

	// �߹�ĉ��H��
	if ( pAuto->nSelect == 1 ) {
		CDXFshape*	pShape;
		// ����������
		for ( i=0; i<pLayer->GetShapeSize() && IsThread(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			if ( pShape->GetShapeFlag() & DXFMAPFLG_INSIDE )
				CreateScanLine(pShape, pAuto);
		}
		// ������������Ώۂɂ����̾�ē��m�̌�_����
		CheckStrictOffset_forScan(pLayer);
		// �����̗֊s��޼ު�Ă𑩂˂�
	}
}

//////////////////////////////////////////////////////////////////////
//	�̾�Ēl�ύX�ɂ��Čv�Z
void AutoRecalcWorking(const CLayerData* pLayer, LPVOID pParam)
{
	DXFTREETYPE*	vSelect = reinterpret_cast<DXFTREETYPE*>(pParam);		// �Čv�Z���̌`��W��
	INT_PTR			i, j, nOutline;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	int			nInOut;
	float		dOffset, dOffsetOrg;
	CDXFshape*	pShape;
	CDXFshape*	pShapeSrc = NULL;
	CDXFchain	ltOutline;
	COutlineList*		pOutlineList;
	CDXFworkingOutline*	pOutline;

	// �ΏۏW��
	if ( vSelect && vSelect->which()==DXFTREETYPE_SHAPE )
		pShapeSrc = get<CDXFshape*>(*vSelect);

	_SetProgressRange(nLoop);

	try {
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			// ��۸�ڽ�ް
			_SetProgressPos(i);
			// �`��W�����w�肳��Ă���΁A�����ϯ�������̂���
			if ( pShapeSrc && pShapeSrc != pShape )
				continue;
			// ���ɂ�����H�w���𒊏o
			pOutlineList = pShape->GetOutlineList();
			nOutline = pOutlineList->GetCount();
			if ( nOutline <= 0 )
				continue;
			dOffset = dOffsetOrg = pShape->GetOffset();
			nInOut = pShape->GetInOutFlag();
			// ���ݓo�^����Ă�������ŗ֊s��޼ު�Đ���
			for ( j=0; j<nOutline; j++ ) {
				if ( !pShape->CreateOutlineTempObject(nInOut, &ltOutline, dOffset) ) {
					// �I�I���s�I�I�ꎞ��޼ު�đS�폜
					PLIST_FOREACH(CDXFdata* pData, &ltOutline)
						if ( pData )
							delete	pData;
					END_FOREACH
					continue;
				}
				// ���H�w���o�^
				if ( !ltOutline.IsEmpty() ) {
					pOutline = new CDXFworkingOutline(pShape, &ltOutline, dOffset, DXFWORKFLG_AUTO);
					pShape->AddOutlineData(pOutline, nInOut);
				}
				// ����ٰ�߂ɔ�����
				pOutline = NULL;
				dOffset += dOffsetOrg;
				ltOutline.RemoveAll();
			}
			if ( pShapeSrc )
				break;	// �`��w�肠��Έȍ~�K�v�Ȃ�
		}
	}
	catch ( CMemoryException* e ) {
		if ( pOutline )
			delete	pOutline;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	_SetProgressPos(nLoop);

	// �̾�ĵ�޼ު�ē��m�̌�_����
	CheckStrictOffset(pLayer, reinterpret_cast<LPVOID>(TRUE));	// ������ٰ�߂��K�v
}

//////////////////////////////////////////////////////////////////////
//	�̾�ĵ�޼ު�ē��m�̌�_���������A�����ȵ̾�Ă��v�Z����

void CheckStrictOffset(const CLayerData* pLayer, LPVOID pParam)
{
	INT_PTR			i, j, ii, jj, n, nMax = 0;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	float			dOffset1, dOffset2;
	CRect3F			rc, rcCross;
	CDXFshape*		pShape1;
	CDXFshape*		pShape2;
	CDXFworkingOutline*	pOutline1;
	CDXFworkingOutline*	pOutline2;
	CDXFchain*		pChain1;
	CDXFchain*		pChain2;
	function<CDXFworkingOutline* (CDXFshape*, INT_PTR)>	pfnGetOutline;

	// �̾�ĵ�޼ު�ĕ����گ�ދN��
	SEPARATEOUTLINETHREADPARAM	thSepParam;
	CWinThread*	pSepThread = AfxBeginThread(SeparateOutline_Thread, &thSepParam);
	if ( !pSepThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	_SetProgressRange(nLoop);
	thSepParam.evEnd.SetEvent();

	if ( pParam ) {
		// ���Ԗڂ̗֊s��޼ު�Ă��擾
		pfnGetOutline = &GetOutlineHierarchy;
		// �K�w�̍ő吔���擾
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			ii = pLayer->GetShapeData(i)->GetOutlineList()->GetCount();
			if ( nMax < ii )
				nMax = ii;
		}
	}
	else {
		// �Ō�ɓo�^���ꂽ�֊s��޼ު�Ă��擾
		pfnGetOutline = &GetOutlineLastObj;
		nMax = 1;
	}

	// Ҳ�ٰ��
	for ( n=0; n<nMax && IsThread(); n++ ) {		// �K�wٰ��
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pShape1 = pLayer->GetShapeData(i);
			_SetProgressPos(i);
			pOutline1 = pfnGetOutline(pShape1, n);	// �����ɉ������֊s��޼ު�Ď擾
			if ( !pOutline1 )
				continue;
			dOffset1 = pOutline1->GetOutlineOffset();
			for ( ii=0; ii<pOutline1->GetOutlineSize() && IsThread(); ii++ ) {
				pChain1 = pOutline1->GetOutlineObject(ii);
				rc = pChain1->GetMaxRect();
				for ( j=i+1; j<nLoop && IsThread(); j++ ) {
					pShape2 = pLayer->GetShapeData(j);
					pOutline2 = pfnGetOutline(pShape2, n);
					if ( !pOutline2 )
						continue;
					dOffset2 = pOutline2->GetOutlineOffset();
					for ( jj=0; jj<pOutline2->GetOutlineSize() && IsThread(); jj++ ) {
						pChain2 = pOutline2->GetOutlineObject(jj);
						if ( rcCross.CrossRect(rc, pChain2->GetMaxRect()) ) {
#ifdef _DEBUG
							printf("ShapeName1=%s No.%d Obj=%d\n", LPCTSTR(pShape1->GetShapeName()),
								ii, pChain1->GetSize());
							printf("ShapeName2=%s No.%d Obj=%d\n", LPCTSTR(pShape2->GetShapeName()),
								jj, pChain2->GetSize());
#endif
							// ���O����̂��߂��׸ސݒ�
							pChain1->ClearSideFlg();
							pChain1->SetChainFlag( pChain1->GetChainFlag() | 
								(pShape1->GetShapeFlag()&(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) );
							pChain2->ClearSideFlg();
							pChain2->SetChainFlag( pChain2->GetChainFlag() | 
								(pShape2->GetShapeFlag()&(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) );
							// �̾�ĵ�޼ު�Ă̕�������
							if ( CheckOffsetIntersection(pChain1, pChain2, FALSE) ) {
								pOutline1->SetMergeHandle(pShape2->GetShapeHandle());
								pOutline2->SetMergeHandle(pShape1->GetShapeHandle());
							}
						}
					}
				}
			}
			// �ŏI����
//			CheckCircleIntersection(pLayer, pShape1, pOutline1);
			// �����گ�ފJ�n
			thSepParam.evEnd.Lock();
			thSepParam.evEnd.ResetEvent();
			thSepParam.pOutline = pOutline1;
			thSepParam.evStart.SetEvent();
		}
	}

	// �گ�ޏI���w��
	thSepParam.evEnd.Lock();
	thSepParam.evEnd.ResetEvent();
	thSepParam.bThread = FALSE;
	thSepParam.evStart.SetEvent();
	WaitForSingleObject(pSepThread->m_hThread, INFINITE);

	_SetProgressPos(nLoop);

#ifdef _DEBUG
	// �������̊m�F
	extern	LPCTSTR	gg_szCat;		// ", "
	POSITION	dbgPos;
	CPointF		dbgPts, dbgPte;
	BOOL		dbgConnect;
	CDXFdata*	dbgData;
	CString		dbgMsg;
	for ( i=0; i<nLoop; i++ ) {
		pShape1 = pLayer->GetShapeData(i);
		pOutline1  = pShape1->GetOutlineLastObj();
		if ( !pOutline1 )
			continue;
		printf("Name=%s Sep=%d Merge=%d\n",
			LPCTSTR(pShape1->GetShapeName()),
			pOutline1->GetOutlineSize(), pOutline1->GetMergeHandleSize());
		// �����֊s���ް�����
		for ( j=0; j<pOutline1->GetOutlineSize(); j++ ) {
			dbgConnect = TRUE;
			pChain1 = pOutline1->GetOutlineObject(j);
			dbgPos = pChain1->GetHeadPosition();
			if ( !dbgPos )
				continue;
			dbgPte = pChain1->GetNext(dbgPos)->GetNativePoint(1);
			while ( dbgPos ) {
				dbgData = pChain1->GetNext(dbgPos);
				dbgPts = dbgData->GetNativePoint(0);
				if ( sqrt(GAPCALC(dbgPts-dbgPte)) >= NCMIN ) {
					dbgConnect = FALSE;
					break;
				}
				dbgPte = dbgData->GetNativePoint(1);
			}
			if ( !dbgConnect ) {
				printf("---> No.%d Connect Error!\n", j);
				PLIST_FOREACH(dbgData, pChain1)
					dbgPts = dbgData->GetNativePoint(0);
					dbgPte = dbgData->GetNativePoint(1);
					printf("(%.3f, %.3f)-(%.3f, %.3f)\n",
						dbgPts.x, dbgPts.y, dbgPte.x, dbgPte.y);
				END_FOREACH
			}
			else if ( pChain1->GetChainFlag() & DXFMAPFLG_SEPARATE ) {
				dbgPts = pChain1->GetHead()->GetNativePoint(0);
				dbgPte = pChain1->GetTail()->GetNativePoint(1);
				if ( sqrt(GAPCALC(dbgPts-dbgPte)) < NCMIN )
					printf("---> No.%d Loop Outline ? size=%d\n", j, pChain1->GetSize());
			}
		}
		// ������
		if ( pOutline1->GetMergeHandleSize() > 0 ) {
			for ( j=0; j<pOutline1->GetMergeHandleSize(); j++ ) {
				if ( !dbgMsg.IsEmpty() )
					dbgMsg += gg_szCat;
				dbgMsg += pOutline1->GetMergeHandle(j);
			}
			printf("---> %s\n", LPCTSTR(dbgMsg));
			dbgMsg.Empty();
		}
	}
#endif
}

void CheckStrictOffset_forScan(const CLayerData* pLayer)
{
	INT_PTR			i, ii, j, jj;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	float		dOffset1, dOffset2;
	CRect3F		rc1, rc2, rcCross;
	CDXFshape*	pShape1;
	CDXFshape*	pShape2;
	CDXFworkingOutline*	pOutline1;
	CDXFworkingOutline*	pOutline2;
	CDXFchain*	pChain1;
	CDXFchain*	pChain2;

	// �̾�ĵ�޼ު�ĕ����گ�ދN��
	SEPARATEOUTLINETHREADPARAM	thSepParam;
	CWinThread*	pSepThread = AfxBeginThread(SeparateOutline_Thread, &thSepParam);
	if ( !pSepThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	_SetProgressRange(nLoop);
	thSepParam.evEnd.SetEvent();

	// Ҳ�ٰ��
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape1 = pLayer->GetShapeData(i);
		pOutline1 = pShape1->GetOutlineLastObj();
		_SetProgressPos(i);
		if ( !pOutline1 || !(pShape1->GetShapeFlag()&DXFMAPFLG_INSIDE) )
			continue;
		dOffset1 = pOutline1->GetOutlineOffset();
		for ( ii=0; ii<pOutline1->GetOutlineSize() && IsThread(); ii++ ) {
			pChain1 = pOutline1->GetOutlineObject(ii);
			rc1 = pChain1->GetMaxRect();
			for ( j=i+1; j<nLoop && IsThread(); j++ ) {
				pShape2 = pLayer->GetShapeData(j);
				pOutline2 = pShape2->GetOutlineLastObj();
				if ( !pOutline2 || !(pShape2->GetShapeFlag()&DXFMAPFLG_OUTSIDE) )
					continue;
				dOffset2 = pOutline2->GetOutlineOffset();
				for ( jj=0; jj<pOutline2->GetOutlineSize() && IsThread(); jj++ ) {
					pChain2 = pOutline2->GetOutlineObject(jj);
					rc2 = pChain2->GetMaxRect();
					if ( rc2.RectInRect(rc1) ) {
						// Inside���ް���Outside�̓����ɂ���̂ō폜
						SetAllExcludeData(pChain1);
					}
					else if ( rcCross.CrossRect(rc2, rc1) ) {	// ��`�̌������܂�
#ifdef _DEBUG
						printf("ShapeName1=%s No.%d Obj=%d\n", LPCTSTR(pShape1->GetShapeName()),
							ii, pChain1->GetSize());
						printf("ShapeName2=%s No.%d Obj=%d\n", LPCTSTR(pShape2->GetShapeName()),
							jj, pChain2->GetSize());
#endif
						pChain1->ClearSideFlg();
						pChain1->SetChainFlag( pChain1->GetChainFlag() | 
							(pShape1->GetShapeFlag()&(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) );
						pChain2->ClearSideFlg();
						pChain2->SetChainFlag( pChain2->GetChainFlag() | 
							(pShape2->GetShapeFlag()&(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) );
						// INSIDE��������_����
						if ( CheckOffsetIntersection(pChain1, pChain2, TRUE) )
							pOutline1->SetMergeHandle(pShape2->GetShapeHandle());	// ���Ԃ�g��Ȃ��ް�
					}
				}
			}
		}
		// �����گ�ފJ�n
		thSepParam.evEnd.Lock();
		thSepParam.evEnd.ResetEvent();
		thSepParam.pOutline = pOutline1;
		thSepParam.evStart.SetEvent();
	}

	// �گ�ޏI���w��
	thSepParam.evEnd.Lock();
	thSepParam.evEnd.ResetEvent();
	thSepParam.bThread = FALSE;
	thSepParam.evStart.SetEvent();
	WaitForSingleObject(pSepThread->m_hThread, INFINITE);

	_SetProgressPos(nLoop);

#ifdef _DEBUG
	// �������̊m�F
	COutlineList*	dbgOutlineList;
	for ( i=0; i<nLoop; i++ ) {
		pShape1 = pLayer->GetShapeData(i);
		if ( !pShape1->IsOutlineList() )
			continue;
		dbgOutlineList = pShape1->GetOutlineList();
		printf("Name=%s Outline=%d\n",
			LPCTSTR(pShape1->GetShapeName()), dbgOutlineList->GetCount());
		ii=0;
		PLIST_FOREACH(pOutline1, dbgOutlineList)
			printf("No.%d Sep=%d\n", ii++, pOutline1->GetOutlineSize());
		END_FOREACH
	}
#endif
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//	�����֊s���H�w��
//		���̵�޼ު�ĂɊ܂܂��ŏ�������޼ު�Ăɓ����̉��H�w���C
//		����ȊO�͊O���̉��H�w��
BOOL SelectOutline(const CLayerData* pLayer)
{
	INT_PTR			i, j;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	BOOL		bResult = FALSE;
	CDXFshape*	pShape;
	CDXFshape*	pShapeTmp;
	CRectF		rcBase;

	_SetProgressRange(nLoop);
	const_cast<CLayerData*>(pLayer)->AscendingShapeSort();	// �ʐςŏ������בւ�

	// ��L��`�̓��O����
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ��۸�ڽ�ް
		_SetProgressPos(i);	// �������Ȃ��̂łP�����X�V
		// ���������Ώۂ��ۂ�(CDXFchain* ������ΏۂƂ���)
		if ( pShape->GetShapeType()!=DXFSHAPETYPE_CHAIN || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		// rcBase�����傫�Ȗʐς����µ�޼ު��(i+1)��rcBase���܂ނ��ǂ����̔���
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType() != DXFSHAPETYPE_CHAIN )
				continue;
			if ( pShapeTmp->GetMaxRect().RectInRect(rcBase) )
				break;	// �Ȍ�̌����͕K�v�Ȃ�
		}
		pShape->SetShapeFlag(j<nLoop ? DXFMAPFLG_INSIDE : DXFMAPFLG_OUTSIDE);
		bResult = TRUE;		// �����ς�
	}

	_SetProgressPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	�����߹�ĉ��H�w��
//		�ő�O���ƍŏ������ɓ����̉��H�w��
BOOL SelectPocket(const CLayerData* pLayer)
{
	INT_PTR			i, j;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	BOOL		bResult = FALSE, bInRect;
	CDXFshape*	pShape;
	CDXFshape*	pShapeTmp;
	CRectF		rcBase;

	_SetProgressRange(nLoop);
	const_cast<CLayerData*>(pLayer)->DescendingShapeSort();	// �ʐςō~�����בւ�

	// �O������
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ��۸�ڽ�ް
		_SetProgressPos(i);
		// ���������Ώۂ��ۂ�(CDXFchain* ������ΏۂƂ���)
		if ( pShape->GetShapeType()!=DXFSHAPETYPE_CHAIN ||
				pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING ||
				pShape->IsSideFlg() )
			continue;
		bInRect = FALSE;
		// rcBase����޼ު�Ă��܂�ł��邩�ǂ����̔���
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType()!=DXFSHAPETYPE_CHAIN || pShapeTmp->IsSideFlg() )
				continue;
			if ( rcBase.RectInRect(pShapeTmp->GetMaxRect()) ) {
				// �ċA�Ăяo���ɂ��K�w����
				SelectPocket_Reflex(pLayer, i, 0);
				bInRect = TRUE;
			}
		}
		if ( !bInRect ) {
			// �P��
			pShape->SetShapeFlag(DXFMAPFLG_INSIDE);
		}
	}

	_SetProgressPos(nLoop);
	return bResult;
}

void SelectPocket_Reflex(const CLayerData* pLayer, INT_PTR i, int n)
{
	CDXFshape*	pShape = pLayer->GetShapeData(i++);	// i++ �Ŏ����ް�����
	CRectF		rcBase(pShape->GetMaxRect());

	// ���Y�W���̔���͊K�w�̐��l����
	pShape->SetShapeFlag( n&0x01 ? DXFMAPFLG_OUTSIDE : DXFMAPFLG_INSIDE);

	for ( ; i<pLayer->GetShapeSize() && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		if ( pShape->GetShapeType()!=DXFSHAPETYPE_CHAIN || pShape->IsSideFlg() )
			continue;
		if ( rcBase.RectInRect(pShape->GetMaxRect()) ) {
			// �ċA
			SelectPocket_Reflex(pLayer, i, n+1);
		}
	}
}

//////////////////////////////////////////////////////////////////////
//	���O���肩����H�w���̐���

void CreateAutoWorking(const CLayerData* pLayer, float dOffset)
{
	INT_PTR			i;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	size_t		n, j;
	float		dArea[2];
	DWORD		dwError;
	CRect3F		rcMax;
	CDXFchain	ltOutline[2];
	CDXFdata*	pData;
	CDXFshape*	pShape;
	CDXFworkingOutline*	pWork;

	_SetProgressRange(nLoop);

	try {
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pWork = NULL;
			pShape = pLayer->GetShapeData(i);
			// ��۸�ڽ�ް
			_SetProgressPos(i);
			// �����Ώ�����
			if ( !pShape->IsSideFlg() )
				continue;
#ifdef _DEBUG
			printf("ShapeName=%s\n", LPCTSTR(pShape->GetShapeName()));
			CRectF	rcDbg(pShape->GetMaxRect());
			printf("(%f, %f)-(%f, %f)\n", rcDbg.left, rcDbg.top, rcDbg.right, rcDbg.bottom);
#endif
			// �֊s�ꎞ��޼ު�Ă̐���
			dwError = 0;
			for ( j=0; j<SIZEOF(ltOutline) && IsThread(); j++ ) {
				if ( pShape->CreateOutlineTempObject((BOOL)j, &ltOutline[j], dOffset) ) {
					// ��`�̈�̑傫���v�Z
					rcMax = ltOutline[j].GetMaxRect();
				}
				else {
					// ���W���̋�`�̈���
					rcMax = pShape->GetMaxRect();
					dwError |= ( 1 << j );
				}
#ifdef _DEBUG
				printf("Outline (%f, %f)-(%f, %f)\n", rcMax.left, rcMax.top, rcMax.right, rcMax.bottom);
#endif
				dArea[j] = rcMax.Width() * rcMax.Height();
			}
			// ���O����`�̑傫���Ō���
			if ( pShape->GetShapeFlag() & DXFMAPFLG_INSIDE )
				j = dArea[0] > dArea[1] ? 1 : 0;	// �����������̗p
			else if ( pShape->GetShapeFlag() & DXFMAPFLG_OUTSIDE )
				j = dArea[0] > dArea[1] ? 0 : 1;	// �傫�������̗p
			else
				j = -1;
			switch ( dwError ) {
			case 1:		// �P��ڴװ
				if ( j == 0 )
					j = -1;
				break;
			case 2:		// �Q��ڴװ
				if ( j == 1 )
					j = -1;
				break;
			case 3:		// �����װ
				j = -1;
				break;
			}
			// ���H�w���o�^
			if ( j >= 0 ) {
				if ( !ltOutline[j].IsEmpty() ) {
					pWork = new CDXFworkingOutline(pShape, &ltOutline[j], dOffset, DXFWORKFLG_AUTO);
					pShape->AddOutlineData(pWork, (int)j);
#ifdef _DEBUG
					printf("Select OutLine = %d\n", j);
#endif
				}
				// Select����CDXFworkingOutline���޽�׸��ɂ�delete
				n = 1 - j;	// 1->0, 0->1
				PLIST_FOREACH(pData, &ltOutline[n])
					if ( pData )
						delete	pData;
				END_FOREACH
			}
			else {
				// �I�I���s�I�I�ꎞ��޼ު�đS�폜
				for ( n=0; n<SIZEOF(ltOutline); n++ ) {
					PLIST_FOREACH(pData, &ltOutline[n])
						if ( pData )
							delete	pData;
					END_FOREACH
				}
			}
			// ����ٰ�߂ɔ����A��`�̏�����
			for ( n=0; n<SIZEOF(ltOutline); n++ )
				ltOutline[n].RemoveAll();
		}
	}
	catch ( CMemoryException* e ) {
		if ( pWork )
			delete	pWork;
		for ( n=0; n<SIZEOF(ltOutline); n++ ) {
			PLIST_FOREACH(pData, &ltOutline[n])
				if ( pData )
					delete	pData;
			END_FOREACH
		}
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	_SetProgressPos(nLoop);
}

//////////////////////////////////////////////////////////////////////
//	�ő��`�̑������𐶐�

BOOL CreateScanLine(CDXFshape* pShape, LPAUTOWORKINGDATA pAuto)
{
	CDXFchain	ltScan;
	CDXFworkingOutline*	pWork;

	if ( pAuto->bCircleScroll ) {
		// �}�`�W�����P��̉~�ް��Ȃ罸۰ِ؍�p�ް��̐���
		CDXFchain*	pChain = pShape->GetShapeChain();
		if ( pChain && pChain->GetCount()==1 && pChain->GetHead()->GetType()==DXFCIRCLEDATA )
			pShape->CreateScanLine_ScrollCircle(&ltScan);
	}

	if ( ltScan.IsEmpty() ) {
		switch ( pAuto->nScanLine ) {
		case 1:		// �w����
			pShape->CreateScanLine_X(&ltScan);
			break;
		case 2:		// �x����
			pShape->CreateScanLine_Y(&ltScan);
			break;
		default:	// �Ȃ�(�֊s)
			pShape->CreateScanLine_Outline(&ltScan);
			break;
		}
	}

	if ( !ltScan.IsEmpty() ) {
		pWork = new CDXFworkingOutline(pShape, &ltScan, pShape->GetOffset(),
						DXFWORKFLG_AUTO|DXFWORKFLG_SCAN);
		pShape->AddOutlineData(pWork, 0);
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////

BOOL CheckOffsetIntersection
	(CDXFchain* pChain1, CDXFchain* pChain2, BOOL bScan)
{
	POSITION	pos1, pos2, pos;
	int			i, j, n;
	CPointF		pt[4], pts;
	CDXFdata*	pData1;
	CDXFdata*	pData2;

	// ��_���ꎞ�i�[
	struct INTER_INFO {
		POSITION	pos;		// �Y����޼ު�Ă�POSITION
		CPointF		pt;			// ��_���W
		int			nCnt;		// ���̌�_���܂ł̵�޼ު�Đ�
		float		dGap;		// �n�_�Ƃ̋���(���בւ��p)
		CDXFchain*	pChain;		// ����W��
		bool operator < (const INTER_INFO& src) const {	// sort�p
			return dGap < src.dGap;
		}
	};
	INTER_INFO			info;
	vector<INTER_INFO>	vTemp, vInfo[2];
	vector<INTER_INFO>::iterator	it;

	// ���W�̒x���X�V�p�\����
	struct	DELAYUPDATE {
		CDXFdata*	pData;		// �X�V�Ώ�
		CPointF		pt;			// �X�V���W
		int			n;			// �n�_���I�_��
	};
	DELAYUPDATE			d;
	vector<DELAYUPDATE>	v;

	// �̾�ĵ�޼ު�ē��m�̌�_����
	for ( i=0; i<SIZEOF(vInfo) && IsThread(); i++ ) {
		for ( pos1=pChain1->GetHeadPosition(); (pos=pos1) && IsThread(); ) {
			pData1 = pChain1->GetNext(pos1);
			if ( !pData1 )
				continue;
			pts = pData1->GetNativePoint(0);
			for ( pos2=pChain2->GetHeadPosition(); pos2 && IsThread(); ) {
				pData2 = pChain2->GetNext(pos2);
				if ( !pData2 )
					continue;
				n = pData1->GetIntersectionPoint(pData2, pt);
				for ( j=0; j<n && IsThread(); j++ ) {	// ��_��������
					info.pos	= pos;
					info.pt		= pt[j];
					info.nCnt	= 0;
					info.dGap	= GAPCALC(pt[j] - pts);
					info.pChain	= pChain2;
					vTemp.push_back(info);
				}
			}
			// �P�̵�޼ު�Ăɕ����̌�_������ꍇ��
			// �n�_����̋����ŕ��בւ�
			if ( vTemp.size() > 1 ) {
//				std::sort(vTemp.begin(), vTemp.end());
				boost::sort(vTemp);	// boost/range/algorithm.hpp
			}
			vInfo[i].insert(vInfo[i].end(), vTemp.begin(), vTemp.end());
			vTemp.clear();
		}
		// �ΏۏW���̐؂�ւ�
		if ( bScan )
			break;		// �������̏ꍇ�͕s�v
		swap(pChain1, pChain2);	// �������@�͓���
	}

	// ��{�I�Ɍ�_���͋����̃n�Y�₯�ǁA
	// ���ɍs��ꂽ��_���������Ŋ�̏ꍇ�����蓾��
	if ( vInfo[0].size()<=0 || (!bScan && vInfo[0].size()!=vInfo[1].size()) )
		return FALSE;

	// ��_�L�^����폜�͈͂̊m�F(�ȍ~��_���P�����̂Ƃ��͏������Ȃ�)
	if ( vInfo[0].size() > 1 ) {
		for ( i=0; i<SIZEOF(vInfo) && IsThread(); i++ ) {
			for ( it=vInfo[i].begin(); it!=vInfo[i].end() && IsThread(); ++it ) {
				pos1 = it->pos;
				pos2 = boost::next(it)!=vInfo[i].end() ? boost::next(it)->pos : vInfo[i][0].pos;
				ASSERT(pos1);
				ASSERT(pos2);
				for ( n=0; pos1!=pos2 && IsThread(); ) {
					pData1 = pChain1->GetNext(pos1);
					if ( pData1 )
						n++;
					if ( !pos1 )
						pos1 = pChain1->GetHeadPosition();
				}
				it->nCnt = n;	// ���̌�_�܂ł̵�޼ު�Đ�
			}
			// �ΏۏW���̐؂�ւ�
			if ( bScan )
				break;
			swap(pChain1, pChain2);
		}
		if ( !IsThread() )
			return FALSE;

		// �폜����(�ȍ~ IsThread() �͕s�v)
		for ( i=0; i<SIZEOF(vInfo); i++ ) {
			// �폜�J�n�ʒu�̌���(����W���ւ̓��O����)
			pos = vInfo[i][0].pos;
			ASSERT(pos);
			pts = pChain1->GetNext(pos)->GetNativePoint(1);		// ��_��޼ު�Ă̏I�_��
			if ( vInfo[i][0].nCnt == 0 ) {
				if ( pChain2->IsPointInPolygon(pts) ) {		// ����W���̓����ɂ���
					// �P�̵�޼ު�ĂŊт��đ���W���ɓ����_���Ȃ̂ŁA
					// ���̌�_��񂩂�J�n
					vInfo[i].push_back(vInfo[i][0]);	// �擪�v�f�𖖔��ɒǉ�����
					vInfo[i].erase(vInfo[i].begin());	// �擪�v�f���폜
				}
			}
			else {
				if ( !pChain2->IsPointInPolygon(pts) ) {	// ����W���̊O���ɂ���
					// ����W������o�Ă����悤�Ȍ�_���Ȃ̂ŁA
					// ���̌�_��񂩂�J�n
					vInfo[i].push_back(vInfo[i][0]);
					vInfo[i].erase(vInfo[i].begin());
				}
			}
			// ��_���Q��΂��ŁA���̊Ԃ̵�޼ު�Ă�����
			for ( it=vInfo[i].begin(); it!=vInfo[i].end(); it+=2 ) {
				if ( boost::next(it) == vInfo[i].end() )
					break;	// ����͏���
				pos1 = it->pos;
				pos2 = boost::next(it)->pos;
				ASSERT(pos1);
				ASSERT(pos2);
				// pos1 ��������޼ު�Ă̏I�_���X�V
				pData1 = pChain1->GetNext(pos1);
				if ( !pos1 )
					pos1 = pChain1->GetHeadPosition();
				ASSERT( pData1 );
				if ( !pData1->IsStartEqEnd() ) {
					pt[0] = pData1->GetNativePoint(1);			// �ύX�O�̏I�_
					d.pData	= pData1;
					d.pt	= it->pt;
					d.n		= 1;	// �I�_�ύXϰ�
					v.push_back(d);
				}
				// pos1 ���� pos2 �܂ŵ�޼ު�č폜
				if ( it->nCnt > 0 ) {
					while ( (pos=pos1) != pos2 ) {
						pData1 = pChain1->GetNext(pos1);
						if ( pData1 )
							pData1->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
						if ( !pos1 )
							pos1 = pChain1->GetHeadPosition();
					}
					pData1 = pChain1->GetAt(pos1);
					d.pData	= pData1;
					d.pt	= boost::next(it)->pt;
					d.n		= 0;	// �n�_�ύXϰ�
					v.push_back(d);
					pChain1->InsertBefore(pos1, (CDXFdata *)NULL);		// ����ϰ�
				}
				else {
					// �����޼ު�Ăɕ����̌�_
					if ( pData1->IsStartEqEnd() ) {
						// �~�E�ȉ~�̏ꍇ�́A�~�E�ȉ~�ǂɕϐg
						pData2 = pData1;	// delete�p
						pData1 = ChangeCircleToArc(static_cast<CDXFcircle *>(pData1),
							it->pChain, it->pt, boost::next(it)->pt);
						if ( pData1 ) {
							pData2->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
							pChain1->AddTail(pData1);
							pChain1->SetChainFlag(DXFMAPFLG_SEPARATE);	// SeparateModify() �ł͗��Ă��Ȃ��׸�
						}
					}
					else {
						pData1 = CreateDxfOffsetObject(pData1, boost::next(it)->pt, pt[0]);	// ���̌�_�ƕύX�O�I�_(CDXFshape)
						pos = pChain1->InsertBefore(pos1, pData1);		// ��޼ު�đ}��
						pChain1->InsertBefore(pos, (CDXFdata *)NULL);	// ����ϰ�
					}
				}
			}
			// �ΏۏW���̐؂�ւ�
			if ( bScan )
				break;
			swap(pChain1,  pChain2);
		}
	}

	// �]�������_(��_�P�������܂�)�ɑ΂��鏈��
	if ( vInfo[0].size() & 0x01 ) {
		for ( i=0; i<SIZEOF(vInfo); i++ ) {
			info = vInfo[i].back();
			pos  = info.pos;
			ASSERT(pos);
			pData1  = pChain1->GetNext(pos);
			d.pData	= pData1;
			d.pt	= info.pt;
			pts = pData1->GetNativePoint(0);
			pt[0] = (info.pt - pts) / 2.0 + pts;	// ��޼ު�Ă̎n�_�ƌ�_�̒��_��
			if ( pChain2->IsPointInPolygon(pt[0]) ) {		// ����W���̓����ɂ���
				d.n		= 0;
				v.push_back(d);
			}
			else {
				d.n		= 1;
				v.push_back(d);
				// �ȍ~�A�I�_������������Ȃ��Ƃ���܂ŵ�޼ު�č폜
				if ( !pos )
					pos = pChain1->GetHeadPosition();
				while ( TRUE ) {
					pData1 = pChain1->GetNext(pos);
					if ( !pos )
						pos = pChain1->GetHeadPosition();
					if ( pData1 ) {
						if ( pData1->GetDxfFlg() & DXFFLG_OFFSET_EXCLUDE )
							break;	// �ȍ~�̏����͕K�v�Ȃ�
						if ( pChain2->IsPointInPolygon(pData1->GetNativePoint(1)) )
							pData1->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
						else
							break;
					}
				}
			}

			// �ΏۏW���̐؂�ւ�
			if ( bScan )
				break;
			swap(pChain1,  pChain2);
		}
	}

	// �x���ް��̍��W�X�V
	BOOST_FOREACH(DELAYUPDATE& dd, v) {
		dd.pData->SetNativePoint(dd.n, dd.pt);
	}

#ifdef _DEBUG
	printf("CheckOffsetIntersection()\n");
	optional<CPointF>	ptDbgE;
	for ( i=0; i<SIZEOF(vInfo); i++ ) {
		ptDbgE.reset();
		PLIST_FOREACH(pData1, pChain1)
			if ( pData1 ) {
				pt[0] = pData1->GetNativePoint(0);
				pt[1] = pData1->GetNativePoint(1);
				if ( pData1->GetDxfFlg() & DXFFLG_OFFSET_EXCLUDE ) {
					printf("pt=(%.3f, %.3f) - (%.3f, %.3f) DEL\n",
						pt[0].x, pt[0].y, pt[1].x, pt[1].y);
				}
				else {
					printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
						pt[0].x, pt[0].y, pt[1].x, pt[1].y,
						ptDbgE && sqrt(GAPCALC(*ptDbgE-pt[0]))>=NCMIN ? "X" : " ");
					ptDbgE = pt[1];
				}
			}
			else {
				printf("null\n");
				ptDbgE.reset();
			}
		END_FOREACH
		printf("---\n");
		if ( bScan )
			break;
		swap(pChain1, pChain2);
	}
#endif

	return IsThread();
}

void CheckCircleIntersection
	(const CLayerData* pLayer, const CDXFshape* pShapeSrc, const CDXFworkingOutline* pOutline)
{
	INT_PTR		i, j,
				nShapeLoop = pLayer->GetShapeSize(),
				nOutlineLoop = pOutline->GetOutlineSize();
	float		dOffset = pOutline->GetOutlineOffset();
	CPointF		pts, pte;
	CRectF		rc(pOutline->GetMaxRect()), rcCross;
	CDXFshape*	pShape;

	//	�̾�ĵ�޼ު�Ă̍ŏI����
	for ( i=0; i<nShapeLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		if ( pShape==pShapeSrc || !pShape->IsOutlineList() )
			continue;
		if ( rcCross.CrossRect(rc, pShape->GetMaxRect()) ) {
			for ( j=0; j<nOutlineLoop && IsThread(); j++ ) {
				PLIST_FOREACH(CDXFdata* pData, pOutline->GetOutlineObject(j));
					if ( IsThread() )
						break;
					if ( pData && !(pData->GetDxfFlg()&DXFFLG_OFFSET_EXCLUDE) ) {
						pts = pData->GetNativePoint(0);
						pte = pData->GetNativePoint(1);
						if ( pShape->CheckIntersectionCircle(pts, dOffset) ) {
							pData->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
						}
						else if ( pShape->CheckIntersectionCircle(pte, dOffset) ) {
							pData->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
						}
					}
				END_FOREACH
			}
		}
	}
}

CDXFdata* ChangeCircleToArc
	(const CDXFcircle* pDataSrc, const CDXFchain* pChain, CPointF& pts, CPointF& pte)
{
	ENDXFTYPE	enType = pDataSrc->GetType();
	if ( enType!=DXFCIRCLEDATA && enType!=DXFELLIPSEDATA )
		return NULL;

	CDXFdata*	pData = NULL;
	CPointF		ptc(pDataSrc->GetCenter()), pt;
	float		sq, eq, q, r = pDataSrc->GetR();

	// ��]�����i�؂���~�ʁj�̌���
	if ( (sq=ptc.arctan(pts)) < 0.0f )
		sq += PI2;
	if ( (eq=ptc.arctan(pte)) < 0.0f )
		eq += PI2;
	while ( sq > eq )
		eq += PI2;		// sq < eq => ��{�̔����v���Ɋp�x�ݒ�
	q = (eq - sq) / 2.0f + sq;	// sq��eq�̂�����
	pt.x = r * cos(q);		pt.y = r * sin(q);
	pt += ptc;
	// ���肪�����������O���������Ő؂���~�ʂ�����
	if ( pChain->GetChainFlag() & DXFMAPFLG_INSIDE ) {
		if ( !pChain->GetMaxRect().PtInRect(pt) ) {
			// �p�x�̐^�񒆂̍��W������̏W����`�O�Ȃ�
			swap(sq, eq);		// �p�x�����ւ����Α��̉~�ʂŐ���
			swap(pts, pte);		// �ȉ~�Ǘp
		}
	}
	else {
		if ( pChain->GetMaxRect().PtInRect(pt) ) {
			// �p�x�̐^�񒆂̍��W������̏W����`���Ȃ�
			swap(sq, eq);		// �p�x�����ւ����Α��̉~�ʂŐ���
			swap(pts, pte);		// �ȉ~�Ǘp
		}
	}

	// �~�� �܂��� �ȉ~�ǂ̐���
	if ( enType == DXFCIRCLEDATA ) {
		DXFAARGV	dxfArc;
		dxfArc.pLayer = pDataSrc->GetParentLayer();
		dxfArc.c = ptc;
		dxfArc.r = r;
		dxfArc.sq = DEG(sq);	// ARC�o�^��DEG
		dxfArc.eq = DEG(eq);
		pData = new CDXFarc(&dxfArc);
	}
	else {
		const CDXFellipse* pEllipse = static_cast<const CDXFellipse *>(pDataSrc);
		DXFEARGV	dxfEllipse;
		dxfEllipse.pLayer = pEllipse->GetParentLayer();
		dxfEllipse.c = ptc;
		dxfEllipse.l = pEllipse->GetLongPoint();
		dxfEllipse.s = pEllipse->GetShortMagni();
		dxfEllipse.bRound = TRUE;
		// �ȉ~�p�Ɋp�x�̍Čv�Z(DXFshape.cpp CreateDxfOffsetObject() �Q��)
		pts.RoundPoint(-pEllipse->GetLean());
		pte.RoundPoint(-pEllipse->GetLean());
		float	l = pEllipse->GetLongLength();
		q = pts.x / l;
		if ( q < -1.0f || 1.0f < q )
			q = copysign(1.0f, q);	// -1.0 or 1.0
		dxfEllipse.sq = copysign(acos(q), pts.y);
		if ( dxfEllipse.sq < 0.0f )
			dxfEllipse.sq += PI2;
		q = pte.x / l;
		if ( q < -1.0f || 1.0f < q )
			q = copysign(1.0f, q);
		dxfEllipse.eq = copysign(acos(q), pte.y);
		if ( dxfEllipse.eq < 0.0f )
			dxfEllipse.eq += PI2;
		pData = new CDXFellipse(&dxfEllipse);
	}

	return pData;
}

void SetAllExcludeData(CDXFchain* pChain)
{
	PLIST_FOREACH(CDXFdata* pData, pChain)
		if ( pData )
			pData->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
	END_FOREACH
}

CDXFworkingOutline*	GetOutlineHierarchy(CDXFshape* pShape, INT_PTR n)
{
	// ���Ԗڂ̗֊s��޼ު�Ă��擾
	COutlineList*	pOutlineList = pShape->GetOutlineList();
	CDXFworkingOutline*	pOutline = NULL;
	INT_PTR		i;
	POSITION	pos;
	// 0����s��break�̉\�� -> PLIST_FOREACH �g���Ȃ�
	for ( i=0, pos=pOutlineList->GetHeadPosition(); pos && i<=n; i++ )
		pOutline = pOutlineList->GetNext(pos);

	return pOutline;
}

CDXFworkingOutline*	GetOutlineLastObj(CDXFshape* pShape, INT_PTR)
{
	return pShape->GetOutlineLastObj();
}

//////////////////////////////////////////////////////////////////////
//	�̾�ĵ�޼ު�Ă̕���ϰ�(NULL)�ŕ���

UINT SeparateOutline_Thread(LPVOID pVoid)
{
	LPSEPARATEOUTLINETHREADPARAM	pParam = reinterpret_cast<LPSEPARATEOUTLINETHREADPARAM>(pVoid);

	while ( TRUE ) {
		// ��ʂ̎��s���������܂ų���
		pParam->evStart.Lock();
		pParam->evStart.ResetEvent();
		// �p���׸�����
		if ( !pParam->bThread )
			break;
		// ���������J�n
		pParam->pOutline->SeparateModify();
		// �����I��
		pParam->evEnd.SetEvent();
	}

	return 0;
}
