// TH_AutoWorkingSet.cpp
// �������H�w��
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"	// DXFView.h
#include "DXFdata.h"
#include "DXFshape.h"
#include "DXFDoc.h"
#include "DXFView.h"	// DXFTREETYPE
#include "Layer.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;
#define	IsThread()	pParent->IsThreadContinue()

typedef	BOOL	(*PFNAUTOPROC)(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoOutlineProc(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoPocketProc(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoAllInside(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoAllOutside(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoRecalcWorking(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	void	CreateAutoWorking(CThreadDlg*, CLayerData*, BOOL);

//////////////////////////////////////////////////////////////////////
//	�������H�w���گ��
//////////////////////////////////////////////////////////////////////

UINT AutoWorkingSet_Thread(LPVOID pVoid)
{
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CThreadDlg*	pParent = pParam->pParent;
	CDXFDoc*	pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	int			nType = (int)(pParam->wParam);	// ��������
	DXFTREETYPE*	vSelect = reinterpret_cast<DXFTREETYPE*>(pParam->lParam);	// �Čv�Z���̌`��W��
	ENAUTOWORKINGTYPE enType;
	BOOL		bPocket;
	PFNAUTOPROC	pfnAutoProc;

	if ( nType >= 100 ) {	// �߹�Ďw��
		enType = (ENAUTOWORKINGTYPE)(nType - 100);
		bPocket = TRUE;
	}
	else {					// �֊s�܂��͍Čv�Z(DXFShapeView)����
		enType = (ENAUTOWORKINGTYPE)nType;
		bPocket = FALSE;
	}

	switch ( enType ) {
	case AUTOOUTLINE:
		pfnAutoProc = &AutoOutlineProc;
		break;
	case AUTOPOCKET:
		pfnAutoProc = &AutoPocketProc;
		break;
	case AUTOALLINSIDE:
		pfnAutoProc = &AutoAllInside;
		break;
	case AUTOALLOUTSIDE:
		pfnAutoProc = &AutoAllOutside;
		break;
	case AUTORECALCWORKING:
		pfnAutoProc = &AutoRecalcWorking;
		break;
	default:
		pParent->PostMessage(WM_USERFINISH, IDCANCEL);
		return 0;
	}

	int		i, nLoop = pDoc->GetLayerCnt(), nResult = IDOK;
	CString		strMsg;
	CLayerData*	pLayer;

	VERIFY(strMsg.LoadString(IDS_AUTOWORKING));

	// ڲԂ��Ƃɍ��Wϯ�߂�����
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pLayer = pDoc->GetLayerData(i);
		if ( !pLayer->IsCutType() || 
				(vSelect && vSelect->which()==DXFTREETYPE_LAYER && pLayer!=get<CLayerData*>(*vSelect)) )
			continue;
		pParent->SetFaseMessage(strMsg, pLayer->GetStrLayer());
		// ���ߕʂ̎�������
		if ( (*pfnAutoProc)(pParent, pLayer, vSelect) )
			CreateAutoWorking(pParent, pLayer, bPocket);
		if ( vSelect && vSelect->which()==DXFTREETYPE_LAYER )
			break;	// ڲԎw�肠��Έȍ~�K�v�Ȃ�
	}

	pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
//	�����֊s���H�w��
//		���̵�޼ު�ĂɊ܂܂��ŏ�������޼ު�Ăɓ����̉��H�w���C
//		����ȊO�͊O���̉��H�w��
BOOL AutoOutlineProc(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE*)
{
	int		i, j, nLoop = pLayer->GetShapeSize();
	BOOL	bResult = FALSE;
	CDXFshape*	pShape;
	CDXFshape*	pShapeTmp;
	CRectD		rcBase;

	pParent->m_ctReadProgress.SetRange32(0, nLoop);
	pLayer->AscendingShapeSort();	// �ʐςŏ������בւ�

	// ��L��`�̓��O����
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ��۸�ڽ�ް
		pParent->m_ctReadProgress.SetPos(i);	// �������Ȃ��̂łP�����X�V
		pShape->ClearSideFlg();
		// ���������Ώۂ��ۂ�(CDXFchain* ������ΏۂƂ���)
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		// rcBase�����傫�Ȗʐς����µ�޼ު��(i+1)��rcBase���܂ނ��ǂ����̔���
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType() != 0 )
				continue;
			if ( pShapeTmp->GetMaxRect().PtInRect(rcBase) )
				break;	// �Ȍ�̌����͕K�v�Ȃ�
		}
		pShape->SetShapeFlag(j<nLoop ? DXFMAPFLG_INSIDE : DXFMAPFLG_OUTSIDE);
		bResult = TRUE;		// �����ς�
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	�����߹�ĉ��H�w��
//		�ő�O���ƍŏ������ɓ����̉��H�w��
BOOL AutoPocketProc(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE*)
{
	int		i, j, nLoop = pLayer->GetShapeSize();
	BOOL	bResult = FALSE;
	CDXFshape*	pShape;
	CDXFshape*	pShapeTmp;
	CRectD		rcBase;

	pParent->m_ctReadProgress.SetRange32(0, nLoop);
	pLayer->DescendingShapeSort();	// �ʐςō~�����בւ�

	// �ő�O������
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ��۸�ڽ�ް
		pParent->m_ctReadProgress.SetPos(i);
		pShape->ClearSideFlg();
		// CDXFchain* ������ΏۂƂ���
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		// rcBase���S�Ă̵�޼ު�Ă��܂�ł��邩�ǂ����̔���
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType() != 0 )
				continue;
			if ( !rcBase.PtInRect(pShapeTmp->GetMaxRect()) )
				break;	// �܂܂Ȃ���ΈȌ�̌����͕K�v�Ȃ�
		}
		if ( j >= nLoop ) {	// �܂�ł����
			pShape->SetShapeFlag(DXFMAPFLG_INSIDE);
			bResult = TRUE;
		}
		break;	// �ő��`�̂�
	}

	// �ŏ���������
	for ( ; i<nLoop && IsThread(); i++ ) {	// [0]�`�͔���ς�
		pShape = pLayer->GetShapeData(i);
		pParent->m_ctReadProgress.SetPos(i);
		pShape->ClearSideFlg();
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		// rcBase�����̵�޼ު��(i+1)���܂ނ��ǂ����̔���
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType() != 0 )
				continue;
			if ( rcBase.PtInRect(pShapeTmp->GetMaxRect()) )
				break;	// �܂�ł���ΈȌ�̌����͕K�v�Ȃ�
		}
		if ( j >= nLoop ) {	// �܂܂Ȃ����
			pShape->SetShapeFlag(DXFMAPFLG_INSIDE);
			bResult = TRUE;
		}
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	�S�ē����w��
BOOL AutoAllInside(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE*)
{
	int		i, nLoop = pLayer->GetShapeSize();
	BOOL	bResult = FALSE;
	CDXFshape*	pShape;

	pParent->m_ctReadProgress.SetRange32(0, nLoop);

	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ��۸�ڽ�ް
		pParent->m_ctReadProgress.SetPos(i);
		pShape->ClearSideFlg();
		// ���������Ώۂ��ۂ�(CDXFchain* ������ΏۂƂ���)
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		pShape->SetShapeFlag(DXFMAPFLG_INSIDE);		// �S�ē���
		bResult = TRUE;		// �����ς�
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	�S�ĊO���w��
BOOL AutoAllOutside(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE*)
{
	int		i, nLoop = pLayer->GetShapeSize();
	BOOL	bResult = FALSE;
	CDXFshape*	pShape;

	pParent->m_ctReadProgress.SetRange32(0, nLoop);

	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ��۸�ڽ�ް
		pParent->m_ctReadProgress.SetPos(i);
		pShape->ClearSideFlg();
		// ���������Ώۂ��ۂ�(CDXFchain* ������ΏۂƂ���)
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		pShape->SetShapeFlag(DXFMAPFLG_OUTSIDE);	// �S�ĊO��
		bResult = TRUE;		// �����ς�
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	�̾�Ēl�ύX�ɂ��Čv�Z
BOOL AutoRecalcWorking(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE* vSelect)
{
	int		i, nInOut, nLoop = pLayer->GetShapeSize();
	DWORD	dwFlags;
	CDXFdata*	pData;
	CDXFshape*	pShape;
	CDXFshape*	pShapeSrc = NULL;
	CDXFchain	ltOutline;
	CDXFworking*	pWork;

	// �ΏۏW��
	if ( vSelect && vSelect->which()==DXFTREETYPE_SHAPE )
		pShapeSrc = get<CDXFshape*>(*vSelect);

	pParent->m_ctReadProgress.SetRange32(0, nLoop);

	try {
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			// ��۸�ڽ�ް
			pParent->m_ctReadProgress.SetPos(i);
			// �`��W�����w�肳��Ă���΁A�����ϯ�������̂���
			if ( pShapeSrc && pShapeSrc != pShape )
				continue;
			// ���ɂ�����H�w���𒊏o
			dwFlags = pShape->GetShapeFlag();
			if ( !pShape->GetOutlineObject() )
				continue;
			// ���ݓo�^����Ă�������ŗ֊s��޼ު�Đ���
			nInOut = pShape->GetInOutFlag();
			if ( !pShape->CreateOutlineTempObject(nInOut, &ltOutline) ) {
				// �ꎞ��޼ު�đS�폜
				for ( POSITION pos=ltOutline.GetHeadPosition(); pos; ) {
					pData = ltOutline.GetNext(pos);
					if ( pData )
						delete	pData;
				}
				continue;
			}
			// ���H�w���o�^
			pWork = new CDXFworkingOutline(pShape, &ltOutline, DXFWORKFLG_AUTO);
			pShape->AddWorkingData(pWork, nInOut);
			pWork = NULL;
			// ����ٰ�߂ɔ�����
			ltOutline.RemoveAll();
			if ( pShapeSrc )
				break;	// �`��w�肠��Έȍ~�K�v�Ȃ�
		}
	}
	catch ( CMemoryException* e ) {
		if ( pWork )
			delete	pWork;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return FALSE;	// �S�Ă��̊֐��ŏ�������̂� CreateAutoWorking() �����s���Ȃ�
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//	���O���肩����H�w���̐���

void CreateAutoWorking(CThreadDlg* pParent, CLayerData* pLayer, BOOL)
{
	int		i, j, n, nLoop = pLayer->GetShapeSize();
	double		dArea[2];
	CRect3D		rcMax;
	CDXFchain	ltOutline[2];
	CDXFdata*		pData;
	CDXFshape*		pShape;
	CDXFworking*	pWork;
	POSITION		pos;
#ifdef _DEBUG
	CRect	rcDbg;
#endif

	pParent->m_ctReadProgress.SetRange32(0, nLoop);

	try {
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pWork = NULL;
			pShape = pLayer->GetShapeData(i);
			// ��۸�ڽ�ް
			pParent->m_ctReadProgress.SetPos(i);
			// �����Ώ�����
			if ( !(pShape->GetShapeFlag() & (DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) )
				continue;
#ifdef _DEBUG
			g_dbg.printf("ShapeName=%s", pShape->GetShapeName());
			rcDbg = pShape->GetMaxRect();
			g_dbg.printStruct(&rcDbg, "Orig");
#endif
			// �֊s�ꎞ��޼ު�Ă̐���
			for ( j=0; j<SIZEOF(ltOutline) && IsThread(); j++ ) {
				if ( !pShape->CreateOutlineTempObject(j, &ltOutline[j]) ) {
					// �ꎞ��޼ު�đS�폜
					for ( n=0; n<SIZEOF(ltOutline); n++ ) {
						for ( pos=ltOutline[n].GetHeadPosition(); pos; ) {
							pData = ltOutline[n].GetNext(pos);
							if ( pData )
								delete	pData;
						}
						ltOutline[n].RemoveAll();
					}
					break;
				}
				if ( ltOutline[j].IsEmpty() )
					rcMax = pShape->GetMaxRect();	// ���W���̋�`�̈���
				else
					rcMax = ltOutline[j].GetMaxRect();
				// ��`�̈�̑傫���v�Z
				dArea[j] = rcMax.Width() * rcMax.Height();
#ifdef _DEBUG
				rcDbg = ltOutline[j].GetMaxRect();
				g_dbg.printStruct(&rcDbg, "OutLine");
#endif
			}
			if ( j < SIZEOF(ltOutline) )	// CreateOutlineTempObject() �ł̴װ
				continue;
			// ���O����`�̑傫���Ō���
			if ( pShape->GetShapeFlag() & DXFMAPFLG_INSIDE )
				j = dArea[0] > dArea[1] ? 1 : 0;	// �����������̗p
			else
				j = dArea[0] > dArea[1] ? 0 : 1;	// �傫�������̗p
			// ���H�w���o�^
			if ( !ltOutline[j].IsEmpty() ) {
				pWork = new CDXFworkingOutline(pShape, &ltOutline[j], DXFWORKFLG_AUTO);
				pShape->AddWorkingData(pWork, j);
#ifdef _DEBUG
				g_dbg.printf("Select OutLine = %d", j);
#endif
			}
			// Select����CDXFworkingOutline���޽�׸��ɂ�delete
			n = 1 - j;	// 1->0, 0->1
			for ( pos=ltOutline[n].GetHeadPosition(); pos; ) {
				pData = ltOutline[n].GetNext(pos);
				if ( pData )
					delete	pData;
			}
			// ����ٰ�߂ɔ����A��`�̏�����
			for ( j=0; j<SIZEOF(ltOutline); j++ )
				ltOutline[j].RemoveAll();
		}
	}
	catch ( CMemoryException* e ) {
		if ( pWork )
			delete	pWork;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return;
}
