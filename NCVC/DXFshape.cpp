// DXFshape.cpp: CDXFmap �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
//#define	_DEBUGDRAW_DXF
#endif

using namespace boost;

IMPLEMENT_DYNAMIC(CDXFworking, CObject)
IMPLEMENT_SERIAL(CDXFworkingDirection, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingOutline, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingPocket, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFmap, CMapPointToDXFarray, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFchain, CDXFlist, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFshape, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

/////////////////////////////////////////////////////////////////////////////
// �ÓI�ϐ��̏�����
double		CDXFmap::ms_dTolerance = NCMIN;

static	DWORD	g_dwMapFlag[] = {
	DXFMAPFLG_DIRECTION, DXFMAPFLG_START,
	DXFMAPFLG_OUTLINE, DXFMAPFLG_POCKET
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^���H�w���̃x�[�X�N���X
/////////////////////////////////////////////////////////////////////////////
CDXFworking::CDXFworking
	(ENWORKINGTYPE enType, CDXFshape* pShape, CDXFdata* pData, DWORD dwFlags)
{
	static	CString	strAuto("����");
	static	TCHAR*	szWork[] = {"����", "�O��"};

	m_enType = enType;
	m_pShape = pShape;
	m_pData = pData;
	m_dwFlags = dwFlags;

	// ��������
	if ( m_dwFlags & DXFWORKFLG_AUTO ) {
		if ( !m_pShape )
			return;
		if ( m_pShape->GetShapeFlag() & DXFMAPFLG_INSIDE )
			m_strWorking = strAuto + szWork[0];
		else if ( m_pShape->GetShapeFlag() & DXFMAPFLG_OUTSIDE )
			m_strWorking = strAuto + szWork[1];
	}
	// ��̫�ĉ��H�w����
	CString	strWork;
	VERIFY(strWork.LoadString(enType+ID_EDIT_SHAPE_VEC));
	int	nStart = strWork.Find('\n') + 1,				// '\n' �̎�����
		nCount = strWork.Find('(', nStart) - nStart;	// '('  �܂�
	m_strWorking += strWork.Mid(nStart, nCount);
}

void CDXFworking::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {
		ar << (m_dwFlags & ~DXFWORKFLG_SELECT) << m_strWorking;	// �I����ԏ���
		if ( m_pData ) {
			ar << (BYTE)1;
			ar << m_pData->GetSerializeSeq();
		}
		else
			ar << (BYTE)0;
	}
	else {
		BYTE	bExist;
		DWORD	nIndex;
		CLayerData*	pLayer = (CLayerData *)(ar.m_pDocument);
		m_pShape = pLayer->GetActiveShape();
		ar >> m_dwFlags >> m_strWorking;
		ar >> bExist;
		if ( bExist ) {
			ar >> nIndex;
			m_pData = pLayer->GetDxfData(nIndex);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�����v���H�w���N���X
//////////////////////////////////////////////////////////////////////
CDXFworkingDirection::CDXFworkingDirection
	(CDXFshape* pShape, CDXFdata* pData, CPointD pts, CPointD pte[]) :
		CDXFworking(WORK_DIRECTION, pShape, pData, 0)
{
	m_ptStart = pts;
	for ( int i=0; i<SIZEOF(m_ptArraw); i++ )
		m_ptArraw[i] = pte[i];
}

void CDXFworkingDirection::DrawTuning(double f)
{
	m_ptDraw[1] = m_ptArraw[1] * f;
	m_ptDraw[0] = m_ptArraw[0] + m_ptDraw[1];
	m_ptDraw[2] = m_ptArraw[2] + m_ptDraw[1];
}

void CDXFworkingDirection::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFworkingDirection::Draw()", DBG_RED);
	dbg.printf("pt.x=%d pt.y=%d", m_ptDraw[1].x, m_ptDraw[1].y);
#endif
	pDC->Polyline(m_ptDraw, SIZEOF(m_ptDraw));
}

void CDXFworkingDirection::Serialize(CArchive& ar)
{
	int		i;
	CDXFworking::Serialize(ar);
	if ( ar.IsStoring() ) {
		ar << m_ptStart.x << m_ptStart.y;
		for ( i=0; i<SIZEOF(m_ptArraw); i++ )
			ar << m_ptArraw[i].x << m_ptArraw[i].y;
	}
	else {
		ar >> m_ptStart.x >> m_ptStart.y;
		for ( i=0; i<SIZEOF(m_ptArraw); i++ )
			ar >> m_ptArraw[i].x >> m_ptArraw[i].y;
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�֊s�v���H�w���N���X
//////////////////////////////////////////////////////////////////////
CDXFworkingOutline::CDXFworkingOutline(CDXFshape* pShape, const CDXFchain* pOutline, DWORD dwFlags) :
	CDXFworking(WORK_OUTLINE, pShape, NULL, dwFlags)
{
	m_ltOutline.AddTail(const_cast<CDXFchain*>(pOutline));
}

CDXFworkingOutline::~CDXFworkingOutline()
{
	for ( POSITION pos=m_ltOutline.GetHeadPosition(); pos; )
		delete	m_ltOutline.GetNext(pos);
}

void CDXFworkingOutline::DrawTuning(double f)
{
	for ( POSITION pos=m_ltOutline.GetHeadPosition(); pos; )
		m_ltOutline.GetNext(pos)->DrawTuning(f);
}

void CDXFworkingOutline::Draw(CDC* pDC) const
{
	for ( POSITION pos=m_ltOutline.GetHeadPosition(); pos; )
		m_ltOutline.GetNext(pos)->Draw(pDC);
}

void CDXFworkingOutline::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);

	// �֊s��޼ު�Ă̼رײ��
	// m_ltOutline.Serialize(ar); �ł� CDXFchain ���ĂсC
	// ��޼ު�Ăւ� SeqNo. �ŏ��������D�䂦�Ɏ��ͺ��ިݸ�
	if ( ar.IsStoring() ) {
		ar << m_ltOutline.GetCount();
		for ( POSITION pos=m_ltOutline.GetHeadPosition(); pos; )
			ar << m_ltOutline.GetNext(pos);
	}
	else {
		INT_PTR		nLoop;
		CObject*	pNewData;	// CDXFdata*
		ar >> nLoop;
		while ( nLoop-- ) {
			ar >> pNewData;
			m_ltOutline.AddTail( (CDXFdata *)pNewData );
		}
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�|�P�b�g�v���H�w���N���X
//////////////////////////////////////////////////////////////////////
CDXFworkingPocket::CDXFworkingPocket(CDXFshape* pShape, DWORD dwFlags) :
	CDXFworking(WORK_POCKET, pShape, NULL, dwFlags)
{
	m_obPocket.SetSize(0, 1024);
}

CDXFworkingPocket::~CDXFworkingPocket()
{
	for ( int i=0; i<m_obPocket.GetSize(); i++ )
		delete	m_obPocket[i];
}

void CDXFworkingPocket::DrawTuning(double f)
{
	for ( int i=0; i<m_obPocket.GetSize(); i++ )
		m_obPocket[i]->DrawTuning(f);
}

void CDXFworkingPocket::Draw(CDC* pDC) const
{
	for ( int i=0; i<m_obPocket.GetSize(); i++ )
		m_obPocket[i]->Draw(pDC);
}

void CDXFworkingPocket::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);
	m_obPocket.Serialize(ar);
}

/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̍��W�}�b�v�N���X
//////////////////////////////////////////////////////////////////////
CDXFmap::CDXFmap() : CMapPointToDXFarray(1024)
{
}

CDXFmap::~CDXFmap()
{
	RemoveAll();
}

void CDXFmap::Serialize(CArchive& ar)
{
	int			i, nDataCnt;
	CPointD		pt;
	CDXFarray*	pArray;

	if ( ar.IsStoring() ) {
		ar << GetCount() << GetHashTableSize();
		for ( POSITION pos=GetStartPosition(); pos; ) {
			GetNextAssoc(pos, pt, pArray);
			nDataCnt = pArray->GetSize();
			ar << pt.x << pt.y << nDataCnt;
			for ( i=0; i<nDataCnt; i++ )
				ar << pArray->GetAt(i)->GetSerializeSeq();
		}
	}
	else {
		INT_PTR		nMapLoop;
		UINT		nHash;
		DWORD		nIndex;
		CDXFdata*	pData;
		CLayerData*	pLayer = (CLayerData *)(ar.m_pDocument);
		ar >> nMapLoop >> nHash;
		InitHashTable(nHash);
		while ( nMapLoop-- ) {
			ar >> pt.x >> pt.y >> nDataCnt;
			if ( nDataCnt > 0 ) {
				pArray = new CDXFarray;
				pArray->SetSize(0, nDataCnt);
				while ( nDataCnt-- ) {
					ar >> nIndex;
					pData = pLayer->GetDxfData(nIndex);
					pArray->Add(pData);
				}
				SetAt(pt, pArray);
			}
		}
	}
}

void CDXFmap::RemoveAll()
{
	CPointD	pt;
	CDXFarray*	pArray;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		if ( pArray )
			delete	pArray;
	}
	CMapPointToDXFarray::RemoveAll();
}

void CDXFmap::SetPointMap(CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointD	pt;
#ifdef _DEBUGOLD
	CDumpContext	dc;
	dc.SetDepth(1);
	Dump(dc);
#endif

	// �e��޼ު�Ē��_�̍��W�o�^
	for ( int i=0; i<pData->GetPointNumber(); i++ ) {
		pt = pData->GetNativePoint(i);	// �ŗL���W�l
		if ( Lookup(pt, pArray) ) {
			pArray->Add(pData);
		}
		else {
			pArray = new CDXFarray;
			ASSERT( pArray );
			pArray->SetSize(0, 16);
			pArray->Add(pData);
			SetAt(pt, pArray);
		}
	}
}

void CDXFmap::SetMakePointMap(CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointD	pt;

	// �e��޼ު�Ē��_�̍��W�o�^
	for ( int i=0; i<pData->GetPointNumber(); i++ ) {
		pt = pData->GetTunPoint(i);		// ���_�������W
		if ( Lookup(pt, pArray) )
			pArray->Add(pData);
		else {
			pArray = new CDXFarray;
			ASSERT( pArray );
			pArray->SetSize(0, 16);
			pArray->Add(pData);
			SetAt(pt, pArray);
		}
	}
}

tuple<BOOL, CDXFarray*, CPointD>
CDXFmap::IsEulerRequirement(const CPointD& ptKey) const
{
	int			i, nObCnt, nOddCnt = 0;
	BOOL		bEuler = FALSE;	// ��M�����v���𖞂����Ă��邩
	double		dGap, dGapMin = HUGE_VAL, dGapMin2 = HUGE_VAL;
	CPointD		pt, ptStart, ptStart2;
	CDXFdata*	pData;
	CDXFarray*	pArray;
	CDXFarray*	pStartArray = NULL;
	CDXFarray*	pStartArray2;

	// ��޼ު�ēo�^���̊��T�� + ����׸ނ̸ر
	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		// ���W���ɑ΂���o�^��޼ު�Đ�����̋ߐڵ�޼ު�Ă�����
		// (�~�ް��Ͷ��Ă��Ȃ����� obArray->GetSize() ���g���Ȃ�)
		for ( i=0, nObCnt=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			pData->ClearSearchFlg();
			if ( !pData->IsStartEqEnd() )	// �n�_�ɖ߂��ް��ȊO
				nObCnt++;
		}
		dGap = GAPCALC(ptKey - pt);
		if ( nObCnt & 0x01 ) {
			nOddCnt++;
			// ���D��I�Ɍ���
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pStartArray = pArray;
				ptStart = pt;
			}
		}
		else {
			// �����̏ꍇ�ł���ԋ߂����W���m��
			if ( dGap < dGapMin2 ) {
				dGapMin2 = dGap;
				pStartArray2 = pArray;
				ptStart2 = pt;
			}
		}
	}
	if ( nOddCnt == 0 ) {		// ����[��
		ASSERT( pStartArray2 );
		bEuler = TRUE;		// ��M�����v���𖞂����Ă���
		// ����W���Ȃ��ꍇ�͋������W�̍ł��߂���޼ު�Ĕz����g��
		pStartArray = pStartArray2;
		ptStart = ptStart2;
	}
	else if ( nOddCnt == 2 ) {	// ��_���Q��
		ASSERT( pStartArray );
		bEuler = TRUE;		// ��M�����v���𖞂����Ă���
	}

	return make_tuple(bEuler, pStartArray, ptStart);
}

