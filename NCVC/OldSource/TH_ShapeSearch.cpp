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
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

static	CThreadDlg*	g_pParent;
#define	IsThread()	g_pParent->IsThreadContinue()

class	CHECKMAPTHREADPARAM {
public:
	CEvent		evStart,	// ٰ�ߊJ�n�����
				evEnd;		// �I���҂��m�F
	BOOL		bThread;	// �گ�ނ̌p���׸�
	CDXFmap*	pMap;		// �����Ώ�ϯ��
	// CHECKMAPTHREADPARAM::CEvent ���蓮����Ăɂ��邽�߂̺ݽ�׸�
	CHECKMAPTHREADPARAM() : evStart(FALSE, TRUE), evEnd(TRUE, TRUE),
		bThread(TRUE), pMap(NULL)
	{}
};
#define	LPCHECKMAPTHREADPARAM	CHECKMAPTHREADPARAM *
static	UINT	CheckMapWorking_Thread(LPVOID);

static	void	SetChainMap(CLayerData*, LPCHECKMAPTHREADPARAM);
static	void	SearchChainMap(const CDXFmap*, CPointD&, CDXFmap*, CDXFmap&);

//////////////////////////////////////////////////////////////////////
//	�A����޼ު�Ă̌����گ��
//////////////////////////////////////////////////////////////////////

