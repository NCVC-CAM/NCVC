// TH_ShapeSearch.cpp
// �A����޼ު�Ă�����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static	CThreadDlg*	g_pParent;
static	BOOL	IsThread_Dlg(void)
{
	return g_pParent->IsThreadContinue();
}
static	BOOL	IsThread_NoChk(void)
{
	return TRUE;
}
static	boost::function<BOOL ()>	IsThread;
static	inline	void	_SetProgressPos(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetPos((int)n);
}

struct	CHECKMAPTHREADPARAM
{
	CEvent		evStart;	// ٰ�ߊJ�n�����
	CEvent		evEnd;		// �I���҂��m�F
	BOOL		bThread;	// �گ�ނ̌p���׸�
	CLayerData*	pLayer;		// �Ώ�ڲ�
	CDXFmap*	pMap;		// �����Ώ�ϯ��
	// CEvent ���蓮����Ăɂ��邽�߂̺ݽ�׸�
	CHECKMAPTHREADPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE), bThread(TRUE), pMap(NULL) {}
};
typedef	CHECKMAPTHREADPARAM*	LPCHECKMAPTHREADPARAM;
static	UINT	CheckMapWorking_Thread(LPVOID);

static	void	SetChainMap(const CDXFmap*, CLayerData*, LPCHECKMAPTHREADPARAM);
static	void	SearchChainMap(const CDXFmap*, const CPointF&, CDXFmap*, CDXFmap&);

//////////////////////////////////////////////////////////////////////
//	�A����޼ު�Ă̌����گ��
//////////////////////////////////////////////////////////////////////

UINT ShapeSearch_Thread(LPVOID pVoid)
{
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CDXFDoc*	pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	g_pParent = pParam->pParent;
	IsThread = g_pParent ? &IsThread_Dlg : &IsThread_NoChk;

	int			nResult = IDOK;
	INT_PTR		i, j, nLayerCnt = pDoc->GetLayerCnt(), nDataCnt;
	CString		strMsg;
	CLayerData*	pLayer;
	CDXFdata*	pData;
	CDXFmap		mpDXFdata;	// ڲԂ��Ƃ̍��Wϯ�ߕ��
	CHECKMAPTHREADPARAM	chkParam;

	VERIFY(strMsg.LoadString(IDS_SHAPESEARCH));

	// ����ď����ݒ�
	chkParam.evStart.ResetEvent();
	// ���Wϯ�ߌ����گ�ދN��
	CWinThread*	pThread = AfxBeginThread(CheckMapWorking_Thread, &chkParam);
	if ( !pThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	// SetPointMap() �O�̐ÓI�ϐ�������
	CDXFmap::ms_dTolerance = NCMIN;

	try {
		// �S�ް��Ώۂ�ڲԂ��Ƃ̍��Wϯ�ߕ�̂��쐬
		for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
			pLayer = pDoc->GetLayerData(i);
			if ( !pLayer->IsCutType() )
				continue;
			pLayer->RemoveAllShape();
			nDataCnt = pLayer->GetDxfSize();
			if ( g_pParent ) {
				g_pParent->SetFaseMessage(strMsg, pLayer->GetLayerName());
				g_pParent->m_ctReadProgress.SetRange32(0, (int)nDataCnt);
			}
			j = GetPrimeNumber((UINT)nDataCnt*2);
			mpDXFdata.InitHashTable((UINT)max(17, j));
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfData(j);
				if ( pData->GetType() != DXFPOINTDATA ) {
					mpDXFdata.SetPointMap(pData);
					pData->ClearMakeFlg();
					pData->ClearSearchFlg();
					// ���ײݎ��g�̌�_����
					if ( pData->GetType() == DXFPOLYDATA )
						static_cast<CDXFpolyline*>(pData)->CheckPolylineIntersection();
				}
				if ( g_pParent && (j & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
					_SetProgressPos(j);		// ��۸�ڽ�ް
			}
			if ( g_pParent )
				_SetProgressPos(nDataCnt);
			// ڲԂ��Ƃ̌ŗL���Wϯ�ߕ�̂���A����޼ު�Ă̌���
			SetChainMap(&mpDXFdata, pLayer, &chkParam);
			// ���Wϯ�ߕ�̂�����
			mpDXFdata.RemoveAll();
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	// �����گ�ޏI���w��
	chkParam.bThread = FALSE;
	chkParam.evStart.SetEvent();
	WaitForSingleObject(pThread->m_hThread, INFINITE);

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

	if ( g_pParent )
		g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

void SetChainMap(const CDXFmap* pMasterMap, CLayerData* pLayer, LPCHECKMAPTHREADPARAM pParam)
{
	int			nCnt = 0,
				nPrime = GetPrimeNumber((UINT)pLayer->GetDxfSize());
	CPointF		pt;
	CDXFmap		mapRegist;	// �A�����W�o�^�ς�ϯ��
	CDXFmap*	pMap;
	CDXFarray*	pDummy;

	if ( g_pParent )
		g_pParent->m_ctReadProgress.SetRange32(0, (int)pMasterMap->GetCount());
	mapRegist.InitHashTable(max(17, nPrime));
	pParam->evEnd.SetEvent();

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
			pParam->pMap	= pMap;
			pParam->pLayer	= pLayer;
			pParam->evStart.SetEvent();
		}
		if ( g_pParent && (nCnt & 0x003f) == 0 )
			_SetProgressPos(nCnt);
	}
	pParam->evEnd.Lock();
	pParam->evEnd.ResetEvent();

#ifdef _DEBUG
	printf("m_obShapeArray.GetSize()=%Id\n", pLayer->GetShapeSize());
#endif
}

void SearchChainMap
	(const CDXFmap* pMasterMap, const CPointF& pt, CDXFmap* pMap, CDXFmap& mapRegist)
{
	int			i, j;
	CPointF		ptSrc;
	CDXFarray*	pArray;
	CDXFarray*	pDummy = NULL;
	CDXFdata*	pData;

	if ( !pMasterMap->Lookup(const_cast<CPointF&>(pt), pArray) )	// �K�����݂���
		NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	// ���o�^�̍��W�ް�������
	if ( !mapRegist.PLookup(const_cast<CPointF&>(pt)) ) {
		mapRegist.SetAt(const_cast<CPointF&>(pt), pDummy);
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
			if ( pParam->pMap->CopyToChain(pChain) ) {
				delete	pParam->pMap;	// ����CDXFmap�͏���
				strShape.Format("�֊s%04d", ++nChain);
				pShape = new CDXFshape(DXFSHAPE_OUTLINE, strShape, 0, pParam->pLayer, pChain);
			}
			else {
				delete	pChain;			// ���i���s
				dwFlags = 1;			// �O�Տ�����
			}
		}
		if ( dwFlags != 0 ) {
			strShape.Format("�O��%04d", ++nMap);
			pShape = new CDXFshape(DXFSHAPE_LOCUS, strShape, dwFlags, pParam->pLayer, pParam->pMap);
		}
		// �`����o�^
		pParam->pLayer->AddShape(pShape);
#ifdef _DEBUG
		printf("Layer=%s %s Add ok\n", LPCTSTR(pParam->pLayer->GetLayerName()), LPCTSTR(strShape));
#endif
		// �����I��
		pParam->evEnd.SetEvent();
	}

	return 0;
}