DWORD CDXFmap::GetMapTypeFlag(void) const
{
	// �P�̍��W�ɂR�ȏ�̵�޼ު�Ă������
	//		DXFMAPFLG_CANNOTWORKING -> ���H�w���ł��Ȃ�
	// ��_��[�_�������
	//		DXFMAPFLG_CANNOTOUTLINE -> �����w���͉\�C�֊s�E�߹�ĉ��H���ł��Ȃ�

	// ������޼ު�Ă��P�� ���� ���ꂪ�~�Ȃ�
	if ( GetObjectCount()==1 && GetFirstObject()->GetType()==DXFCIRCLEDATA )
		return 0;

	int			i, j, nLoop;
	DWORD		dwFlags = 0;
	CDXFarray	obWorkArray;
	CDXFarray*	pArray;
	CDXFdata*	pData;
#ifdef _DEBUG
	CDXFdata*	pDataDbg;
#endif
	CPointD		pt, ptChk[4];

	obWorkArray.SetSize(0, GetSize());
	AllMapObject_ClearSearchFlg(FALSE);

	try {
		for ( POSITION pos=GetStartPosition(); pos; ) {
			GetNextAssoc(pos, pt, pArray);
			nLoop = pArray->GetSize();
			if ( nLoop == 1 )			// �[�_
				dwFlags |= DXFMAPFLG_CANNOTOUTLINE;
			else if ( nLoop != 2 ) {	// �R�ȏ�̵�޼ު��
				dwFlags |= DXFMAPFLG_CANNOTWORKING;
				break;	// ����ȏ㒲���̕K�v�Ȃ�
			}
			// ��_�����̂��߂ɵ�޼ު�Ă��߰
			if ( !dwFlags ) {
				for ( i=0; i<nLoop; i++ ) {
					pData = pArray->GetAt(i);
					if ( !pData->IsSearchFlg() ) {
						obWorkArray.Add(pData);		// ܰ��z��ɓo�^
						pData->SetSearchFlg();
						pData->ClearMakeFlg();
					}
				}
			}
		}
		if ( !dwFlags ) {
			// ��_����
			nLoop = obWorkArray.GetSize();
			for ( i=0; i<nLoop; i++ ) {
				pData = obWorkArray[i];
				for ( j=i+1; j<nLoop; j++ ) {
#ifdef _DEBUG
					pDataDbg = obWorkArray[j];	// obWorkArray[j]�l�m�F�p
#endif
					if ( pData->GetIntersectionPoint(obWorkArray[j], ptChk) > 0 ) {
						dwFlags |= DXFMAPFLG_CANNOTOUTLINE;
						i = nLoop;	// �O��ٰ�߂��I��
						break;
					}
				}
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	return dwFlags;
}

BOOL CDXFmap::IsAllSearchFlg(void) const
{
	int			i;
	BOOL		bResult = TRUE;
	CPointD		pt;
	CDXFarray*	pArray;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			if ( !pArray->GetAt(i)->IsSearchFlg() ) {
				pos = NULL;
				bResult = FALSE;
				break;
			}
		}
	}

	return bResult;
}

void CDXFmap::AllMapObject_ClearSearchFlg(BOOL bMake/*=TRUE*/) const
{
	int			i;
	CPointD		pt;
	CDXFdata*	pData;
	CDXFarray*	pArray;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !bMake || (bMake && !pData->IsMakeFlg()) )
				pData->ClearSearchFlg();
		}
	}
}