UINT ShapeSearch_Thread(LPVOID pVoid)
{
	static	LPCTSTR	szMother = "���Wϯ�ߕ�̐���";

	LPNCVCTHREADPARAM	pParam = (LPNCVCTHREADPARAM)pVoid;
	CDXFDoc*	pDoc = (CDXFDoc *)(pParam->pDoc);
	g_pParent = pParam->pParent;
	
	int		i, nLoop = pDoc->GetDxfSize(), nResult = IDOK;
	POSITION	pos;
	CLayerMap*	pLayerMap = pDoc->GetLayerMap();
	CString		strMsg, strLayer;
	CLayerData*	pLayer;
	CDXFdata*	pData;
	HANDLE		hCheckMapThread;
	CHECKMAPTHREADPARAM	chkParam;

	VERIFY(strMsg.LoadString(IDS_SHAPESEARCH));
	g_pParent->SetFaseMessage(strMsg, szMother);

	// ���Wϯ�ߌ����گ�ދN��
	CWinThread*	pThread = AfxBeginThread(CheckMapWorking_Thread, &chkParam);
	hCheckMapThread = NC_DuplicateHandle(pThread->m_hThread);
	// SetPointMap() �O�̐ÓI�ϐ�������
	CDXFmap::ms_dTolerance = NCMIN;
	for ( pos=pLayerMap->GetStartPosition(); pos && IsThread(); ) {
		pLayerMap->GetNextAssoc(pos, strLayer, pLayer);
		pLayer->GetMasterMap()->InitHashTable(max(17, GetPrimeNumber(pLayer->GetCount()*2)));
	}
	
	try {
		// �S�ް��Ώۂ�ڲԂ��Ƃ̍��Wϯ�ߕ�̂��쐬
		g_pParent->m_ctReadProgress.SetRange32(0, nLoop);
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pData = pDoc->GetDxfData(i);
			if ( pData->GetType() != DXFPOINTDATA ) {
				pData->GetLayerData()->SetPointMasterMap(pData);
				pData->ClearMakeFlg();
				pData->ClearSearchFlg();
			}
			if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
				g_pParent->m_ctReadProgress.SetPos(i);		// ��۸�ڽ�ް
		}
		g_pParent->m_ctReadProgress.SetPos(nLoop);
		// ڲԂ��Ƃ̌ŗL���Wϯ�ߕ�̂���A����޼ު�Ă̌���
		for ( pos=pLayerMap->GetStartPosition(); pos && IsThread(); ) {
			pLayerMap->GetNextAssoc(pos, strLayer, pLayer);
			if ( pLayer->IsCutType() ) {
				g_pParent->SetFaseMessage(strMsg, strLayer);
				SetChainMap(pLayer, &chkParam);
				// ���Wϯ�ߕ�̂�����(�گ�ދN��)
				pLayer->RemoveMasterMap();
			}
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

	if ( nResult==IDCANCEL || !IsThread() ) {
		// �r���ŷ�ݾق��ꂽ����Wϯ�߂�ر
		CDXFmapArray*	pMapArray;
		for ( pos=pLayerMap->GetStartPosition(); pos; ) {
			pLayerMap->GetNextAssoc(pos, strLayer, pLayer);
			if ( pLayer->IsCutType() ) {
				// ���Wϯ�ߕ�̂�����
				pLayer->RemoveMasterMap();
				// �A���W�c������
				pMapArray = pLayer->GetChainMap();
				for ( i=0; i<pMapArray->GetSize(); i++ )
					delete	pMapArray->GetAt(i);
				pMapArray->RemoveAll();
			}
		}
		// �׸ނ�������
		for ( i=0; i<nLoop; i++ ) {
			pData = pDoc->GetDxfData(i);
			if ( pData->GetType() != DXFPOINTDATA ) {
				pData->ClearMakeFlg();
				pData->ClearSearchFlg();
			}
		}
	}

	g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

void SetChainMap(CLayerData* pLayer, LPCHECKMAPTHREADPARAM pParam)
{
	int			nCnt = 0, nShape = 0;
	CPointD		pt;
	const	CDXFmap*	pMasterMap = pLayer->GetMasterMap();
	CDXFmap		mapRegist;	// �A�����W�o�^�ς�ϯ��
	CDXFmap*	pMap;
	CDXFarray*	pDummy;
	CString		strShape;

	g_pParent->m_ctReadProgress.SetRange32(0, pMasterMap->GetCount());
	pLayer->GetChainMap()->SetSize(0, 1024);
	mapRegist.InitHashTable(max(17, GetPrimeNumber(pLayer->GetCount())));
	
	// ���Wϯ�ߕ�̂𼰹ݼ�قɱ������A���ő̂��쐬
	for ( POSITION pos=pMasterMap->GetStartPosition(); pos && IsThread(); nCnt++ ) {
		pMasterMap->GetNextAssoc(pos, pt, pDummy);
		if ( !mapRegist.Lookup(pt, pDummy) ) {
			strShape.Format("�`��%04d", ++nShape);
			pMap = new CDXFmap(0, strShape);
			pMap->InitHashTable(max(17, GetPrimeNumber(pLayer->GetCount())));
			// �A�����W�̌���
			SearchChainMap(pMasterMap, pt, pMap, mapRegist);	// �ċA�Ăяo��
#ifdef _DEBUG
			int		n = 0;
			BOOL	b = FALSE;
			for ( POSITION pos2=pMap->GetStartPosition(); pos2; ) {
				pMap->GetNextAssoc(pos2, pt, pDummy);
				n += pDummy->GetSize();
				if ( pDummy->GetSize() != 2 )
					b = TRUE;
			}
			g_dbg.printf("%s TotalObject=%d%s", strShape, n, b ? "(Warning!)" : "");
#endif
			// �O��̌����گ�ޏI���҂�
			pParam->evEnd.Lock();
			pParam->evEnd.ResetEvent();
			// �����گ�ފJ�n
			pParam->pMap = pMap;
			pParam->evStart.SetEvent();
			// �A�����Wϯ�߂�o�^
			pLayer->GetChainMap()->Add(pMap);
		}
		if ( (nCnt & 0x003f) == 0 )
			g_pParent->m_ctReadProgress.SetPos(nCnt);
	}
#ifdef _DEBUG
	g_dbg.printf("m_obChainMap.GetSize()=%d", pLayer->GetChainMap()->GetSize());
#endif
}

void SearchChainMap
	(const CDXFmap* pMasterMap, CPointD& pt, CDXFmap* pMap, CDXFmap& mapRegist)
{
	int			i, j;
	CPointD		ptSrc;
	CDXFarray*	pArray;
	CDXFarray*	pDummy;
	CDXFdata*	pData;

	VERIFY( pMasterMap->Lookup(pt, pArray) );		// �K�����݂���

	// ���o�^�̍��W�ް�������
	if ( !mapRegist.Lookup(pt, pDummy) ) {
		pDummy = NULL;
		mapRegist.SetAt(pt, pDummy);
		for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pMap->SetPointMap(pData);
				pData->SetMakeFlg();
				// ��޼ު�Ă̒[�_������Ɍ���
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					ptSrc = pData->GetNativePoint(j);
					if ( ptSrc != HUGE_VAL && ptSrc != pt )
						SearchChainMap(pMasterMap, ptSrc, pMap, mapRegist);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
//	���Wϯ�߂̌����گ��
//		��_��[�_�������
//			DXFMAPFLG_CANNOTWORKING -> �֊s�C�߹�ĉ��H���ł��Ȃ�
//		�P�̍��W�ɂR�ȏ�̵�޼ު�Ă������
//			DXFMAPFLG_CANNOTAUTOWORKING -> �����������珜�O

UINT CheckMapWorking_Thread(LPVOID pVoid)
{
	LPCHECKMAPTHREADPARAM	pParam = (LPCHECKMAPTHREADPARAM)pVoid;
	int			i, j, nLoop;
	POSITION	pos;
	CPointD		pt, ptChk[4];
	CDXFarray*	pArray;
	CDXFarray	obArray;
	CDXFdata*	pData;

	obArray.SetSize(0, 1024);

	while ( IsThread() ) {
		// ��ʂ̎��s���������܂ų���
		pParam->evStart.Lock();
		pParam->evStart.ResetEvent();
		// �p���׸�����
		if ( !pParam->bThread )
			break;
		// �����J�n
		for ( pos=pParam->pMap->GetStartPosition(); pos && IsThread(); ) {
			pParam->pMap->GetNextAssoc(pos, pt, pArray);
			// �[�_��P�̍��W�ɂR�ȏ�̵�޼ު�Ă��Ȃ���
			if ( pArray->GetSize() != 2 )
				pParam->pMap->SetMapFlag(DXFMAPFLG_CANNOTAUTOWORKING);
			// ��_�����̂��߂ɵ�޼ު�Ă��߰
			for ( i=0; i<pArray->GetSize(); i++ ) {
				pData = pArray->GetAt(i);
				if ( !pData->IsSearchFlg() ) {
					obArray.Add(pData);
					pData->SetSearchFlg();
				}
			}
		}
		// ��_����
		nLoop = obArray.GetSize();
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pData = obArray[i];
			for ( j=i+1; j<nLoop && IsThread(); j++ ) {
				if ( pData->GetIntersectionPoint(obArray[j], ptChk) > 0 ) {
					pParam->pMap->SetMapFlag(DXFMAPFLG_CANNOTWORKING);
					break;
				}
			}
		}
		obArray.RemoveAll();
		// �����I��
		pParam->evEnd.SetEvent();
	}

	pParam->evEnd.SetEvent();
	return 0;
}
