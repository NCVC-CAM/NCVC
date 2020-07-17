// TH_ShapeSearch.cpp
// �A����޼ު�Ă�����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

static	CThreadDlg*	g_pParent;
#define	IsThread()	g_pParent->IsThreadContinue()

struct	CHECKMAPTHREADPARAM
{
	CEvent		evStart,	// ٰ�ߊJ�n�����
				evEnd;		// �I���҂��m�F
	BOOL		bThread;	// �گ�ނ̌p���׸�
	CLayerData*	pLayer;		// �Ώ�ڲ�
	CDXFmap*	pMap;		// �����Ώ�ϯ��
	// CHECKMAPTHREADPARAM::CEvent ���蓮����Ăɂ��邽�߂̺ݽ�׸�
	CHECKMAPTHREADPARAM() : evStart(FALSE, TRUE), evEnd(TRUE, TRUE),
		bThread(TRUE), pMap(NULL)
	{}
};
#define	LPCHECKMAPTHREADPARAM	CHECKMAPTHREADPARAM *
static	UINT	CheckMapWorking_Thread(LPVOID);

static	void	SetChainMap(const CDXFmap*, LPCHECKMAPTHREADPARAM);
static	void	SearchChainMap(const CDXFmap*, const CPointD&, CDXFmap*, CDXFmap&);

//////////////////////////////////////////////////////////////////////
//	�A����޼ު�Ă̌����گ��
//////////////////////////////////////////////////////////////////////