void CDXFmap::AllMapObject_ClearMakeFlg() const
{
	int			i;
	CPointD		pt;
	CDXFarray*	pArray;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->ClearMakeFlg();
	}
}

CDXFdata* CDXFmap::GetFirstObject(void) const
{
	CPointD		pt;
	CDXFarray*	pArray;
	CDXFdata*	pData = NULL;
	POSITION	pos = GetStartPosition();

	if ( pos ) {
		GetNextAssoc(pos, pt, pArray);
		if ( !pArray->IsEmpty() )
			pData = pArray->GetAt(0);
	}

	return pData;
}

void CDXFmap::CopyToChain(CDXFchain* pChain)
{
	int			i, nLoop;
	CDXFarray*	pArray;
	CDXFdata*	pData = GetFirstObject();	// �����(?)�ȍŏ��̵�޼ު�Ă��擾
	CPointD		pt(pData->GetNativePoint(1));

	AllMapObject_ClearMakeFlg();
	pChain->AddTail(pData);
	pData->SetMakeFlg();

	while ( Lookup(pt, pArray) ) {
		nLoop = pArray->GetSize();
		for ( i=0; i<nLoop; i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pData->SetDirectionFixed(pt);
				pChain->AddTail(pData);
				pData->SetMakeFlg();
				pt = pData->GetNativePoint(1);
				break;
			}
		}
		if ( i >= nLoop )
			break;
	}
}

void CDXFmap::Append(const CDXFmap* pMap)
{
	CPointD		pt;
	CDXFarray*	pArraySrc;
	CDXFarray*	pArrayDst;

	for ( POSITION pos=pMap->GetStartPosition(); pos; ) {
		pMap->GetNextAssoc(pos, pt, pArraySrc);
		if ( !pArraySrc->IsEmpty() ) {
			if ( Lookup(pt, pArrayDst) )
				pArrayDst->InsertAt(pArrayDst->GetCount(), pArraySrc);	// Append�g���Ȃ�
			else {
				pArrayDst = new CDXFarray;
				ASSERT( pArrayDst );
				pArrayDst->SetSize(0, 16);
				pArrayDst->InsertAt(0, pArraySrc);		// Copy�g���Ȃ�
				SetAt(pt, pArrayDst);
			}
		}
	}
}

void CDXFmap::Append(const CDXFchain* pChain)
{
	for ( POSITION pos=pChain->GetHeadPosition(); pos; )
		SetPointMap( pChain->GetNext(pos) );
}

int CDXFmap::GetObjectCount(void) const
{
	int			i, nCnt = 0;
	CPointD		pt;
	CDXFdata*	pData;
	CDXFarray*	pArray;

	AllMapObject_ClearSearchFlg(FALSE);

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsSearchFlg() ) {
				nCnt++;
				pData->SetSearchFlg();
			}
		}
	}

	return nCnt;
}

double CDXFmap::GetSelectObjectFromShape
	(const CPointD& pt, const CRectD* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	int		i;
	CPointD	ptKey;
	double	dGap, dGapMin = HUGE_VAL;
	CDXFarray*	pArray;
	CDXFarray*	pArrayMin = NULL;
	CDXFdata*	pData;

	if ( pDataResult )
		*pDataResult = NULL;

	// �w����W�Ɉ�ԋ߂���޼ު�Ă�����
	// --- �u��ԋ߂����W���v�ł͒[�_�̌����ɂȂ��Ă��܂� ---
	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, ptKey, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( rcView && !rcView->PtInRectpt(pData->GetMaxRect()) )
				continue;
			dGap = pData->GetSelectPointGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				if ( pDataResult )
					*pDataResult = pData;
			}
		}
	}

	return dGapMin;
}

void CDXFmap::SetShapeSwitch(BOOL bSelect)
{
	CDXFarray*	pArray;
	CPointD	pt;
	int		i;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->SetSelectFlg(bSelect);
	}
}

void CDXFmap::RemoveObject(const CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointD	pt;
	int		i, j;

	for ( i=0; i<pData->GetPointNumber(); i++ ) {
		pt = pData->GetNativePoint(i);	// �ŗL���W�l
		if ( Lookup(pt, pArray) ) {
			for ( j=0; j<pArray->GetSize(); j++ ) {
				if ( pData == pArray->GetAt(j) ) {
					pArray->RemoveAt(j);
					break;
				}
			}
			if ( pArray->IsEmpty() ) {
				delete	pArray;
				RemoveKey(pt);
			}
		}
	}
}

void CDXFmap::DrawShape(CDC* pDC) const
{
	CDXFarray*	pArray;
	CDXFdata*	pData;
	CPointD	pt;
	int		i;
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			dwSel = pData->GetSelectFlg() & DXFSEL_SELECT;
			if ( dwSel != dwSelBak ) {
				dwSelBak = dwSel;
				pDC->SelectObject(pData->GetDrawPen());
			}
			pData->Draw(pDC);
		}
	}
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

void CDXFmap::OrgTuning(void)
{
	CDXFarray*	pArray;
	CPointD	pt;
	int		i;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->OrgTuning(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̗֊s�W�c�N���X
/////////////////////////////////////////////////////////////////////////////
CDXFchain::CDXFchain()
{
	m_rcMax.SetRectMinimum();
}

CDXFchain::~CDXFchain()
{
}

void CDXFchain::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {
		ar << GetCount();
		for ( POSITION pos=GetHeadPosition(); pos; )
			ar << GetNext(pos)->GetSerializeSeq();
	}
	else {
		INT_PTR		nListLoop;
		DWORD		nIndex;
		CDXFdata*	pData;
		CLayerData*	pLayer = (CLayerData *)(ar.m_pDocument);
		ar >> nListLoop;
		while ( nListLoop-- ) {
			ar >> nIndex;
			pData = pLayer->GetDxfData(nIndex);
			AddTail(pData);
		}
	}
}

void CDXFchain::ReversPoint(void)
{
	// �S��޼ު�Ă̍��W���]
	for ( POSITION pos=GetHeadPosition(); pos; )
		GetNext(pos)->ReversePt();
}

void CDXFchain::CopyToMap(CDXFmap* pMap)
{
	for ( POSITION pos=GetHeadPosition(); pos; )
		pMap->SetPointMap( GetNext(pos) );
}

int CDXFchain::GetObjectCount(void) const
{
	return GetCount();
}

double CDXFchain::GetSelectObjectFromShape
	(const CPointD& pt, const CRectD* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	double	dGap, dGapMin = HUGE_VAL;
	CDXFdata*	pData;

	if ( pDataResult )
		*pDataResult = NULL;

	// �w����W�Ɉ�ԋ߂���޼ު�Ă�����
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		if ( rcView && !rcView->PtInRectpt(pData->GetMaxRect()) )
			continue;
		dGap = pData->GetSelectPointGap(pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			if ( pDataResult )
				*pDataResult = pData;
		}
	}

	return dGapMin;
}