UINT ShapeSearch_Thread(LPVOID pVoid)
{
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CDXFDoc*	pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	g_pParent = pParam->pParent;
	
	int		i, j, nResult = IDOK;
	INT_PTR	nLayerCnt = pDoc->GetLayerCnt(), nDataCnt;
	CString		strMsg;
	CLayerData*	pLayer;
	CDXFdata*	pData;
	CDXFmap		mpDXFdata;	// ڲԂ��Ƃ̍��Wϯ�ߕ��
	HANDLE		hCheckMapThread;
	CHECKMAPTHREADPARAM	chkParam;

	VERIFY(strMsg.LoadString(IDS_SHAPESEARCH));

	// ���Wϯ�ߌ����گ�ދN��
	CWinThread*	pThread = AfxBeginThread(CheckMapWorking_Thread, &chkParam);
	hCheckMapThread = ::NCVC_DuplicateHandle(pThread->m_hThread);
	if ( !hCheckMapThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
	// SetPointMap() �O�̐ÓI�ϐ�������
	CDXFmap::ms_dTolerance = NCMIN;

	try {
		// �S�ް��Ώۂ�ڲԂ��Ƃ̍��Wϯ�ߕ�̂��쐬
		for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
			pLayer = pDoc->GetLayerData(i);
			if ( !pLayer->IsCutType() )
				continue;
			nDataCnt = pLayer->GetDxfSize();
			g_pParent->SetFaseMessage(strMsg, pLayer->GetStrLayer());
			g_pParent->m_ctReadProgress.SetRange32(0, nDataCnt);
			j = GetPrimeNumber(nDataCnt*2);
			mpDXFdata.InitHashTable(max(17, j));
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfData(j);
				if ( pData->GetType() != DXFPOINTDATA ) {
					mpDXFdata.SetPointMap(pData);
					pData->ClearMakeFlg();
					pData->ClearSearchFlg();
				}
				if ( (j & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
					g_pParent->m_ctReadProgress.SetPos(j);		// ��۸�ڽ�ް
			}
			g_pParent->m_ctReadProgress.SetPos(nDataCnt);
			// ڲԂ��Ƃ̌ŗL���Wϯ�ߕ�̂���A����޼ު�Ă̌���
			chkParam.pLayer = pLayer;
			SetChainMap(&mpDXFdata, &chkParam);
			// ���Wϯ�ߕ�̂�����
			mpDXFdata.RemoveAll();
		}
		// �ŏI�����گ�ޏI���҂�
		chkParam.evEnd.Lock();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	// �����گ�ޏI���w��
	chkParam.bThread = FALSE;
	chkParam.evStart.SetEvent();
	WaitForSingleObject(hCheckMapThread, INFINITE);
	CloseHandle(hCheckMapThread);

	// �r���ŷ�ݾق��ꂽ����Wϯ�߂�ر
	if ( nResult==IDCANCEL || !IsThread() ) {
		for ( i=0; i<nLayerCnt; i++ ) {
			pLayer = pDoc->GetLayerData(i);
			if ( !pLayer->IsCutType() )
				continue;
			// �A���W�c������
			pLayer->RemoveAllShape();
			// �׸ނ�������
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt; j++ ) {
				pData = pLayer->GetDxfData(j);
				if ( pData->GetType() != DXFPOINTDATA ) {
					pData->ClearMakeFlg();
					pData->ClearSearchFlg();
				}
			}
		}
	}

	g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

void SetChainMap(const CDXFmap* pMasterMap, LPCHECKMAPTHREADPARAM pParam)
{
	int			nCnt = 0,
				nPrime = GetPrimeNumber(pParam->pLayer->GetDxfSize());
	CPointD		pt;
	CDXFmap		mapRegist;	// �A�����W�o�^�ς�ϯ��
	CDXFmap*	pMap;
	CDXFarray*	pDummy;

	g_pParent->m_ctReadProgress.SetRange32(0, pMasterMap->GetCount());
	mapRegist.InitHashTable(max(17, nPrime));
	
	// ���Wϯ�ߕ�̂𼰹ݼ�قɱ������A���ő̂��쐬
	for ( POSITION pos=pMasterMap->GetStartPosition(); pos && IsThread(); nCnt++ ) {
		pMasterMap->GetNextAssoc(pos, pt, pDummy);
		if ( !mapRegist.PLookup(pt) ) {
			pMap = new CDXFmap;
			pMap->InitHashTable(max(17, nPrime));
			// �A�����W�̌���
			SearchChainMap(pMasterMap, pt, pMap, mapRegist);	// �ċA�Ăяo��
			// �O��̌����گ�ޏI���҂�
			pParam->evEnd.Lock();
			pParam->evEnd.ResetEvent();
			// �����گ�ފJ�n
			pParam->pMap = pMap;
			pParam->evStart.SetEvent();
		}
		if ( (nCnt & 0x003f) == 0 )
			g_pParent->m_ctReadProgress.SetPos(nCnt);
	}
#ifdef _DEBUG
	g_dbg.printf("m_obShapeArray.GetSize()=%d", pParam->pLayer->GetShapeSize());
#endif
}

void SearchChainMap
	(const CDXFmap* pMasterMap, const CPointD& pt, CDXFmap* pMap, CDXFmap& mapRegist)
{
	int			i, j;
	CPointD		ptSrc;
	CDXFarray*	pArray;
	CDXFarray*	pDummy = NULL;
	CDXFdata*	pData;

	if ( !pMasterMap->Lookup(const_cast<CPointD&>(pt), pArray) )	// �K�����݂���
		NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	// ���o�^�̍��W�ް�������
	if ( !mapRegist.PLookup(const_cast<CPointD&>(pt)) ) {
		mapRegist.SetAt(const_cast<CPointD&>(pt), pDummy);
		for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pMap->SetPointMap(pData);
				pData->SetMakeFlg();
				// ��޼ު�Ă̒[�_������Ɍ���
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					ptSrc = pData->GetNativePoint(j);
					if ( ptSrc != pt )
						SearchChainMap(pMasterMap, ptSrc, pMap, mapRegist);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
//	�e�W�c(���Wϯ��)���Ƃ̌����گ��

UINT CheckMapWorking_Thread(LPVOID pVoid)
{
	LPCHECKMAPTHREADPARAM	pParam = reinterpret_cast<LPCHECKMAPTHREADPARAM>(pVoid);
	int			nChain = 0, nMap = 0;
	CString		strShape;
	DWORD		dwFlags;
	CDXFchain*	pChain;
	CDXFshape*	pShape;

	while ( IsThread() ) {
		// ��ʂ̎��s���������܂ų���
		pParam->evStart.Lock();
		pParam->evStart.ResetEvent();
		// �p���׸�����
		if ( !pParam->bThread )
			break;
		// �����J�n
		dwFlags = pParam->pMap->GetMapTypeFlag();
		// �`���񐶐�
		if ( dwFlags == 0 ) {
			// CDXFmap����CDXFchain�ɏ��i
			pChain = new CDXFchain;
			pParam->pMap->CopyToChain(pChain);
			delete	pParam->pMap;	// ����CDXFmap�͏���
			strShape.Format("�֊s%04d", ++nChain);
			pShape = new CDXFshape(DXFSHAPE_OUTLINE, strShape, 0, pChain);
		}
		else {
			strShape.Format("�O��%04d", ++nMap);
			pShape = new CDXFshape(DXFSHAPE_LOCUS, strShape, dwFlags, pParam->pMap);
		}
		// �`����o�^
		pParam->pLayer->AddShape(pShape);
		// �����I��
		pParam->evEnd.SetEvent();
	}

	pParam->evEnd.SetEvent();
	return 0;
}