void CDXFchain::SetShapeSwitch(BOOL bSelect)
{
	for ( POSITION pos=GetHeadPosition(); pos; )
		GetNext(pos)->SetSelectFlg(bSelect);
}

void CDXFchain::RemoveObject(const CDXFdata* pData)
{
	POSITION	pos1=GetHeadPosition(), pos2;
	while ( (pos2=pos1) ) {
		if ( pData == GetNext(pos1) ) {
			RemoveAt(pos2);
			break;
		}
	}
}

void CDXFchain::DrawShape(CDC* pDC) const
{
	CDXFdata*	pData;
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		dwSel = pData->GetSelectFlg() & DXFSEL_SELECT;
		if ( dwSel != dwSelBak ) {
			dwSelBak = dwSel;
			pDC->SelectObject(pData->GetDrawPen());
		}
		pData->Draw(pDC);
	}
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

void CDXFchain::OrgTuning(void)
{
	for ( POSITION pos=GetHeadPosition(); pos; )
		GetNext(pos)->OrgTuning(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̌`��W���N���X
/////////////////////////////////////////////////////////////////////////////
CDXFshape::CDXFshape()
{
	Constructor(DXFSHAPE_OUTLINE, NULL, 0);
}

CDXFshape::CDXFshape(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CDXFchain* pChain)
{
	m_vShape = pChain;
	Constructor(enAssemble, lpszShape, dwFlags);
	// ���Wϯ�߂̵�޼ު�čő��`�Ə���ϯ�߂̍X�V
	SetDetailInfo(pChain);
}

CDXFshape::CDXFshape(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CDXFmap* pMap)
{
	m_vShape = pMap;
	Constructor(enAssemble, lpszShape, dwFlags);
	// ���Wϯ�߂̵�޼ު�čő��`�Ə���ϯ�߂̍X�V
	SetDetailInfo(pMap);
}

void CDXFshape::Constructor(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags)
{
	m_enAssemble = enAssemble;
	m_dwFlags = dwFlags;
	m_dOffset = 1.0;	// ��̫�ĵ̾�Ēl
	m_nInOut  = -1;		// ��̫�ė֊s��������
	m_hTree   = NULL;
	if ( lpszShape && lstrlen(lpszShape) > 0 )
		m_strShape = lpszShape;
	// ��޼ު�ċ�`�̏�����
	m_rcMax.SetRectMinimum();
}

void CDXFshape::SetDetailInfo(CDXFchain* pChain)
{
	CDXFdata*	pData;
	for ( POSITION pos=pChain->GetHeadPosition(); pos; ) {
		pData = pChain->GetNext(pos);
		m_rcMax |= pData->GetMaxRect();
		pData->SetParentMap(this);
	}
}

void CDXFshape::SetDetailInfo(CDXFmap* pMap)
{
	CDXFarray*	pArray;
	CDXFdata*	pData;
	CPointD	pt;
	int		i;
	for ( POSITION pos=pMap->GetStartPosition(); pos; ) {
		pMap->GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			m_rcMax |= pData->GetMaxRect();
			pData->SetParentMap(this);
		}
	}
}

CDXFshape::~CDXFshape()
{
	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; )
		delete	m_ltWork.GetNext(pos);
	if ( m_vShape.which() == 0 )
		delete	get<CDXFchain*>(m_vShape);
	else
		delete	get<CDXFmap*>(m_vShape);
}

void CDXFshape::Serialize(CArchive& ar)
{
	WORD	nType, nAssemble;

	if ( ar.IsStoring() ) {
		nType = m_vShape.which();
		nAssemble = m_enAssemble;
		ar << (m_dwFlags & ~DXFMAPFLG_SELECT) << nAssemble << m_strShape;	// �I����ԏ���
		ar << m_dOffset << m_nInOut;
		ar << nType;
		if ( nType == 0 )
			get<CDXFchain*>(m_vShape)->Serialize(ar);
		else
			get<CDXFmap*>(m_vShape)->Serialize(ar);
	}
	else {
		CLayerData*	pLayer = (CLayerData *)(ar.m_pDocument);
		ar >> m_dwFlags >> nAssemble >> m_strShape;
		ar >> m_dOffset >> m_nInOut;
		ar >> nType;
		m_enAssemble = (DXFSHAPE_ASSEMBLE)nAssemble;
		if ( nType == 0 ) {
			CDXFchain* pChain = new CDXFchain;
			m_vShape = pChain;
			pChain->Serialize(ar);
			SetDetailInfo(pChain);
		}
		else {
			CDXFmap* pMap = new CDXFmap;
			m_vShape = pMap;
			pMap->Serialize(ar);
			SetDetailInfo(pMap);
		}
		// CDXFworking�رײ�ޏ��p��CDXFshape*��CLayerData::m_pActiveMap�Ɋi�[
		pLayer->SetActiveShape(this);
	}
	// ���H�w�����̼رײ��
	m_ltWork.Serialize(ar);
}

BOOL CDXFshape::AddWorkingData(CDXFworking* pWork, int nInOut/*=-1*/)
{
	// ��O�͏�ʂōs��
	m_ltWork.AddTail(pWork);
	// ϯ���׸ނ����ޯĵ�޼ު�Ă��׸ނ�ݒ�
	ENWORKINGTYPE nType = pWork->GetWorkingType();
	m_dwFlags |= g_dwMapFlag[nType];
	m_nInOut   = nInOut;
	CDXFdata*	pData = pWork->GetTargetObject();
	if ( pData && nType==WORK_DIRECTION )
		pData->SetWorkingFlag(DXFSEL_DIRECTIONFIX);
	return TRUE;
}

BOOL CDXFshape::DelWorkingData(CDXFworking* pDelWork, CDXFshape* pShape/*=NULL*/)
{
	CDXFworking*	pWork;
	CDXFdata*		pData;
	ENWORKINGTYPE	nType;
	POSITION	pos1, pos2;
	BOOL	bResult = FALSE;

	for ( pos1=m_ltWork.GetHeadPosition(); (pos2=pos1); ) {
		pWork = m_ltWork.GetNext(pos1);
		if ( pWork == pDelWork ) {
			m_ltWork.RemoveAt(pos2);
			nType = pWork->GetWorkingType();
			// �e���H�w�����Ƃ�ϯ���׸ނ�ر
			m_dwFlags &= ~g_dwMapFlag[nType];
			if ( pShape ) {
				// �t���ւ� { LinkShape, CDXFView::OnLButtonUp_Sel() �̂� }
				pShape->AddWorkingData(pWork);
			}
			else {
				// ��޼ު�Ă̏���
				pData = pWork->GetTargetObject();
				if ( pData && nType==WORK_DIRECTION )
					pData->SetWorkingFlag(DXFSEL_DIRECTIONFIX, FALSE);
				delete	pWork;
			}
			bResult = TRUE;
			break;
		}
	}

	return bResult;
}

tuple<CDXFworking*, CDXFdata*> CDXFshape::GetDirectionObject(void) const
{
	CDXFworking*	pWork;
	CDXFdata*		pData = NULL;

	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; ) {
		pWork = m_ltWork.GetNext(pos);
		if ( pWork->GetWorkingType() == WORK_DIRECTION ) {
			pData = pWork->GetTargetObject();
			break;
		}
	}

	return make_tuple(pWork, pData);
}

CDXFchain*	CDXFshape::GetOutlineObject(void) const
{
	CDXFworking*	pWork;
	CDXFchain*		pChain = NULL;

	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; ) {
		pWork = m_ltWork.GetNext(pos);
		if ( pWork->GetWorkingType() == WORK_OUTLINE ) {
			pChain = ((CDXFworkingOutline *)pWork)->GetOutlineChain();
			break;
		}
	}

	return pChain;
}

void CDXFshape::RemoveExceptDirection(void)
{
	CDXFworking*	pWork;
	ENWORKINGTYPE	nType;
	POSITION	pos1=m_ltWork.GetHeadPosition(), pos2;
	while ( (pos2=pos1) ) {
		pWork = m_ltWork.GetNext(pos1);
		nType = pWork->GetWorkingType();
		if ( nType > WORK_START ) {		// WORK_OUTLINE, WORK_POCKET
			m_dwFlags &= ~g_dwMapFlag[nType];
			delete	pWork;
			m_ltWork.RemoveAt(pos2);
		}
	}
}

BOOL CDXFshape::LinkObject(void)
{
	CDXFmap*	pMap = GetShapeMap();

	// �W������
	if ( pMap ) {
		if ( pMap->GetMapTypeFlag() == 0 ) {
			// CDXFmap����CDXFchain�ɏ��i
			if ( !ChangeCreate_MapToChain(pMap) )
				return FALSE;
			delete	pMap;
		}
	}
	else {
		// CDXFchain����͕K��CDXFmap�ɍ~�i
		CDXFchain* pChain = GetShapeChain();
		if ( !ChangeCreate_ChainToMap(pChain) )
			return FALSE;
		delete	pChain;
	}

	return TRUE;
}

BOOL CDXFshape::ChangeCreate_MapToChain(CDXFmap* pMap)
{
	CDXFchain*	pChain = NULL;

	try {
		// �V���Ȍ`��W���𐶐�
		pChain = new CDXFchain;
		pMap->CopyToChain(pChain);
		// CDXFshape ���X�V
		m_enAssemble = DXFSHAPE_OUTLINE;
		m_vShape = pChain;
		m_dwFlags = 0;
		m_strShape += "(���i)";
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pChain )
			delete	pChain;
		return FALSE;
	}

	return TRUE;
}

BOOL CDXFshape::ChangeCreate_ChainToMap(CDXFchain* pChain)
{
	CDXFmap*	pMap = NULL;

	try {
		// �V���Ȍ`��W���𐶐�
		pMap = new CDXFmap;
		pChain->CopyToMap(pMap);
		// CDXFshape ���X�V
		m_enAssemble = DXFSHAPE_LOCUS;
		m_vShape = pMap;
		m_dwFlags = pMap->GetMapTypeFlag();
		m_strShape += "(�~�i)";
		// �K���Ȃ����H�w��(�����w���ȊO)���폜
		RemoveExceptDirection();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pMap )
			delete	pMap;
		return FALSE;
	}

	return TRUE;
}

BOOL CDXFshape::LinkShape(CDXFshape* pShape)
{
	CDXFmap*	pMapTmp = NULL;
	CDXFmap*	pMap;
	CDXFchain*	pChainOrg;
	CDXFchain*	pChain;
	CPointD		pt;
	POSITION	pos;
	BOOL		bResult = FALSE;

	try {
		// �������g��CDXFmap�ꎞ�̈��
		pMapTmp = new CDXFmap;
		pChainOrg = GetShapeChain();
		if ( pChainOrg )
			pChainOrg->CopyToMap(pMapTmp);
		else
			pMapTmp->Append(GetShapeMap());
		// �����o���邩�ǂ���(���W����)
		pChain = pShape->GetShapeChain();
		if ( pChain ) {
			for ( pos=pChain->GetHeadPosition(); pos; ) {
				pt = pChain->GetNext(pos)->GetNativePoint(1);
				if ( pMapTmp->PLookup(pt) ) {
					// �P�ł�������Ό�������
					bResult = TRUE;
					pMapTmp->Append(pChain);
					break;
				}
			}
		}
		else {
			pMap = pShape->GetShapeMap();
			CDXFarray*	pDummy;
			for ( pos=pMap->GetStartPosition(); pos; ) {
				pMap->GetNextAssoc(pos, pt, pDummy);
				if ( pMapTmp->PLookup(pt) ) {
					bResult = TRUE;
					pMapTmp->Append(pMap);
					break;
				}
			}
		}
		if ( !bResult ) {
			delete	pMapTmp;
			return FALSE;
		}
		// �s�v���H�w��(�����w���ȊO)���폜
		RemoveExceptDirection();
		// �����w���̓���
		CDXFworking*	pWork1;
		CDXFworking*	pWork2;
		CDXFdata*		pData1;
		CDXFdata*		pData2;
		tie(pWork1, pData1) = GetDirectionObject();
		tie(pWork2, pData2) = pShape->GetDirectionObject();
		if ( pData1 ) {
			// �������g�ɕ����w��������
			if ( pData2 ) {
				// ����ɂ������w��������
				pShape->DelWorkingData(pWork2);
			}
		}
		else if ( pData2 ) {
			// �������g�ɕ����w�����Ȃ��C����ɂ���
			pShape->DelWorkingData(pWork2, this);	// �����w���̕t���ւ�
		}
		// �W������
		DWORD dwFlag = pMapTmp->GetMapTypeFlag();
		if ( dwFlag == 0 ) {
			// ���i
			pMap = pChainOrg ? NULL : GetShapeMap();
			if ( ChangeCreate_MapToChain(pMapTmp) ) {	// pMapTmp����CDXFchain����
				// ���W���폜
				if ( pChainOrg )
					delete	pChainOrg;
				else
					delete	pMap;
			}
			else
				bResult = FALSE;
		}
		else {
			if ( pChainOrg ) {
				// �~�i(CDXFchain����CDXFmap�𐶐����Ă�����)
				if ( ChangeCreate_ChainToMap(pChainOrg) ) {
					delete	pChainOrg;
					pMap = GetShapeMap();
					// ����W����ǉ�
					pChain = pShape->GetShapeChain();
					if ( pChain )
						pMap->Append(pChain);
					else
						pMap->Append(pShape->GetShapeMap());
					// �`���׸ނ̍X�V(ChangeCreate_ChainToMap�ł͏o���Ȃ�)
					SetShapeFlag(dwFlag);
				}
				else
					bResult = FALSE;
			}
			else {
				// ����ێ�(pMapTmp���㏑����߰)
				pMap = GetShapeMap();
				pMap->RemoveAll();
				pMap->Append(pMapTmp);
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pMapTmp )
			delete	pMapTmp;
		return FALSE;
	}

	if ( pMapTmp )
		delete	pMapTmp;

	return bResult;
}

BOOL CDXFshape::CreateOutlineTempObject(BOOL bLeft, CDXFchain* pResult) const
{
	CDXFchain*	pChain = GetShapeChain();
	if ( !pChain || pChain->IsEmpty() )
		return FALSE;

	CDXFdata*	pData;
	CPointD		pte;
	optional<CPointD>	ptResult, pt;

	// �B��̵�޼ު�Ă��~�ް��Ȃ�
	if ( pChain->GetCount() == 1 ) {
		pData = pChain->GetHead();
		if ( pData->GetType() == DXFCIRCLEDATA ) {
			pData = CreateOutlineTempObject_new(pData, pte, pte, bLeft);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				return TRUE;
			}
		}
		return FALSE;	// �~�ް��ȊO�ʹװ
	}

	// �֊s��޼ު��ٰ�ߏ���
	POSITION	pos = pChain->GetHeadPosition();
	CDXFdata*	pData1 = pChain->GetNext(pos);	// �ŏ��̵�޼ު��
	CDXFdata*	pData2;

	// CDXFchain�́C�n�_�I�_�̒������s���Ă���(SetDirectionFixed)�̂�
	// �P��ٰ�߂ŗǂ�
	while ( pos ) {		// �Q��ڈȍ~����ٰ��
		pData2 = pChain->GetNext(pos);
		// �̾�č��W�v�Z
		ptResult = pData1->CalcOffsetIntersectionPoint(pData2, m_dOffset, bLeft);
		if ( !ptResult )
			return FALSE;
		// �֊s��޼ު�Đ���(����͖���)
		if ( pt ) {
			pData = CreateOutlineTempObject_new(pData1, *pt, *ptResult, bLeft);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
			}
			else
				return FALSE;
		}
		else
			pte = *ptResult;	// �ŏ��̗֊s��޼ު�Ă̏I�_
		pt = ptResult;
		pData1 = pData2;
	}
	// �c��̗֊s��޼ު�Đ���
	ptResult = pData1->CalcOffsetIntersectionPoint(pChain->GetHead(), m_dOffset, bLeft);
	if ( !ptResult )
		return FALSE;
	pData = CreateOutlineTempObject_new(pData1, *pt, *ptResult, bLeft);
	if ( pData ) {
		pResult->AddTail(pData);
		pResult->SetMaxRect(pData);
	}
	else
		return FALSE;
	pData = CreateOutlineTempObject_new(pChain->GetHead(), *ptResult, pte, bLeft);
	if ( pData ) {
		pResult->AddTail(pData);
		pResult->SetMaxRect(pData);
	}
	else
		return FALSE;

	return TRUE;
}

CDXFdata* CDXFshape::CreateOutlineTempObject_new
	(const CDXFdata* pDataSrc, const CPointD& pts, const CPointD& pte, BOOL bLeft) const
{
	CDXFdata*	pData = NULL;
	DXFLARGV	dxfLine;
	DXFCARGV	dxfCircle;
	DXFAARGV	dxfArc;

	switch ( pDataSrc->GetType() ) {
	case DXFLINEDATA:
		dxfLine.pLayer = pDataSrc->GetParentLayer();
		dxfLine.s = pts;
		dxfLine.e = pte;
		pData = new CDXFline(&dxfLine);
		break;
	case DXFCIRCLEDATA:
		dxfCircle.c = ((CDXFcircle *)pDataSrc)->GetCenter();
		dxfCircle.r = ((CDXFcircle *)pDataSrc)->GetR() + m_dOffset * (bLeft ? 1 : -1);
		pData = new CDXFcircle(&dxfCircle);
		break;
	case DXFARCDATA:
		{
			BOOL	bRound = ((CDXFarc *)pDataSrc)->GetRoundOrig();
			dxfArc.c = ((CDXFarc *)pDataSrc)->GetCenter();
			if ( bRound )
				dxfArc.r = ((CDXFarc *)pDataSrc)->GetR() + m_dOffset * ( bLeft ? -1 : 1 );
			else
				dxfArc.r = ((CDXFarc *)pDataSrc)->GetR() + m_dOffset * ( bLeft ? 1 : -1 );
			if ( (dxfArc.sq=atan2(pts.y - dxfArc.c.y, pts.x - dxfArc.c.x)) < 0.0 )
				dxfArc.sq += 360.0*RAD;
			if ( (dxfArc.eq=atan2(pte.y - dxfArc.c.y, pte.x - dxfArc.c.x)) < 0.0 )
				dxfArc.eq += 360.0*RAD;
			if ( bRound ) {
				while ( dxfArc.sq - dxfArc.eq >= 0 )
					dxfArc.eq += 360.0*RAD;
			}
			else {
				while ( dxfArc.eq - dxfArc.sq >= 0 )
					dxfArc.sq += 360.0*RAD;
			}
			pData = new CDXFarc(&dxfArc, bRound, pts, pte);
		}
		break;
	}

	return pData;
}

void CDXFshape::AllChangeFactor(double dFactor) const
{
	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; )
		m_ltWork.GetNext(pos)->DrawTuning(dFactor);
}

void CDXFshape::DrawWorking(CDC* pDC) const
{
	CDXFworking*	pWork;
	int		i = 0, j;
	CPen*	pPen[] = {
		AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER),
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL)
	};
	CPen*	pOldPen   = pDC->SelectObject(pPen[i]);
	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	pDC->SetROP2(R2_COPYPEN);
	for ( POSITION pos = m_ltWork.GetHeadPosition(); pos; ) {
		pWork = m_ltWork.GetNext(pos);
		j = pWork->GetWorkingFlag() & DXFWORKFLG_SELECT ? 1 : 0;
		if ( i != j ) {
			i = j;
			pDC->SelectObject( pPen[i] );
		}
		pWork->Draw(pDC);
	}
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

int CDXFshape::GetObjectCount(void) const
{
	return apply_visitor( GetObjectCount_Visitor(), m_vShape );
}

double CDXFshape::GetSelectObjectFromShape
	(const CPointD& pt, const CRectD* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	if ( m_vShape.which() == 0 )
		return get<CDXFchain*>(m_vShape)->GetSelectObjectFromShape(pt, rcView, pDataResult);
	else
		return get<CDXFmap*>(m_vShape)->GetSelectObjectFromShape(pt, rcView, pDataResult);
}

void CDXFshape::SetShapeSwitch(BOOL bSelect)
{
	if ( m_vShape.which() == 0 )
		get<CDXFchain*>(m_vShape)->SetShapeSwitch(bSelect);
	else
		get<CDXFmap*>(m_vShape)->SetShapeSwitch(bSelect);
}

void CDXFshape::RemoveObject(const CDXFdata* pData)
{
	if ( m_vShape.which() == 0 )
		get<CDXFchain*>(m_vShape)->RemoveObject(pData);
	else
		get<CDXFmap*>(m_vShape)->RemoveObject(pData);
}

void CDXFshape::DrawShape(CDC* pDC) const
{
	if ( m_vShape.which() == 0 )
		get<CDXFchain*>(m_vShape)->DrawShape(pDC);
	else
		get<CDXFmap*>(m_vShape)->DrawShape(pDC);
}

void CDXFshape::OrgTuning(void)
{
	apply_visitor( OrgTuning_Visitor(), m_vShape );
}
