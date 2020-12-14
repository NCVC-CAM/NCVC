// DXFshape.cpp: CDXFmap �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#define	_DEBUGOLD
//#define	_DEBUGDRAW_DXF
#endif

IMPLEMENT_DYNAMIC(CDXFworking, CObject)
IMPLEMENT_SERIAL(CDXFworkingDirection, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingStart, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingOutline, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingPocket, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFmap, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFchain, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFshape, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

using std::vector;
using namespace boost;

extern	DWORD	g_dwCamVer;		// NCVC.cpp

/////////////////////////////////////////////////////////////////////////////
// �ÓI�ϐ��̏�����
float	CDXFmap::ms_dTolerance = NCMIN;

static	DWORD	g_dwMapFlag[] = {
	DXFMAPFLG_DIRECTION, DXFMAPFLG_START,
	DXFMAPFLG_OUTLINE, DXFMAPFLG_POCKET
};

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^���H�w���̃x�[�X�N���X
/////////////////////////////////////////////////////////////////////////////
CDXFworking::CDXFworking
	(ENWORKINGTYPE enType, CDXFshape* pShape, CDXFdata* pData, DWORD dwFlags)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
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
	int	nStart = strWork.Find(gg_szReturn) + 1,			// '\n' �̎�����
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
		CLayerData*	pLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
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
	(CDXFshape* pShape, CDXFdata* pData, CPointF pts, CPointF pte[]) :
		CDXFworking(WORK_DIRECTION, pShape, pData, 0)
{
	m_ptStart = pts;
	for ( int i=0; i<SIZEOF(m_ptArraw); i++ )
		m_ptArraw[i] = pte[i];
}

void CDXFworkingDirection::DrawTuning(float f)
{
	m_ptDraw[1] = m_ptArraw[1] * f;
	m_ptDraw[0] = m_ptArraw[0] + m_ptDraw[1];
	m_ptDraw[2] = m_ptArraw[2] + m_ptDraw[1];
}

void CDXFworkingDirection::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	printf("CDXFworkingDirection::Draw() pt.x=%d pt.y=%d\n", m_ptDraw[1].x, m_ptDraw[1].y);
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
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CPointD	pt;
			ar >> pt.x >> pt.y;
			m_ptStart = pt;
			for ( i=0; i<SIZEOF(m_ptArraw); i++ ) {
				ar >> pt.x >> pt.y;
				m_ptArraw[i] = pt;
			}
		}
		else {
			ar >> m_ptStart.x >> m_ptStart.y;
			for ( i=0; i<SIZEOF(m_ptArraw); i++ )
				ar >> m_ptArraw[i].x >> m_ptArraw[i].y;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�J�n�ʒu�v�w���N���X
//////////////////////////////////////////////////////////////////////
CDXFworkingStart::CDXFworkingStart
	(CDXFshape* pShape, CDXFdata* pData, CPointF pts) :
		CDXFworking(WORK_START, pShape, pData, 0)
{
	m_ptStart = pts;
}

void CDXFworkingStart::DrawTuning(float f)
{
	CPointF	pt( m_ptStart * f ); 
	// �ʒu��\���ۈ�͏��2.5�_������ (CDXFpoint����)
	float	dFactor = LOMETRICFACTOR * 2.5;
	m_rcDraw.TopLeft()		= pt - dFactor;
	m_rcDraw.BottomRight()	= pt + dFactor;
}

void CDXFworkingStart::Draw(CDC* pDC) const
{
	pDC->Ellipse(&m_rcDraw);
}

void CDXFworkingStart::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_ptStart.x << m_ptStart.y;
	else {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CPointD	pt;
			ar >> pt.x >> pt.y;
			m_ptStart = pt;
		}
		else {
			ar >> m_ptStart.x >> m_ptStart.y;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�֊s�v���H�w���N���X
//////////////////////////////////////////////////////////////////////
CDXFworkingOutline::CDXFworkingOutline
	(CDXFshape* pShape, const CDXFchain* pOutline, float dOffset, DWORD dwFlags) :
		CDXFworking(WORK_OUTLINE, pShape, NULL, dwFlags)
{
	m_obOutline.SetSize(0, 64);
	m_obMergeHandle.SetSize(0, 64);
	m_rcMax.SetRectMinimum();
	m_dOffset = dOffset;		// �֊s���������ꂽ�Ƃ��̵̾�Ēl
	SeparateAdd_Construct(pOutline);
}

CDXFworkingOutline::~CDXFworkingOutline()
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ ) {
		CDXFchain* pChain = m_obOutline[i];
		PLIST_FOREACH(CDXFdata* pData, pChain)
			delete	pData;
		END_FOREACH
		delete	pChain;
	}
	m_obOutline.RemoveAll();
	m_obMergeHandle.RemoveAll();
}

void CDXFworkingOutline::SeparateAdd_Construct(const CDXFchain* pOutline)
{
	CDXFchain*	pChain = NULL;
	;
	// NULL �ŕ������ꂽ�W���𕪗����ēo�^
	PLIST_FOREACH(CDXFdata* pData, pOutline)
		if ( pData ) {
			if ( !pChain )
				pChain = new CDXFchain;
			pChain->AddTail(pData);
			pChain->SetMaxRect(pData);
		}
		else if ( pChain ) {
			m_obOutline.Add(pChain);
			pChain = NULL;
		}
	END_FOREACH

	if ( pChain )
		m_obOutline.Add(pChain);
	m_rcMax = pOutline->GetMaxRect();

#ifdef _DEBUG
	printf("SeparateAdd_Construct() OutlineSize=%d\n", m_obOutline.GetSize());
	for ( int i=0; i<m_obOutline.GetSize(); i++ ) {
		pChain = m_obOutline[i];
		int	j = 0;
		PLIST_FOREACH(auto ref, pChain)
			if ( !ref )
				printf("??? NULL element %d\n", j);
			j++;
		END_FOREACH
	}
#endif
}

void CDXFworkingOutline::SeparateModify(void)
{
#ifdef _DEBUG
	printf("CDXFworkingOutline::SeparateModify() Start\n");
	CPointF		ptDbgS, ptDbgE;
#endif
	INT_PTR		i, j, nLoop,
				nMainLoop = m_obOutline.GetSize();	// �r���Œǉ������̂Ő�ɍő�l�擾
	POSITION	pos, pos1, pos2;
	optional<CPointF>	pts, pte;
	CDXFdata*	pData;
	CDXFchain*	pOutline;
	CDXFchain*	pChain;
	vector<POSITION>	vPos;

	// ���łɓo�^����Ă����޼ު�Ăɑ΂���
	// NULL �ŕ������ꂽ�W���𕪗����ēo�^
	for ( i=0; i<nMainLoop; i++ ) {
		pOutline = m_obOutline[i];
		for ( pos1=pOutline->GetHeadPosition(); (pos2=pos1); ) {
			pData = pOutline->GetNext(pos1);
			if ( pData ) {
				if ( pData->GetDxfFlg() & DXFFLG_OFFSET_EXCLUDE ) {
					// �s�v��޼ު�Ă̍폜
					pOutline->RemoveAt(pos2);
					delete	pData;
				}
			}
			else
				vPos.push_back(pos2);	// NULL�߼޼�݂��L�^
		}
		nLoop = vPos.size();
		// ��������
		if ( nLoop > 1 ) {
			// NULL�Ԃ̵�޼ު�Ă�V�����W����
			for ( j=0; j<nLoop-1; j++ ) {
				pos = pos1 = vPos[j];
				pos2 = vPos[j+1];
				pOutline->GetNext(pos1);	// NULL�ް�(pos1++)
				pOutline->RemoveAt(pos);	// ��ؽĂ������
				if ( pos == pOutline->GetHeadPosition() )
					continue;	// �擪��NULL������ꍇ�́ANULL��������
				if ( pos1 != pos2 ) {		// �A��NULL���s�h�~
					pChain = new CDXFchain(DXFMAPFLG_SEPARATE);
					while ( (pos=pos1) && pos1!=pos2 ) {
						pData = pOutline->GetNext(pos1);
						pOutline->RemoveAt(pos);
						pChain->AddTail(pData);
						pChain->SetMaxRect(pData);
					}
					m_obOutline.Add(pChain);
				}
			}
		}
		if ( nLoop > 0 ) {
			pos2 = vPos.back();		// NULL���߼޼�ݏ��
			pts.reset();
			pte.reset();
			// �擪�ް��Ɩ����ް��̎n�_�I�_����ٰ�ߐڑ����f
			for ( pos=pOutline->GetHeadPosition(); pos; ) {
				pData = pOutline->GetNext(pos);
				if ( pData ) {
					pts = pData->GetNativePoint(0);
					break;
				}
			}
			for ( pos=pOutline->GetTailPosition(); pos; ) {
				pData = pOutline->GetPrev(pos);
				if ( pData ) {
					pte = pData->GetNativePoint(1);
					break;
				}
			}
			if ( pts && pte ) {
				if ( sqrt(GAPCALC(*pts - *pte)) < NCMIN ) {
					// �擪�ް��̎n�_�Ɩ����ް��̍��W���������ꍇ�́A
					// NULL�߼޼�݂���|����ɁA�[�_���擪�ɗ���悤�����ύX
					for ( pos1=pOutline->GetTailPosition(); pos1 && pos1!=pos2; ) {
						pData = pOutline->GetPrev(pos1);
						pOutline->RemoveTail();		// �Ōォ���������
						pOutline->AddHead(pData);	// �擪�Ɉړ�
					}
				}
				else if ( pOutline->GetHead() && pOutline->GetTail() ) {	// �擪��������NULL�łȂ����
					// NULL�ȍ~�𕪗�
					pChain = new CDXFchain(DXFMAPFLG_SEPARATE);
					for ( pos1=pOutline->GetTailPosition(); pos1 && pos1!=pos2; ) {
						pData = pOutline->GetPrev(pos1);
						pOutline->RemoveTail();
						pChain->AddHead(pData);
						pChain->SetMaxRect(pData);
					}
					m_obOutline.Add(pChain);
				}
			}
			pOutline->SetChainFlag(DXFMAPFLG_SEPARATE);
			// NULL����
			pOutline->RemoveAt(pos2);
			// ���̏�����
			vPos.clear();
		}
#ifdef _DEBUG
		pte.reset();
		printf("%s nLoop=%d\n", LPCTSTR(m_pShape->GetShapeName()), i);
		PLIST_FOREACH(pData, pOutline)
			ptDbgS = pData->GetNativePoint(0);
			ptDbgE = pData->GetNativePoint(1);
			printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
				ptDbgS.x, ptDbgS.y, ptDbgE.x, ptDbgE.y,
				pte && sqrt(GAPCALC(*pte-ptDbgS))>=NCMIN ? "X" : " ");
			pte = ptDbgE;
		END_FOREACH
#endif
	}

#ifdef _DEBUG
	if ( nMainLoop < m_obOutline.GetSize() ) {
		for ( i=nMainLoop; i<m_obOutline.GetSize(); i++ ) {
			pte.reset();
			printf("--- Separate Add %d ---\n", i);
			PLIST_FOREACH(pData, m_obOutline[i])
				ptDbgS = pData->GetNativePoint(0);
				ptDbgE = pData->GetNativePoint(1);
				printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
					ptDbgS.x, ptDbgS.y, ptDbgE.x, ptDbgE.y,
					pte && sqrt(GAPCALC(*pte-ptDbgS))>=NCMIN ? "X" : " ");
				pte = ptDbgE;
			END_FOREACH
		}
	}
#endif

	// �����W������
	while ( nMainLoop-- ) {		//	for ( i=nMainLoop-1; i>=0; i-- ) {
		pOutline = m_obOutline[nMainLoop];
		if ( pOutline->IsEmpty() ) {
#ifdef _DEBUG
			printf("--- Separete Delete %d ---\n", nMainLoop);
#endif
			delete	pOutline;
			m_obOutline.RemoveAt(nMainLoop);
		}
	}
}

void CDXFworkingOutline::SetMergeHandle(const CString& strHandle)
{
	int		i;
	for ( i=0; i<m_obMergeHandle.GetSize(); i++ ) {
		if ( m_obMergeHandle[i] == strHandle )
			break;
	}
	if ( i >= m_obMergeHandle.GetSize() )
		m_obMergeHandle.Add(strHandle);
}

void CDXFworkingOutline::DrawTuning(float f)
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ ) {
		PLIST_FOREACH(CDXFdata* pData, m_obOutline[i])
			pData->DrawTuning(f);
		END_FOREACH
	}
}

void CDXFworkingOutline::Draw(CDC* pDC) const
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ ) {
		PLIST_FOREACH(CDXFdata* pData, m_obOutline[i])
			pData->Draw(pDC);
		END_FOREACH
	}
}

void CDXFworkingOutline::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);

	CObject*	pNewData;	// CDXFdata*
	CDXFdata*	pData;
	CDXFchain*	pChain;
	int			i, nLoop1, nLoop2;

	// �֊s��޼ު�Ă̼رײ��
	// m_obOutline.Serialize(ar); �ł� CDXFchain ���ĂсC
	// ��޼ު�Ăւ� SeqNo. �ŏ��������D�䂦�Ɏ��ͺ��ިݸ�
	if ( ar.IsStoring() ) {
		// �֊s�̾�Ēl
		ar << m_dOffset;
		// �֊s��޼ު�Đ�
		nLoop1 = (int)m_obOutline.GetSize();
		ar << nLoop1;
		// �ް��{��
		for ( i=0; i<nLoop1; i++ ) {
			pChain = m_obOutline[i];
			nLoop2 =  (int)pChain->GetSize();
			ar << nLoop2 << pChain->GetChainFlag();
			PLIST_FOREACH(pData, pChain)
				ar << pData;
			END_FOREACH
		}
		// �������
		m_obMergeHandle.Serialize(ar);
		return;
	}

	if ( g_dwCamVer > NCVCSERIALVERSION_1507 ) {		// Ver1.60�`
		if ( g_dwCamVer > NCVCSERIALVERSION_1600 ) {	// Ver1.70�`
			if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
				double	d;
				ar >> d;
				m_dOffset = (float)d;
			}
			else {
				ar >> m_dOffset;
			}
		}
		else
			m_dOffset = m_pShape->GetOffset();
		DWORD	dwFlags;
		ar >> nLoop1;
		while ( nLoop1-- ) {
			ar >> i >> dwFlags;
			pChain = new CDXFchain(dwFlags);
			while ( i-- ) {
				ar >> pNewData;
				pData = static_cast<CDXFdata *>(pNewData);
				pChain->AddTail(pData);
				pChain->SetMaxRect(pData);
			}
			m_obOutline.Add(pChain);
			m_rcMax |= pChain->GetMaxRect();
		}
		m_obMergeHandle.Serialize(ar);
	}
	else {
		CDXFchain	ltOutline;
		DWORD		dwCnt = 0;	// �ް��̐؂�ډ�
		CDWordArray	obGap;		// �ް��̐؂�ڈꗗ
		INT_PTR		n = 0;
		// �ް��̐؂��(NULL�l)�̓ǂݍ���
		if ( g_dwCamVer > NCVCSERIALVERSION_1503 )	// Ver1.10�`
			obGap.Serialize(ar);
		dwCnt = obGap.IsEmpty() ? -1 : obGap[0];
		// �ް��{��
		ar >> nLoop1;
		for ( i=0; i<nLoop1; i++ ) {
			if ( i == dwCnt ) {
				ltOutline.AddTail( (CDXFdata *)NULL );
				dwCnt = n < obGap.GetUpperBound() ? obGap[++n] : -1;
			}
			else {
				ar >> pNewData;
				pData = static_cast<CDXFdata *>(pNewData);
				ltOutline.AddTail(pData);
			}
		}
		// �V�����ް��i�[���@�ɕϊ�
		SeparateAdd_Construct(&ltOutline);
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�|�P�b�g�v���H�w���N���X
//////////////////////////////////////////////////////////////////////
CDXFworkingPocket::CDXFworkingPocket(CDXFshape* pShape, DWORD dwFlags) :
	CDXFworking(WORK_POCKET, pShape, NULL, dwFlags)
{
	m_obPocket.SetSize(0, 64);
}

CDXFworkingPocket::~CDXFworkingPocket()
{
	for ( int i=0; i<m_obPocket.GetSize(); i++ )
		delete	m_obPocket[i];
	m_obPocket.RemoveAll();
}

void CDXFworkingPocket::DrawTuning(float f)
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

#ifdef _DEBUG
void CDXFmap::DbgDump(void) const
{
	CPointF		pt;
	CDXFarray*	pArray;

	printf("CDXFmap::DbgDump() Cnt=%d\n", GetCount());
	PMAP_FOREACH(pt, pArray, this)
		printf("Key=(%f, %f)\n", pt.x, pt.y);
		printf("DataCnt=%d\n", pArray->GetCount());
	END_FOREACH
}
#endif

void CDXFmap::Serialize(CArchive& ar)
{
	UINT		i, nMapCnt, nDataCnt;
	CPointF		pt;
	CDXFarray*	pArray;

	if ( ar.IsStoring() ) {
		nMapCnt = (UINT)GetCount();
		ar << nMapCnt << GetHashTableSize();
		PMAP_FOREACH(pt, pArray, this)
			nDataCnt = (UINT)pArray->GetSize();
			ar << pt.x << pt.y << nDataCnt;
			for ( i=0; i<nDataCnt; i++ )
				ar << pArray->GetAt(i)->GetSerializeSeq();
		END_FOREACH
	}
	else {
		UINT		nHash;
		DWORD		nIndex;
		CDXFdata*	pData;
		CLayerData*	pLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
		ar >> nMapCnt >> nHash;
		InitHashTable(nHash);
		while ( nMapCnt-- ) {
			if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
				CPointD	ptD;
				ar >> ptD.x >> ptD.y >> nDataCnt;
				pt = ptD;
			}
			else {
				ar >> pt.x >> pt.y >> nDataCnt;
			}
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
	CPointF	pt;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		if ( pArray )
			delete	pArray;
	END_FOREACH
	CMapPointToDXFarray::RemoveAll();
}

void CDXFmap::SetPointMap(CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointF	pt;
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
	CPointF	pt;

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

tuple<BOOL, CDXFarray*, CPointF>
CDXFmap::IsEulerRequirement(const CPointF& ptKey) const
{
	int			i, nObCnt, nOddCnt = 0;
	BOOL		bEuler = FALSE,	// ��M�����v���𖞂����Ă��邩
				bOne = FALSE;	// �[�_
	float		dGap, dGapMin = FLT_MAX, dGapMin2 = FLT_MAX;
	CPointF		pt, ptStart, ptStart2;
	CDXFdata*	pData;
	CDXFarray*	pArray;
	CDXFarray*	pStartArray = NULL;
	CDXFarray*	pStartArray2;

	// ��޼ު�ēo�^���̊��T�� + ����׸ނ̸ر
	PMAP_FOREACH(pt, pArray, this)
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
			// ���D��I�Ɍ���
			nOddCnt++;
			if ( nObCnt == 1 ) {
				// �[�_(1)������ɗD��
				if ( bOne ) {
					if ( dGap < dGapMin ) {
						dGapMin = dGap;
						pStartArray = pArray;
						ptStart = pt;
					}
				}
				else {
					bOne = TRUE;
					dGapMin = dGap;
					pStartArray = pArray;
					ptStart = pt;
				}
			}
			else if ( !bOne ) {
				// �[�_���Ȃ��������߂����W����
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pStartArray = pArray;
					ptStart = pt;
				}
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
	END_FOREACH
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
	//		DXFMAPFLG_EDGE|INTERSEC -> �����w���͉\�C�֊s�E�߹�ĉ��H���ł��Ȃ�
	INT_PTR		i, j, nLoop;
	DWORD		dwFlags = 0;
	CDXFarray	obWorkArray;
	CDXFarray*	pArray;
	CPointF		pt, ptChk[4];
	CDXFdata*	pData;

	// ������޼ު�Ă��P�� ���� ��ٰ�߂Ȃ�
	if ( GetObjectCount() == 1 ) {
		POSITION pos = GetStartPosition();
		GetNextAssoc(pos, pt, pArray);
		pData = pArray->GetAt(0);
		if ( !pData->IsStartEqEnd() )
			dwFlags |= DXFMAPFLG_EDGE;
		if ( pData->GetType()==DXFPOLYDATA &&
						static_cast<CDXFpolyline*>(pData)->IsIntersection() )
			dwFlags |= DXFMAPFLG_INTERSEC;
		return dwFlags;
	}

	obWorkArray.SetSize(0, GetSize());
	AllMapObject_ClearSearchFlg(FALSE);

	try {
		PMAP_FOREACH(pt, pArray, this)
			nLoop = pArray->GetSize();
			if ( nLoop == 1 )			// �[�_
				dwFlags |= DXFMAPFLG_EDGE;
			else if ( nLoop != 2 )		// �R�ȏ�̵�޼ު��
				return DXFMAPFLG_CANNOTWORKING;	// ����ȏ㒲���̕K�v�Ȃ�
			// ��_�����̂��߂ɵ�޼ު�Ă��߰
			for ( i=0; i<nLoop; i++ ) {
				pData = pArray->GetAt(i);
				if ( pData->GetType()==DXFPOLYDATA &&
						static_cast<CDXFpolyline*>(pData)->IsIntersection() ) {
					dwFlags |= DXFMAPFLG_INTERSEC;
					break;
				}
				if ( !pData->IsSearchFlg() ) {
					obWorkArray.Add(pData);		// ܰ��z��ɓo�^
					pData->SetSearchFlg();
					pData->ClearMakeFlg();
				}
			}
		END_FOREACH
		// ��_����
		if ( !(dwFlags & DXFMAPFLG_INTERSEC) ) {
			nLoop = obWorkArray.GetSize();
			for ( i=0; i<nLoop; i++ ) {
				pData = obWorkArray[i];
				for ( j=i+1; j<nLoop; j++ ) {
					if ( pData->GetIntersectionPoint(obWorkArray[j], ptChk) > 0 ) {
#ifdef _DEBUG
						printf("Intersection !!! i=%d j=%d\n", i, j);
						pData->DbgDump();
						obWorkArray[j]->DbgDump();
#endif
						dwFlags |= DXFMAPFLG_INTERSEC;
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
	CPointF		pt;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			if ( !pArray->GetAt(i)->IsSearchFlg() ) {
				pos = NULL;
				bResult = FALSE;
				break;
			}
		}
	END_FOREACH

	return bResult;
}

BOOL CDXFmap::IsAllMakeFlg(void) const
{
	int			i;
	BOOL		bResult = TRUE;
	CPointF		pt;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			if ( !pArray->GetAt(i)->IsMakeFlg() ) {
				pos = NULL;
				bResult = FALSE;
				break;
			}
		}
	END_FOREACH

	return bResult;
}

void CDXFmap::AllMapObject_ClearSearchFlg(BOOL bMake/*=TRUE*/) const
{
	int			i;
	CPointF		pt;
	CDXFdata*	pData;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !bMake || (bMake && !pData->IsMakeFlg()) )
				pData->ClearSearchFlg();
		}
	END_FOREACH
}

void CDXFmap::AllMapObject_ClearMakeFlg(void) const
{
	int			i;
	CPointF		pt;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->ClearMakeFlg();
	END_FOREACH
}

BOOL CDXFmap::CopyToChain(CDXFchain* pChain)
{
	INT_PTR		i, nLoop;
	CPointF		pt, pts, pte;
	CDXFarray*	pArray;
	CDXFdata*	pData = NULL;

	// �n�_�̒[�_����
	PMAP_FOREACH(pt, pArray, this)
		if ( pArray->GetSize() == 1 ) {
			pData = pArray->GetAt(0);
			break;
		}
		if ( !pData && pArray->GetSize()>0 )
			pData = pArray->GetAt(0);
	END_FOREACH
	if ( !pData )
		return FALSE;

	pts = pData->GetNativePoint(0);
	pte = pData->GetNativePoint(1);
	if ( GAPCALC(pts-pt) > GAPCALC(pte-pt) )
		pData->SwapNativePt();
	pt = pData->GetNativePoint(1);	// �I�_==���̍��W��

	AllMapObject_ClearMakeFlg();
	pChain->AddTail(pData);
	pChain->SetMaxRect(pData);
	pData->SetMakeFlg();

	while ( Lookup(pt, pArray) ) {
		nLoop = pArray->GetSize();
		for ( i=0; i<nLoop; i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pts = pData->GetNativePoint(0);
				pte = pData->GetNativePoint(1);
				if ( GAPCALC(pts-pt) > GAPCALC(pte-pt) )
					pData->SwapNativePt();
				pChain->AddTail(pData);
				pChain->SetMaxRect(pData);
				pData->SetMakeFlg();
				pt = pData->GetNativePoint(1);
				break;
			}
		}
		if ( i >= nLoop )
			break;
	}

	return TRUE;
}

void CDXFmap::Append(const CDXFmap* pMap)
{
	CPointF		pt;
	CDXFarray*	pArraySrc;
	CDXFarray*	pArrayDst;

	PMAP_FOREACH(pt, pArraySrc, pMap)
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
	END_FOREACH
}

void CDXFmap::Append(const CDXFchain* pChain)
{
	PLIST_FOREACH(CDXFdata* pData, pChain)
		SetPointMap(pData);
	END_FOREACH
}

INT_PTR CDXFmap::GetObjectCount(void) const
{
	INT_PTR		i, nCnt = 0;
	CPointF		pt;
	CDXFdata*	pData;
	CDXFarray*	pArray;

	AllMapObject_ClearSearchFlg(FALSE);

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsSearchFlg() ) {
				nCnt++;
				pData->SetSearchFlg();
			}
		}
	END_FOREACH

	return nCnt;
}

float CDXFmap::GetSelectObjectFromShape
	(const CPointF& pt, const CRectF* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	int		i;
	CPointF	ptKey;
	CRectF	rc, _rcView;
	float	dGap, dGapMin = FLT_MAX;
	CDXFarray*	pArray;
	CDXFarray*	pArrayMin = NULL;
	CDXFdata*	pData;

	if ( pDataResult )
		*pDataResult = NULL;
	if ( rcView )
		_rcView.SetRect(rcView);

	// �w����W�Ɉ�ԋ߂���޼ު�Ă�����
	// --- �u��ԋ߂����W���v�ł͒[�_�̌����ɂȂ��Ă��܂� ---
	PMAP_FOREACH(ptKey, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( rcView && !rc.CrossRect(_rcView, pData->GetMaxRect()) )
				continue;
			dGap = pData->GetSelectPointGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				if ( pDataResult )
					*pDataResult = pData;
			}
		}
	END_FOREACH

	return dGapMin;
}

void CDXFmap::SetShapeSwitch(BOOL bSelect)
{
	CDXFarray*	pArray;
	CPointF	pt;

	PMAP_FOREACH(pt, pArray, this)
		for ( int i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->SetDxfFlg(DXFFLG_SELECT, bSelect);
	END_FOREACH
}

void CDXFmap::RemoveObject(const CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointF	pt;
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
	CPointF	pt;
	int		i;
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			dwSel = pData->GetDxfFlg() & DXFFLG_SELECT;
			if ( dwSel != dwSelBak ) {
				dwSelBak = dwSel;
				pDC->SelectObject(pData->GetDrawPen());
			}
			pData->Draw(pDC);
		}
	END_FOREACH
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

void CDXFmap::OrgTuning(void)
{
	CDXFarray*	pArray;
	CPointF	pt;

	PMAP_FOREACH(pt, pArray, this)
		for ( int i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->OrgTuning(FALSE);
	END_FOREACH
}

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̗֊s�W�c�N���X
/////////////////////////////////////////////////////////////////////////////
CDXFchain::CDXFchain(DWORD dwFlag/*=0*/)
{
	m_rcMax.SetRectMinimum();
	m_dwFlags = dwFlag;
}

CDXFchain::~CDXFchain()
{
}

void CDXFchain::Serialize(CArchive& ar)
{
	UINT		nLstCnt;
	CDXFdata*	pData;

	if ( ar.IsStoring() ) {
		nLstCnt = (UINT)GetCount();
		ar << nLstCnt << m_dwFlags;
		PLIST_FOREACH(CDXFdata* pData, this)
			ar << pData->GetSerializeSeq();
		END_FOREACH
	}
	else {
		DWORD		nIndex;
		CLayerData*	pLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
		ar >> nLstCnt >> m_dwFlags;
		while ( nLstCnt-- ) {
			ar >> nIndex;
			if ( nIndex >= 0 ) {
				pData = pLayer->GetDxfData(nIndex);
				SetMaxRect(pData);
				AddTail(pData);
			}
		}
	}
}

void CDXFchain::SetMakeFlags(void)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->SetMakeFlg();
	END_FOREACH
}

void CDXFchain::ReverseMakePt(void)
{
	// �S��޼ު�Ă̍��W���]
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->SwapMakePt(0);
	END_FOREACH
}

void CDXFchain::ReverseNativePt(void)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->SwapNativePt();
	END_FOREACH
}

void CDXFchain::CopyToMap(CDXFmap* pMap)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pMap->SetPointMap(pData);
	END_FOREACH
}

BOOL CDXFchain::IsLoop(void) const
{
	if ( IsEmpty() )
		return FALSE;
	if ( GetCount()==1 && GetHead()->IsStartEqEnd() )
		return TRUE;

	CPointF	pts( GetHead()->GetNativePoint(0) ),
			pte( GetTail()->GetNativePoint(1) );
	return sqrt(GAPCALC(pts-pte)) < NCMIN;
}

BOOL CDXFchain::IsPointInPolygon(const CPointF& ptTarget) const
{
	CDXFdata*	pData;
	CPointF		pt;
	CVPointF	vpt;

	if ( GetCount() == 1 ) {
		pData = GetHead();
		if ( pData && pData->GetType()==DXFCIRCLEDATA ) {
			CDXFcircle* pCircle = static_cast<CDXFcircle*>(pData);
			pt = ptTarget - pCircle->GetCenter();
			return pt.hypot() < pCircle->GetR();
		}
	}

	BOOL	bNext = TRUE;
	// �e��޼ު�Ă̎n�_��z���
	PLIST_FOREACH(pData, this)
		if ( pData ) {
			switch ( pData->GetType() ) {
			case DXFLINEDATA:
				if ( bNext )
					pData->SetVectorPoint(vpt);
				bNext = TRUE;
				break;
			case DXFARCDATA:
			case DXFELLIPSEDATA:
			case DXFPOLYDATA:
				pData->SetVectorPoint(vpt);
				bNext = FALSE;	// �I�_�܂œo�^���邩�玟��DXFLINEDATA�̎n�_�͖���
				break;
			default:
				bNext = TRUE;
			}
		}
	END_FOREACH

	if ( bNext ) {
		// �ŏI��޼ު�Ă̏I�_���
		for ( POSITION pos=GetTailPosition(); pos; ) {
			pData = GetPrev(pos);
			if ( pData ) {
				vpt.push_back(pData->GetNativePoint(1));
				break;
			}
		}
	}
/*
	// �Ȃ�ׂ���ٰ�߂ɂȂ�悤��
	CPointF	pts(vpt.front()), pte(vpt.back());
	if ( sqrt(GAPCALC(pts-pte)) >= NCMIN )
		vpt.push_back(pts);
*/
	return ::IsPointInPolygon(ptTarget, vpt);
}

POSITION CDXFchain::SetLoopFunc(const CDXFdata* pData, BOOL bReverse, BOOL bNext)
{
	POSITION	pos1, pos2;

	if ( bReverse ) {
		ReverseMakePt();	// ���W�ް����]
		m_pfnGetFirstPos	= &CDXFchain::GetTailPosition;
		m_pfnGetFirstData	= &CDXFchain::GetTail;
		m_pfnGetData		= &CDXFchain::GetPrev;
	}
	else {
		m_pfnGetFirstPos	= &CDXFchain::GetHeadPosition;
		m_pfnGetFirstData	= &CDXFchain::GetHead;
		m_pfnGetData		= &CDXFchain::GetNext;
	}

	// �J�n��޼ު�Ă̌���
	if ( pData && IsLoop() ) {
		for ( pos1=(this->*m_pfnGetFirstPos)(); (pos2=pos1); ) {
			if ( pData == (this->*m_pfnGetData)(pos1) )
				break;
		}
		pos1 = pos2;	// pData ���߼޼��
		// �J�n��޼ު�Ă̍ŏI����
		if ( bNext ) {
			for ( int i=0; i<2; i++ ) {	// �������g�Ǝ��̵�޼ު�Ă��߼޼��
				pos2 = pos1;
				(this->*m_pfnGetData)(pos1);
				if ( !pos1 )
					pos1 = (this->*m_pfnGetFirstPos)();
			}
			pos1 = pos2;
		}
	}
	else {
		pos1 = pos2 = (this->*m_pfnGetFirstPos)();
	}

	ASSERT( pos1 );

#ifdef _DEBUG
	optional<CPointF>	ptDbg;
	CPointF		ptDbg1, ptDbg2;
	CDXFdata*	pDataDbg;
	POSITION	posDbg;
	// CDXFchainؽč\���������ǂ�����ł��邩
	printf("--- pChain Fin\n");
	ptDbg.reset();

	for ( posDbg=pos1; posDbg; ) {
		pDataDbg = (this->*m_pfnGetData)(posDbg);
		ptDbg1 = pDataDbg->GetTunPoint(0);
		ptDbg2 = pDataDbg->GetTunPoint(1);
		printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y,
			ptDbg && sqrt(GAPCALC(*ptDbg-ptDbg1))>=NCMIN ? "X" : " ");
		ptDbg = ptDbg2;
	}
	for ( posDbg=(this->*m_pfnGetFirstPos)(); posDbg!=pos2; ) {
		pDataDbg = (this->*m_pfnGetData)(posDbg);
		ptDbg1 = pDataDbg->GetTunPoint(0);
		ptDbg2 = pDataDbg->GetTunPoint(1);
		printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y,
			ptDbg && sqrt(GAPCALC(*ptDbg-ptDbg1))>=NCMIN ? "X" : " ");
		ptDbg = ptDbg2;
	}
#endif

	return pos1;
}

INT_PTR CDXFchain::GetObjectCount(void) const
{
	return GetCount();
}

float CDXFchain::GetLength(void) const
{
	float	dLength = 0;
	PLIST_FOREACH(CDXFdata* pData, this)
		dLength += pData->GetLength();
	END_FOREACH
	return dLength;
}

void CDXFchain::AllChainObject_ClearSearchFlg(void)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->ClearSearchFlg();
	END_FOREACH
}

float CDXFchain::GetSelectObjectFromShape
	(const CPointF& pt, const CRectF* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	CRectF	rc, _rcView;
	float	dGap, dGapMin = FLT_MAX;

	if ( pDataResult )
		*pDataResult = NULL;
	if ( rcView )
		_rcView.SetRect(rcView);

	// �w����W�Ɉ�ԋ߂���޼ު�Ă�����
	PLIST_FOREACH(CDXFdata* pData, this)
		// DXFView����̌Ăяo���� pData �� NULL �̉\���A��
		if ( !pData )
			continue;
		if ( rcView && !rc.CrossRect(_rcView, pData->GetMaxRect()) )
			continue;
		dGap = pData->GetSelectPointGap(pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			if ( pDataResult )
				*pDataResult = pData;
		}
	END_FOREACH

	return dGapMin;
}

void CDXFchain::SetVectorPoint(POSITION pos1, CVPointF& vpt, float k)
{
	POSITION	pos2 = pos1;
	CDXFdata*	pData;

	do {
		pData = GetSeqData(pos1);
		if ( !pos1 )
			pos1 = GetFirstPosition();
		pData->SetVectorPoint(vpt, k);
	} while ( pos1 != pos2 );
}

void CDXFchain::SetVectorPoint(POSITION pos1, CVPointF& vpt, size_t n)
{
	POSITION	pos2 = pos1;
	CDXFdata*	pData;
	size_t		nCnt, nTotal = 0;
	vector<size_t>	vCnt;
	vector<size_t>::iterator	it;

	float		L = GetLength();	// ������

	// �e��޼ު�Ă̒������番�����銄�����v�Z
	do {
		pData = GetSeqData(pos1);
		if ( !pos1 )
			pos1 = GetFirstPosition();
		nCnt = (size_t)(n * pData->GetLength() / L + 0.5);	// �l�̌ܓ�
		nTotal += nCnt;
		vCnt.push_back(nCnt);
	} while ( pos1 != pos2 );

	// �l�̌ܓ�����̂ŉߕs���𒲐�
	if ( nTotal > n ) {
		do {
			it = max_element(vCnt.begin(), vCnt.end());
			(*it)--;
			nTotal--;
		} while ( nTotal > n );
	}
	else if ( nTotal < n ) {
		do {
			it = min_element(vCnt.begin(), vCnt.end());
			(*it)++;
			nTotal++;
		} while ( nTotal < n );
	}

	// ������̕������Ŋe��޼ު�Ă𕪊�
	it = vCnt.begin();
	do {
		pData = GetSeqData(pos1);
		if ( !pos1 )
			pos1 = GetFirstPosition();
		pData->SetVectorPoint(vpt, *it++);
	} while ( pos1 != pos2 );
}

void CDXFchain::SetShapeSwitch(BOOL bSelect)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->SetDxfFlg(DXFFLG_SELECT, bSelect);
	END_FOREACH
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
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	PLIST_FOREACH(CDXFdata* pData, this)
		dwSel = pData->GetDxfFlg() & DXFFLG_SELECT;
		if ( dwSel != dwSelBak ) {
			dwSelBak = dwSel;
			pDC->SelectObject(pData->GetDrawPen());
		}
		pData->Draw(pDC);
	END_FOREACH
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

void CDXFchain::OrgTuning(void)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->OrgTuning(FALSE);
	END_FOREACH
	// �������g���׸ނ�������
	m_dwFlags &= ~(DXFMAPFLG_MAKE|DXFMAPFLG_SEARCH);
}

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̌`��W���N���X
/////////////////////////////////////////////////////////////////////////////
CDXFshape::CDXFshape()
{
	Constructor(DXFSHAPE_OUTLINE, NULL, 0, NULL);
}

CDXFshape::CDXFshape(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CLayerData* pLayer, CDXFchain* pChain)
{
	m_vShape = pChain;
	Constructor(enAssemble, lpszShape, dwFlags, pLayer);
	// ���Wϯ�߂̵�޼ު�čő��`�Ə���ϯ�߂̍X�V
	SetDetailInfo(pChain);
}

CDXFshape::CDXFshape(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CLayerData* pLayer, CDXFmap* pMap)
{
	m_vShape = pMap;
	Constructor(enAssemble, lpszShape, dwFlags, pLayer);
	// ���Wϯ�߂̵�޼ު�čő��`�Ə���ϯ�߂̍X�V
	SetDetailInfo(pMap);
}

void CDXFshape::Constructor(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CLayerData* pLayer)
{
	m_enAssemble	= enAssemble;
	m_pParentLayer	= pLayer;
	m_dwFlags	= dwFlags;
	m_dOffset	= 1.0f;		// ��̫�ĵ̾�Ēl
	m_nInOut	= -1;		// ��̫�ė֊s��������
	m_bAcute	= TRUE;
	m_hTree		= NULL;
	m_nSerialSeq= 0;
	if ( lpszShape && lstrlen(lpszShape) > 0 )
		m_strShapeHandle = m_strShape = lpszShape;
	// ��޼ު�ċ�`�̏�����
	m_rcMax.SetRectMinimum();
}

void CDXFshape::SetDetailInfo(CDXFchain* pChain)
{
	PLIST_FOREACH(CDXFdata* pData, pChain)
		m_rcMax |= pData->GetMaxRect();
		pData->SetParentMap(this);
	END_FOREACH
}

void CDXFshape::SetDetailInfo(CDXFmap* pMap)
{
	CDXFarray*	pArray;
	CDXFdata*	pData;
	CPointF	pt;
	int		i;
	PMAP_FOREACH(pt, pArray, pMap)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			m_rcMax |= pData->GetMaxRect();
			pData->SetParentMap(this);
		}
	END_FOREACH
}

CDXFshape::~CDXFshape()
{
	PLIST_FOREACH(auto ref, &m_ltWork)
		delete	ref;
	END_FOREACH
	PLIST_FOREACH(auto ref, &m_ltOutline)
		delete	ref;
	END_FOREACH
	m_ltWork.RemoveAll();
	m_ltOutline.RemoveAll();
	CrearScanLine_Lathe();		// Clear m_obLathe

	// m_vShape���̵�޼ު�Ă�CDXFshape�ō��ꂽ�ް��ł͂Ȃ��̂�
	// ������delete���Ă͂����Ȃ�
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
		ar << (m_dwFlags & ~DXFMAPFLG_SELECT) << nAssemble <<	// �I����ԏ���
			m_strShape << m_strShapeHandle <<
			m_dOffset << m_nInOut << m_bAcute <<
			nType;
		if ( nType == 0 )
			get<CDXFchain*>(m_vShape)->Serialize(ar);
		else
			get<CDXFmap*>(m_vShape)->Serialize(ar);
	}
	else {
		m_pParentLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
		ar >> m_dwFlags >> nAssemble >> m_strShape;
		if ( g_dwCamVer > NCVCSERIALVERSION_1507 )	// Ver1.60�`
			ar >> m_strShapeHandle;
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			double	d;
			ar >> d;
			m_dOffset = (float)d;
		}
		else {
			ar >> m_dOffset;
		}
		ar >> m_nInOut;
		if ( g_dwCamVer > NCVCSERIALVERSION_1503 )	// Ver1.10�`
			ar >> m_bAcute;
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
		m_pParentLayer->SetActiveShape(this);
	}
	// ���H�w�����̼رײ��
	m_ltWork.Serialize(ar);
	if ( ar.IsStoring() )
		m_ltOutline.Serialize(ar);
	else {
		if ( g_dwCamVer > NCVCSERIALVERSION_1600 )		// Ver1.70�`
			m_ltOutline.Serialize(ar);
		else {
			// m_ltWork �ɂ��� CDXFworkingOutline �� m_ltOutline ��
			CDXFworkingOutline*	pWork;
			POSITION		pos1, pos2;
			for ( pos1=m_ltWork.GetHeadPosition(); (pos2=pos1); ) {
				pWork = static_cast<CDXFworkingOutline*>(m_ltWork.GetNext(pos1));
				if ( pWork->GetWorkingType() == WORK_OUTLINE ) {
					m_ltOutline.AddTail(pWork);
					m_ltWork.RemoveAt(pos2);
				}
			}
		}
	}
}

BOOL CDXFshape::AddWorkingData(CDXFworking* pWork)
{
	// ��O�͏�ʂōs��
	m_ltWork.AddTail(pWork);
	// ϯ���׸ނ����ޯĵ�޼ު�Ă��׸ނ�ݒ�
	ENWORKINGTYPE nType = pWork->GetWorkingType();
	m_dwFlags |= g_dwMapFlag[nType];
	CDXFdata*	pData = pWork->GetTargetObject();
	if ( pData && nType==WORK_DIRECTION )
		pData->SetDxfFlg(DXFFLG_DIRECTIONFIX);
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
					pData->SetDxfFlg(DXFFLG_DIRECTIONFIX, FALSE);
				delete	pWork;
			}
			bResult = TRUE;
			break;
		}
	}

	return bResult;
}

BOOL CDXFshape::AddOutlineData(CDXFworkingOutline* pOutline, int nInOut)
{
	m_ltOutline.AddTail(pOutline);
	m_dwFlags |= DXFMAPFLG_OUTLINE;
	m_nInOut   = nInOut;
	return TRUE;
}

BOOL CDXFshape::DelOutlineData(CDXFworkingOutline* pWork/*=NULL*/)
{
	POSITION	pos1, pos2;
	CDXFworkingOutline*	pOutline;
	for ( pos1=m_ltOutline.GetHeadPosition(); (pos2=pos1); ) {
		pOutline = m_ltOutline.GetNext(pos1);
		if ( !pWork ) {
			m_ltOutline.RemoveAt(pos2);
			delete	pOutline;
		}
		else if ( pWork && pWork==pOutline ) {
			m_ltOutline.RemoveAt(pos2);
			delete	pOutline;
			break;
		}
	}
	if ( m_ltOutline.IsEmpty() )
		m_dwFlags &= ~DXFMAPFLG_OUTLINE;

	return TRUE;
}

tuple<CDXFworking*, CDXFdata*> CDXFshape::GetDirectionObject(void) const
{
	CDXFworking*	pWork;
	CDXFdata*		pData = NULL;

	PLIST_FOREACH(pWork, &m_ltWork)
		if ( pWork->GetWorkingType() == WORK_DIRECTION ) {
			pData = pWork->GetTargetObject();
			break;
		}
	END_FOREACH

	return make_tuple(pWork, pData);
}

tuple<CDXFworking*, CDXFdata*> CDXFshape::GetStartObject(void) const
{
	CDXFworking*	pWork;
	CDXFdata*		pData = NULL;

	PLIST_FOREACH(pWork, &m_ltWork)
		if ( pWork->GetWorkingType() == WORK_START ) {
			pData = pWork->GetTargetObject();
			break;
		}
	END_FOREACH

	return make_tuple(pWork, pData);
}

POSITION CDXFshape::GetFirstChainPosition(void)
{
	float			dGap1, dGap2;
	const CPointF	ptOrg( CDXFdata::ms_ptOrg );
	CPointF			ptNow;
	CDXFworking*	pWork;
	CDXFdata*		pData;
	CDXFdata*		pDataFix;
	CDXFchain*		pChain = GetShapeChain();

	m_dwFlags &= ~(DXFMAPFLG_REVERSE|DXFMAPFLG_NEXTPOS);

	// �J�n�ʒu�w��
	tie(pWork, pData) = GetStartObject();
	if ( pData ) {
		// �擪��޼ު�Ăƌ��݈ʒu���X�V
		pDataFix = pData;
		ptNow = static_cast<CDXFworkingStart*>(pWork)->GetStartPoint() - ptOrg;	// ���_�������˂�
	}
	else {
		ptNow = CDXFdata::ms_pData->GetEndCutterPoint();
		// �J�n��޼ު�Ă̌��� (pData�ɋߐڵ�޼ު�Ă��)
		GetSelectObjectFromShape(ptNow+ptOrg, NULL, &pDataFix);
	}
	ASSERT(pDataFix);

	// �����w������ѐ�����������
	tie(pWork, pData) = GetDirectionObject();
	if ( pChain->GetCount()==1 && pDataFix->IsStartEqEnd() ) {
		if ( pData ) {
			CPointF	pts( static_cast<CDXFworkingDirection*>(pWork)->GetStartPoint() ),
					pte( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() );
			// ��]�ݒ�
			pData->IsDirectionPoint(pts, pte);
		}
		pDataFix->GetEdgeGap(ptNow);	// �֊s��޼ު�Ă̋ߐڍ��W�v�Z
	}
	else {
		dGap1 = GAPCALC(pDataFix->GetStartCutterPoint() - ptNow);
		dGap2 = GAPCALC(pDataFix->GetEndCutterPoint()   - ptNow);
		if ( pData ) {
			CPointF	ptFix( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() - ptOrg );
			if ( pData->GetEndCutterPoint().IsMatchPoint(&ptFix) ) {
				// �J�n��޼ު�Ă̏I�_�̕����߂��ꍇ�́A
				// ���̵�޼ު�Ă���J�n
				if ( dGap1 > dGap2 )
					m_dwFlags |= DXFMAPFLG_NEXTPOS;
			}
			else {
				m_dwFlags |= DXFMAPFLG_REVERSE;
				// �J�n��޼ު�Ă̏I�_�� bReverse �Ȃ̂Ŏn�_�Ŕ��f
				if ( dGap1 < dGap2 )
					m_dwFlags |= DXFMAPFLG_NEXTPOS;
			}
		}
		else {
			if ( dGap1 > dGap2 )
				m_dwFlags |= DXFMAPFLG_REVERSE;
		}
	}

	return pChain->SetLoopFunc(pDataFix, m_dwFlags&DXFMAPFLG_REVERSE, m_dwFlags&DXFMAPFLG_NEXTPOS);
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
		CDXFchain* pChain = GetShapeChain();	// �ڍs���delete
		if ( !ChangeCreate_ChainToMap(pChain) )
			return FALSE;
		delete	pChain;
	}

	return TRUE;
}

BOOL CDXFshape::ChangeCreate_MapToChain(CDXFmap* pMap/*=NULL*/)
{
	CDXFchain*	pChain = NULL;
	if ( !pMap )
		pMap = GetShapeMap();

	try {
		// �V���Ȍ`��W���𐶐�
		pChain = new CDXFchain;
		if ( pMap->CopyToChain(pChain) ) {
			// CDXFshape ���X�V
			m_enAssemble = DXFSHAPE_OUTLINE;
			m_vShape = pChain;
			m_dwFlags = 0;
			m_strShape += "(���i)";
		}
		else {
			delete pChain;
			return FALSE;
		}
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

BOOL CDXFshape::ChangeCreate_ChainToMap(CDXFchain* pChain/*=NULL*/)
{
	CDXFmap*	pMap = NULL;
	if ( !pChain )
		pChain = GetShapeChain();

	try {
		// �V���Ȍ`��W���𐶐�
		pMap = new CDXFmap;
		pChain->CopyToMap(pMap);
		// CDXFshape ���X�V
		m_enAssemble = DXFSHAPE_LOCUS;
		m_vShape = pMap;
		m_dwFlags = pMap->GetMapTypeFlag();
		m_strShape += "(�~�i)";
		// �K���Ȃ����H�w��(�֊s�w���Ȃ�)���폜
		DelOutlineData();
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
	CPointF		pt;
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
			PLIST_FOREACH(CDXFdata* pData, pChain)
				pt = pData->GetNativePoint(1);
				if ( pMapTmp->PLookup(pt) ) {
					// �P�ł�������Ό�������
					bResult = TRUE;
					pMapTmp->Append(pChain);
					break;
				}
			END_FOREACH
		}
		else {
			pMap = pShape->GetShapeMap();
			CDXFarray*	pDummy;
			PMAP_FOREACH(pt, pDummy, pMap)
				if ( pMapTmp->PLookup(pt) ) {
					bResult = TRUE;
					pMapTmp->Append(pMap);
					break;
				}
			END_FOREACH
		}
		if ( !bResult ) {
			delete	pMapTmp;
			return FALSE;
		}
		// �s�v���H�w��(�֊s�w���Ȃ�)���폜
		DelOutlineData();
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
		// �J�n�ʒu�̓���
		tie(pWork1, pData1) = GetStartObject();
		tie(pWork2, pData2) = pShape->GetStartObject();
		if ( pData1 ) {
			if ( pData2 ) {
				pShape->DelWorkingData(pWork2);
			}
		}
		else if ( pData2 ) {
			pShape->DelWorkingData(pWork2, this);
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

BOOL CDXFshape::CreateOutlineTempObject(BOOL bLeft, CDXFchain* pResult, float dOffset/*=0.0f*/)
{
#ifdef _DEBUG
	printf("CreateOutlineTempObject() Start\n");
	CPointF		ptDbg1, ptDbg2, ptDbg3;
#endif
	if ( dOffset <= 0.0f )
		dOffset = m_dOffset;	// ��̫�ĵ̾�Ēl

	// ��L��`�̈�̏�����
	pResult->ClearMaxRect();

	const CDXFchain*	pChain = GetShapeChain();
	if ( !pChain || pChain->IsEmpty() )
		return FALSE;

	CTypedPtrArrayEx<CPtrArray, CDXFlist*>	obSepArray;
	obSepArray.SetSize(0, 32);

	int			k = bLeft ? -1 : 1;
	INT_PTR		nLoop;
	BOOL		bResult = TRUE;
	CDXFdata*	pData;
	CPointF		pt, pte;
	optional<CPointF>	ptResult, pts;
	POSITION	pos;

	// �B��̵�޼ު�Ă���ٰ�߂Ȃ�
	if ( pChain->GetCount() == 1 ) {
		pData = pChain->GetHead();
		CDXFcircle*		pCircle;
		CDXFellipse*	pEllipse;
		CDXFpolyline*	pPolyline;
		switch ( pData->GetType() ) {
		case DXFCIRCLEDATA:
			pCircle = static_cast<CDXFcircle*>(pData);
			// �̾�Ĕ��a��ϲŽ����
			pData = pCircle->GetR()+dOffset*k > NCMIN ?
				::CreateDxfOffsetObject(pData, pte, pte, k, dOffset) :	// pte is dummy
				NULL;
			break;
		case DXFELLIPSEDATA:
			pEllipse = static_cast<CDXFellipse*>(pData);
			if ( pEllipse->IsArc() )
				return FALSE;
			pData = (pEllipse->GetLongLength() +dOffset*k > NCMIN &&
						pEllipse->GetShortLength()+dOffset*k > NCMIN) ?
				::CreateDxfOffsetObject(pData, pte, pte, k, dOffset) :
				NULL;
			break;
		case DXFPOLYDATA:
			pPolyline = static_cast<CDXFpolyline*>(pData);
			if ( !pPolyline->IsStartEqEnd() || pPolyline->IsIntersection() )
				return FALSE;
			bResult = CreateOutlineTempObject_polyline(pPolyline, bLeft, dOffset, pResult, obSepArray);
			nLoop = obSepArray.GetSize();
			if ( bResult ) {
				CheckSeparateChain(pResult, dOffset);
				for ( k=0; k<nLoop; k++ ) {
					CheckSeparateChain(obSepArray[k], dOffset);
					if ( !obSepArray[k]->IsEmpty() ) {
						if ( !pResult->IsEmpty() )
							pResult->AddTail((CDXFdata *)NULL);
						pResult->AddTail(obSepArray[k]);
					}
					delete	obSepArray[k];
				}
			}
			else {
				for ( k=0; k<nLoop; k++ ) {
					PLIST_FOREACH(pData, obSepArray[k])
						delete	pData;
					END_FOREACH
					delete	obSepArray[k];
				}
			}
			return bResult;
		default:
			return FALSE;	// �~�E�ȉ~�E���ײ݂̕�ٰ�߈ȊO�ʹװ
		}
		if ( pData ) {
			pResult->AddTail(pData);
			pResult->SetMaxRect(pData);
		}
		return TRUE;
	}

	// �֊s��޼ު��ٰ�ߏ���
	pos = pChain->GetHeadPosition();
	CDXFdata*	pData1 = pChain->GetNext(pos);	// �ŏ��̵�޼ު��
	CDXFdata*	pData2;

	// CDXFchain�́C�n�_�I�_�̒������s���Ă���̂�
	// �P��ٰ�߂ŗǂ�
	while ( pos ) {		// �Q��ڈȍ~����ٰ��
		pData2 = pChain->GetNext(pos);
#ifdef _DEBUG
		ptDbg1 = pData1->GetNativePoint(0);
		ptDbg2 = pData1->GetNativePoint(1);
		ptDbg3 = pData2->GetNativePoint(1);
		printf("p1=(%.3f, %.3f) p2=(%.3f, %.3f) p3=(%.3f, %.3f)\n",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y, ptDbg3.x, ptDbg3.y);
#endif
		// �̾�č��W�v�Z
		ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dOffset, bLeft);
		if ( !ptResult ) {
			bResult = FALSE;
			break;
		}
		pt = *ptResult;
#ifdef _DEBUG
		printf("Offset=(%.3f, %.3f)\n", pt.x, pt.y);
#endif
		// �֊s��޼ު�Đ���(����͖���)
		if ( pts ) {
			pData = ::CreateDxfOffsetObject(pData1, *pts, pt, k, dOffset);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				// �̾�ĵ�޼ު�Ă̌�_����
				if ( !SeparateOutlineIntersection(pResult, obSepArray) ) {
					bResult = FALSE;
					break;
				}
			}
		}
		else
			pte = pt;		// �ŏ��̗֊s��޼ު�Ă̏I�_
		pts = pt;
		pData1 = pData2;
	}

	if ( bResult ) {
		// �c��̗֊s��޼ު�Đ���(�Ō�̵�޼ު�ĂƐ擪�̵�޼ު��)
		pData2 = pChain->GetHead();
#ifdef _DEBUG
		ptDbg1 = pData1->GetNativePoint(0);
		ptDbg2 = pData1->GetNativePoint(1);
		ptDbg3 = pData2->GetNativePoint(1);
		printf("p1=(%.3f, %.3f) p2=(%.3f, %.3f) p3=(%.3f, %.3f)\n",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y, ptDbg3.x, ptDbg3.y);
#endif
		ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dOffset, bLeft);
		if ( ptResult ) {
			pt = *ptResult;
#ifdef _DEBUG
			printf("Offset=(%.3f, %.3f)\n", pt.x, pt.y);
#endif
			pData = ::CreateDxfOffsetObject(pData1, *pts, pt, k, dOffset);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				if ( !SeparateOutlineIntersection(pResult, obSepArray) )
					bResult = FALSE;
			}
			// �ŏ��̵�޼ު��(�̏I�_��)
			if ( bResult ) {
				pData = ::CreateDxfOffsetObject(pData2, pt, pte, k, dOffset);
				if ( pData ) {
					pResult->AddTail(pData);
					pResult->SetMaxRect(pData);
					if ( !SeparateOutlineIntersection(pResult, obSepArray, TRUE) )
						bResult = FALSE;
				}
			}
		}
		else
			bResult = FALSE;
	}

	nLoop = obSepArray.GetSize();
	if ( !bResult ) {
		// �װض���
		for ( k=0; k<nLoop; k++ ) {
			PLIST_FOREACH(pData, obSepArray[k])
				delete	pData;
			END_FOREACH
			delete	obSepArray[k];
		}
		return FALSE;
	}

#ifdef _DEBUG
	printf("Result member = %d\n", pResult->GetCount());
#endif
	// �{�W���̌���
	CheckSeparateChain(pResult, dOffset);
#ifdef _DEBUG
	printf("CheckSeparateChain() -> Result member = %d\n", pResult->GetCount());
	printf("obSepArray.GetSize()=%d\n", nLoop);
#endif

	// �����W���̌���
	for ( k=0; k<nLoop; k++ ) {
#ifdef _DEBUG
		printf("Separate member = %d\n", obSepArray[k]->GetCount()); 
#endif
		CheckSeparateChain(obSepArray[k], dOffset);
#ifdef _DEBUG
		printf("CheckSeparateChain() -> Separate member = %d\n", obSepArray[k]->GetCount()); 
#endif
		// �����W���𖖔��Ɍ���
		if ( !obSepArray[k]->IsEmpty() ) {
			if ( !pResult->IsEmpty() )
				pResult->AddTail((CDXFdata *)NULL);		// ����ϰ�
			pResult->AddTail(obSepArray[k]);
		}
		delete	obSepArray[k];
	}

	return !pResult->IsEmpty();
}

BOOL CDXFshape::CreateOutlineTempObject_polyline
	(const CDXFpolyline* pPolyline, BOOL bLeft, float dOffset,
		CDXFchain* pResult, CTypedPtrArrayEx<CPtrArray, CDXFlist*>& obSepArray)
{
	int		k = bLeft ? -1 : 1;
	BOOL	bResult = TRUE;
	CDXFdata*	pData;
	CDXFdata*	pData1 = NULL;
	CDXFdata*	pData2 = NULL;
	CDXFdata*	pDataFirst = NULL;
	CDXFarray	obTemp;		// �ꎞ��޼ު�Ċi�[
	optional<CPointF>	pt1, pr1, ptResult;
	CPointF				pt2, pte, pt;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = pPolyline->GetParentLayer();
	obTemp.SetSize(0, pPolyline->GetVertexCount());

	for ( POSITION pos=pPolyline->GetFirstVertex(); pos; ) {
		pData = pPolyline->GetNextVertex(pos);
		if ( pData->GetType() == DXFPOINTDATA ) {
			pt2 = pData->GetNativePoint(0);
			if ( pt1 ) {
				dxfLine.s = *pt1;
				dxfLine.e = pt2;
				pData2 = new CDXFline(&dxfLine);
				obTemp.Add(pData2);
			}
			else
				pData2 = NULL;
			pt1 = pt2;
		}
		else {
			pData2 = pData;
			pt1.reset();
		}
		//
		if ( pData1 && pData2 ) {
			ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dOffset, bLeft);
			if ( !ptResult ) {
				bResult = FALSE;
				break;
			}
			pt = *ptResult;
			if ( pr1 ) {
				pData = ::CreateDxfOffsetObject(pData1, *pr1, pt, k, dOffset);
				if ( pData ) {
					pResult->AddTail(pData);
					pResult->SetMaxRect(pData);
					if ( !SeparateOutlineIntersection(pResult, obSepArray) ) {
						bResult = FALSE;
						break;
					}
				}
			}
			else
				pte = pt;
			pr1 = pt;
		}
		if ( pData2 )
			pData1 = pData2;
		if ( !pDataFirst && pData1 )
			pDataFirst = pData1;
	}

	if ( bResult && pDataFirst ) {
		ptResult = pData1->CalcOffsetIntersectionPoint(pDataFirst, dOffset, bLeft);
		if ( ptResult ) {
			pt = *ptResult;
			pData = ::CreateDxfOffsetObject(pData1, *pr1, pt, k, dOffset);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				if ( !SeparateOutlineIntersection(pResult, obSepArray) )
					bResult = FALSE;
			}
			if ( bResult ) {
				pData = ::CreateDxfOffsetObject(pDataFirst, pt, pte, k, dOffset);
				if ( pData ) {
					pResult->AddTail(pData);
					pResult->SetMaxRect(pData);
					if ( !SeparateOutlineIntersection(pResult, obSepArray, TRUE) )
						bResult = FALSE;
				}
			}
		}
		else
			bResult = FALSE;
	}

	for ( k=0; k<obTemp.GetSize(); k++ )
		delete	obTemp[k];

	return bResult;
}

BOOL CDXFshape::CreateScanLine_X(CDXFchain* pResult)
{
	CDXFdata*	pData;
	DXFLARGV	dxfLine, dxfLine2;
	INT_PTR		nLoop1 = m_ltOutline.GetCount(), nLoop2;
	if ( nLoop1 == 0 )
		nLoop1 = nLoop2 = 1;
	else
		nLoop2 = nLoop1 + 1;		// �O�������ɑ��݂���

	dxfLine.pLayer = dxfLine2.pLayer = NULL;
	dxfLine.s = dxfLine.e = m_rcMax.TopLeft();
	dxfLine.s.x += m_dOffset*nLoop1;
	dxfLine.s.y += m_dOffset*nLoop2;
	dxfLine.e.x = m_rcMax.right - m_dOffset*nLoop1;
	dxfLine.e.y = dxfLine.s.y;
	float	ptEnd = ::RoundUp(m_rcMax.bottom - m_dOffset*nLoop1);

	while ( TRUE ) {
		pData = new CDXFline(&dxfLine);
		pResult->AddTail(pData);
		pResult->SetMaxRect(pData);
		dxfLine2 = dxfLine;
		dxfLine.s.y += m_dOffset;
		if ( ::RoundUp(dxfLine.s.y) <= ptEnd ) {
			dxfLine2.s = dxfLine.e;
			dxfLine2.e.x = dxfLine.e.x;
			dxfLine2.e.y = dxfLine.s.y;
			pData = new CDXFline(&dxfLine2);
			pResult->AddTail(pData);
			dxfLine.e.y = dxfLine.s.y;
			swap(dxfLine.s, dxfLine.e);
		}
		else
			break;	// ٰ�ߏI������
	}

	return TRUE;
}

BOOL CDXFshape::CreateScanLine_Y(CDXFchain* pResult)
{
	CDXFdata*	pData;
	DXFLARGV	dxfLine, dxfLine2;
	INT_PTR		nLoop1 = m_ltOutline.GetCount(), nLoop2;
	if ( nLoop1 == 0 )
		nLoop1 = nLoop2 = 1;
	else
		nLoop2 = nLoop1 + 1;

	dxfLine.pLayer = dxfLine2.pLayer = NULL;
	dxfLine.s = dxfLine.e = m_rcMax.TopLeft();
	dxfLine.s.x += m_dOffset*nLoop2;
	dxfLine.s.y += m_dOffset*nLoop1;
	dxfLine.e.x = dxfLine.s.x;
	dxfLine.e.y = m_rcMax.bottom - m_dOffset*nLoop1;
	float	ptEnd = ::RoundUp(m_rcMax.right - m_dOffset*nLoop1);

	while ( TRUE ) {
		pData = new CDXFline(&dxfLine);
		pResult->AddTail(pData);
		pResult->SetMaxRect(pData);
		dxfLine.s.x += m_dOffset;
		if ( ::RoundUp(dxfLine.s.x) <= ptEnd ) {
			dxfLine2.s = dxfLine.e;
			dxfLine2.e.x = dxfLine.s.x;
			dxfLine2.e.y = dxfLine.e.y;
			pData = new CDXFline(&dxfLine2);
			pResult->AddTail(pData);
			dxfLine.e.x = dxfLine.s.x;
			swap(dxfLine.s, dxfLine.e);
		}
		else
			break;	// ٰ�ߏI������
	}

	return TRUE;
}

BOOL CDXFshape::CreateScanLine_Outline(CDXFchain* pResult)
{
	INT_PTR		n = m_ltOutline.GetCount() + 1;
	CDXFchain	ltOutline;

	while ( CreateOutlineTempObject(m_nInOut, &ltOutline, m_dOffset*n++) ) {
		if ( ltOutline.IsEmpty() )
			break;
		pResult->AddTail(&ltOutline);
		pResult->AddTail((CDXFdata *)NULL);
		ltOutline.RemoveAll();
	}

	return TRUE;
}

BOOL CDXFshape::CreateScanLine_ScrollCircle(CDXFchain* pResult)
{
	DXFAARGV	dxfArc;
	CDXFdata*	pData;
	const CDXFcircle*	pCircleOrg = static_cast<const CDXFcircle *>(GetShapeChain()->GetHead());
	int			nCnt = 1;
	float		r = m_dOffset / 2.0f,
				dMax = pCircleOrg->GetR() - m_dOffset * m_ltOutline.GetCount();
	CPointF		pto(pCircleOrg->GetCenter()),
				pts(pto), pte(pts.x+m_dOffset, pts.y);

	dxfArc.pLayer = NULL;
	dxfArc.r	= r;
	dxfArc.c.x	= pto.x + r;
	dxfArc.c.y	= pto.y;
	dxfArc.sq	= PI;	// RAD(180.0)
	dxfArc.eq	= 0.0f;

	while ( dMax - fabs(pte.x-pto.x) > NCMIN ) {
		pData = new CDXFarc(&dxfArc, FALSE, pts, pte);
		pResult->AddTail(pData);

		dxfArc.r += r;
		pts.x = pte.x;
		if ( nCnt++ & 0x01 ) {
			dxfArc.c.x = pto.x;
			pte.x = dxfArc.c.x - dxfArc.r;
			dxfArc.sq = 0.0f;
			dxfArc.eq = -PI;
		}
		else {
			dxfArc.c.x = pto.x + r;
			pte.x = dxfArc.c.x + dxfArc.r;
			dxfArc.sq = PI;
			dxfArc.eq = 0.0f;
		}
	}

	// �c��̗����𐶐�
	pte.x = pto.x + dMax;				// �O���~�̉E�ڏI�_
	dxfArc.r = (pte.x - pts.x) / 2.0f;	// �I�_����t�Z�������a��
	dxfArc.c.x = pte.x - dxfArc.r;		// ���S
	pData = new CDXFarc(&dxfArc, FALSE, pts, pte);
	pResult->AddTail(pData);

	return TRUE;
}

void CDXFshape::CreateScanLine_Lathe(int n, float d)
{
	CDXFchain*	pChainOrig = GetShapeChain();
	if ( !pChainOrig )
		return;

	CDXFchain*	pChain;
	CDXFdata*	pData;

	while ( n > 0 ) {
		pChain = new CDXFchain;
		PLIST_FOREACH(CDXFdata* pDataOrig, pChainOrig)
			pData = CreateDxfLatheObject(pDataOrig, n*d);
			if ( pData ) {
				pChain->AddTail(pData);
				pChain->SetMaxRect(pData);
			}
		END_FOREACH
		n--;
		m_obLathe.Add(pChain);
	}
}

void CDXFshape::CrearScanLine_Lathe(void)
{
	for ( int i=0; i<m_obLathe.GetSize(); i++ ) {
		CDXFchain* pChain = m_obLathe[i];
		PLIST_FOREACH(CDXFdata* pData, pChain)
			delete	pData;
		END_FOREACH
		delete	pChain;
	}
	m_obLathe.RemoveAll();
}

BOOL CDXFshape::SeparateOutlineIntersection
	(CDXFchain* pOffset, CTypedPtrArrayEx<CPtrArray, CDXFlist*>& obSepList, BOOL bFinish/*=FALSE*/)
{
#ifdef _DEBUG
	printf("SeparateOutlineIntersection() Start\n");
	CPointF		ptDbg1, ptDbg2, ptDbg3;
#endif
	POSITION	pos1 = pOffset->GetTailPosition(), pos2, pos;
	CDXFdata*	pData;
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	CDXFlist*	pSepList = NULL;
	CPointF		pt[4];	// ��_�͍ő�S��
	int			nCnt, nLoop = 0;
	BOOL		bResult = TRUE, bUpdate;

	if ( pos1 )
		pData1 = pOffset->GetPrev(pos1);	// �����Ώ��ް�
	else
		return TRUE;

	for ( ; (pos2=pos1); nLoop++ ) {
		pData2 = pOffset->GetPrev(pos1);		// �����̂ڂ��Č���
		if ( nLoop == 0 ) {
			// �P�ڂ͕K�� pData1 �̎n�_�� pData2 �̏I�_��������
			continue;
		}
		// �̾�ĵ�޼ު�ē��m�̌�_����(�[�_�܂�)
		nCnt = pData1->GetIntersectionPoint(pData2, pt, FALSE);
		ASSERT( nCnt <= 1 );	// ��_�Q�ȏ゠��΂ǂ�����H
		if ( nCnt <= 0 )
			continue;
		else if ( bFinish && !pos1 ) {
			// �Ō��ٰ�߂͕K�� pData1 �̏I�_�� pData2 �̎n�_��������
			continue;
		}
		// --- ��_����I�I
#ifdef _DEBUG
		printf("pData1 type=%d pData2 type=%d\n", pData1->GetType(), pData2->GetType());
		printf("pt=(%.3f, %.3f)\n", pt[0].x, pt[0].y);
#endif
		pSepList = new CDXFlist;
		// 1) ��_���� pData2 �̏I�_�܂ŵ�޼ު�Đ���
		if ( pData2->GetNativePoint(1).IsMatchPoint(&pt[0]) ) {
			// pData2 �̏I�_�ƌ�_���������ꍇ�A
			// ��޼ު�č��Ȃ����A��(next)���ް����� pSepList �ֈړ�
			bUpdate = TRUE;
		}
		else if ( pData2->GetNativePoint(0).IsMatchPoint(&pt[0]) ) {
			// pData2 �̎n�_�ƌ�_���������ꍇ�A
			// ��޼ު�č��Ȃ������ pData2 ���g�� pSepList �ֈړ�
			bUpdate = FALSE;
		}
		else {
			pData = ::CreateDxfOffsetObject(pData2, pt[0], pData2->GetNativePoint(1));
			if ( pData ) {
				pSepList->AddTail(pData);
				// 1-2) pData2 �̏I�_���X�V
				pData2->SetNativePoint(1, pt[0]);
			}
			bUpdate = TRUE;
		}
		// 2) pData2 �܂��� ���̎����� pData1 �̎�O�܂ŕ����W���Ɉړ�
		pos = pos2;
		if ( bUpdate )
			pOffset->GetNext(pos);		// pos���P�i�߂�
		while ( (pos2=pos) ) {
			pData = pOffset->GetNext(pos);
			if ( !pos )		// pData1 == pData
				break;
			pSepList->AddTail(pData);
			pOffset->RemoveAt(pos2);
		}
		// 3) pData1 �̎n�_�����_�܂ŵ�޼ު�Đ���
		if ( pData1->GetNativePoint(1).IsMatchPoint(&pt[0]) ) {
			// pData1�̏I�_�ƌ�_���������ꍇ
			bUpdate = FALSE;
			// pSepList �ֈړ�
			pSepList->AddTail(pData1);
			pOffset->RemoveTail();		// RemoveAt(pos2);
		}
		// pData1�̎n�_�ƌ�_���������ꍇ�͉������Ȃ��ėǂ�
		else if ( !pData1->GetNativePoint(0).IsMatchPoint(&pt[0]) ) {
			pData = ::CreateDxfOffsetObject(pData1, pData1->GetNativePoint(0), pt[0]);
			if ( pData ) {
				pSepList->AddTail(pData);
				// 3-1) pData1 �̎n�_���X�V
				pData1->SetNativePoint(0, pt[0]);
			}
		}
		// 4) �����W���ɒǉ�
		if ( pSepList->IsEmpty() )
			delete	pSepList;
		else
			obSepList.Add(pSepList);
		pSepList = NULL;
	}

	if ( !bResult && pSepList ) {
		// ��Б|��
		PLIST_FOREACH(pData, pSepList)
			delete	pData;
		END_FOREACH
	}

	return TRUE;
}

BOOL CDXFshape::CheckSeparateChain(CDXFlist* pResult, const float dOffset)
{
	int				nCnt = 0;
	POSITION		pos1, pos2, pos;
	CDXFdata*		pData;
	CMapPtrToPtr	mp, mpDel;		// NG�_���߼޼�ݷ�

	// NG�_����(�I�_�Ō���)
	for ( pos1=pResult->GetHeadPosition(); (pos2=pos1); ) {
		pData = pResult->GetNext(pos1);
		if ( CheckIntersectionCircle(pData->GetNativePoint(1), dOffset) )
			mp.SetAt(pos2, pData);		// �P�ł�NG�_�܂�
		else
			nCnt++;						// OK�_����
	}

	// �����W���̍폜�Ώی���
	if ( nCnt <= 2 ) {		// OK�_2�ȉ��౳�
		for ( pos=pResult->GetHeadPosition(); pos; )
			delete	pResult->GetNext(pos);
		pResult->RemoveAll();
		return TRUE;
	}

	if ( mp.IsEmpty() )
		return TRUE;

	// --- NG�_�̏���
	LPVOID		pKey, pVoid;		// dummy

	// �A��NG�_�̵�޼ު�Ă�����
	for ( pos1=mp.GetStartPosition(); pos1; ) {
		mp.GetNextAssoc(pos1, pKey, pVoid);
		pos2 = (POSITION)pKey;
		if ( mpDel.Lookup(pos2, pVoid) )
			continue;	// �폜�����ς�
		pResult->GetNext(pos2);
		if ( !pos2 )
			pos2 = pResult->GetHeadPosition();
		while ( mp.Lookup(pos2, pVoid) ) {
			mpDel.SetAt(pos2, pVoid);	// �����ł� mp.RemoveKey() �ł��Ȃ�
			pos = pos2;
			delete	pResult->GetNext(pos2);
			pResult->RemoveAt(pos);
			if ( !pos2 )
				pos2 = pResult->GetHeadPosition();
		}
	}
	for ( pos=mpDel.GetStartPosition(); pos; ) {	// ��Еt��
		mpDel.GetNextAssoc(pos, pKey, pVoid);
		mp.RemoveKey(pKey);
	}

	// NG�_�̎}����(�I�_��NG�_�����µ�޼ު��)
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	for ( pos=mp.GetStartPosition(); pos; ) {
		mp.GetNextAssoc(pos, pKey, pVoid);
		pos1 = (POSITION)pKey;
		pData1 = pResult->GetNext(pos1);
		if ( !pos1 )
			pos1 = pResult->GetHeadPosition();
		pData2 = pResult->GetNext(pos1);
		// ��޼ު�Ă̏I�_�ύX
		pData1->SetNativePoint(1, pData2->GetNativePoint(1));
		pData2->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);	// �폜ϰ�
	}

	// �ް�����
	for ( pos1=pResult->GetHeadPosition(); (pos=pos1); ) {
		pData = pResult->GetNext(pos1);
		if ( pData->GetDxfFlg() & DXFFLG_OFFSET_EXCLUDE ) {
			pResult->RemoveAt(pos);
			delete	pData;
		}
	}

	return TRUE;
}

BOOL CDXFshape::CheckIntersectionCircle(const CPointF& ptc, float dOffset)
{
	const CDXFchain*	pChain = GetShapeChain();
	ASSERT( pChain );

	// �ؼ��ٵ�޼ު�ĂƂ̌�_����
	PLIST_FOREACH(CDXFdata* pData, pChain)
		// ��_����(==2)�Ȃ� TRUE�A�u�ڂ���v(==1)��OK!
		if ( pData->CheckIntersectionCircle(ptc, dOffset) > 1 )
			return TRUE;
	END_FOREACH

	return FALSE;
}

void CDXFshape::AllChangeFactor(float dFactor) const
{
	PLIST_FOREACH(auto ref, &m_ltWork)
		ref->DrawTuning(dFactor);
	END_FOREACH
	PLIST_FOREACH(auto ref, &m_ltOutline)
		ref->DrawTuning(dFactor);
	END_FOREACH
}

void CDXFshape::DrawWorking(CDC* pDC) const
{
	CDXFworking*		pWork;
	CDXFworkingOutline*	pOutline;
	int		i = 0, j,
			a = 0, b;
	CPen*	pPen[] = {
		AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE),
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL)
	};
	CPen*	pOldPen   = pDC->SelectObject(pPen[i]);
	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	pDC->SetROP2(R2_COPYPEN);
	PLIST_FOREACH(pWork, &m_ltWork)
		j = pWork->GetWorkingFlag() & DXFWORKFLG_SELECT ? 1 : 0;
		if ( i != j ) {
			i = j;
			pDC->SelectObject( pPen[i] );
		}
		b = pWork->GetWorkingType() == WORK_START ? 1 : 0;
		if ( a != b ) {
			a = b;
			if ( a == 0 )
				pDC->SelectStockObject(NULL_BRUSH);
			else
				pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_START));
		}
		pWork->Draw(pDC);
	END_FOREACH
	pDC->SelectStockObject(NULL_BRUSH);
	PLIST_FOREACH(pOutline, &m_ltOutline)
		j = pOutline->GetWorkingFlag() & DXFWORKFLG_SELECT ? 1 : 0;
		if ( i != j ) {
			i = j;
			pDC->SelectObject( pPen[i] );
		}
		pOutline->Draw(pDC);
	END_FOREACH

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

INT_PTR CDXFshape::GetObjectCount(void) const
{
	return apply_visitor( GetObjectCount_Visitor(), m_vShape );
}

float CDXFshape::GetSelectObjectFromShape
	(const CPointF& pt, const CRectF* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
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

/////////////////////////////////////////////////////////////////////////////

CDXFdata* CreateDxfOffsetObject
	(const CDXFdata* pData, const CPointF& pts, const CPointF& pte,
		int k/*=0*/, float dOffset/*=0*/)
{
	CDXFdata*	pDataResult = NULL;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		{
			if ( k!=0 && pts.IsMatchPoint(&pte) )
				break;
			DXFLARGV	dxfLine;
			dxfLine.pLayer = pData->GetParentLayer();
			dxfLine.s = pts;
			dxfLine.e = pte;
			pDataResult = new CDXFline(&dxfLine);
		}
		break;
	case DXFCIRCLEDATA:
		{
			DXFCARGV	dxfCircle;
			const CDXFcircle* pCircle = static_cast<const CDXFcircle*>(pData);
			dxfCircle.pLayer = pCircle->GetParentLayer();
			dxfCircle.c = pCircle->GetCenter();
			dxfCircle.r = pCircle->GetR() + dOffset * k;
			pDataResult = new CDXFcircle(&dxfCircle);
		}
		break;
	case DXFARCDATA:
		{
			DXFAARGV	dxfArc;
			const CDXFarc* pArc = static_cast<const CDXFarc*>(pData);
			BOOL	bRound = pArc->GetRoundOrig(), bCreate = TRUE;
			if ( !bRound && k!=0 )	// ���������
				k = -k;
			dxfArc.pLayer = pArc->GetParentLayer();
			dxfArc.c = pArc->GetCenter();
			dxfArc.r = pArc->GetR() + dOffset * k;
			if ( (dxfArc.sq=dxfArc.c.arctan(pts)) < 0.0f )
				dxfArc.sq += PI2;
			if ( (dxfArc.eq=dxfArc.c.arctan(pte)) < 0.0f )
				dxfArc.eq += PI2;
			// �ؼ��ى~�ʂƵ̾�ĉ~�ʂ̉�]��������
			if ( k != 0 ) {
				optional<CPointF> ptResult = ::CalcIntersectionPoint_LL(
						pts, pArc->GetNativePoint(0),
						pte, pArc->GetNativePoint(1) );
				if ( ptResult )		// �ؼ��ى~�ʂƵ̾�ĉ~�ʂ̉�]�������Ⴄ
					bRound = !bRound;
			}
			float	d;
			if ( bRound ) {
				// for CDXFarc::AngleTuning()
				d = ::RoundUp(DEG(dxfArc.sq));
				while ( d > ::RoundUp(DEG(dxfArc.eq)) )
					dxfArc.eq += PI2;
				// �~���̒���������l�����Ȃ�
				if ( k!=0 && dxfArc.r * ( dxfArc.eq - dxfArc.sq ) < NCMIN )
					bCreate = FALSE;
			}
			else {
				d = ::RoundUp(DEG(dxfArc.eq));
				while ( d > ::RoundUp(DEG(dxfArc.sq)) )
					dxfArc.sq += PI2;
				if ( k!=0 && dxfArc.r * ( dxfArc.sq - dxfArc.eq ) < NCMIN )
					bCreate = FALSE;
			}
			if ( bCreate )
				pDataResult = new CDXFarc(&dxfArc, bRound, pts, pte);
		}
		break;
	case DXFELLIPSEDATA:
		{
			DXFEARGV	dxfEllipse;
			const CDXFellipse* pEllipse = static_cast<const CDXFellipse*>(pData);
			if ( !pEllipse->GetRoundOrig() && k!=0 )	// ���������
				k = -k;
			dxfEllipse.pLayer = pEllipse->GetParentLayer();
			dxfEllipse.c	= pEllipse->GetCenter();
			float	l		= pEllipse->GetLongLength() + dOffset * k,
					lq		= pEllipse->GetLean();
			dxfEllipse.l.x	= l * cos(lq);
			dxfEllipse.l.y	= l * sin(lq);
			dxfEllipse.s	=(pEllipse->GetShortLength() + dOffset * k) / l;
			dxfEllipse.bRound = pEllipse->GetRoundOrig();
			if ( pEllipse->IsArc() ) {
				// �p�x�v�Z�͒����̌X�����l�����Čv�Z
				CPointF	pt1(pts - dxfEllipse.c), pt2(pte - dxfEllipse.c);
				pt1.RoundPoint(-lq);
				pt2.RoundPoint(-lq);
				// �ȉ~�̊p�x�� atan2() �ł͂Ȃ�
				float	q = pt1.x / l;
				if ( q < -1.0f || 1.0f < q )
					q = copysign(1.0f, q);	// -1.0 or 1.0
				dxfEllipse.sq = copysign(acos(q), pt1.y);
				if ( dxfEllipse.sq < 0.0f )
					dxfEllipse.sq += PI2;
				q = pt2.x / l;
				if ( q < -1.0f || 1.0f < q )
					q = copysign(1.0f, q);
				dxfEllipse.eq = copysign(acos(q), pt2.y);
				if ( dxfEllipse.eq < 0.0f )
					dxfEllipse.eq += PI2;
				if ( k != 0 ) {
					optional<CPointF> ptResult = ::CalcIntersectionPoint_LL(
							pts, pEllipse->GetNativePoint(0),
							pte, pEllipse->GetNativePoint(1) );
					if ( ptResult ) {
						dxfEllipse.bRound = !dxfEllipse.bRound;
						swap(dxfEllipse.sq, dxfEllipse.eq);
					}
				}
				if ( dxfEllipse.bRound ) {
					q = ::RoundUp(DEG(dxfEllipse.sq));
					while ( q > ::RoundUp(DEG(dxfEllipse.eq)) )
						dxfEllipse.eq += PI2;
				}
				else {
					q = ::RoundUp(DEG(dxfEllipse.eq));
					while ( q > ::RoundUp(DEG(dxfEllipse.sq)) )
						dxfEllipse.sq += PI2;
				}
			}
			else {
				dxfEllipse.sq = pEllipse->GetStartAngle();
				dxfEllipse.eq = pEllipse->GetEndAngle();
			}
			pDataResult = new CDXFellipse(&dxfEllipse);
		}
		break;
	}

	return pDataResult;
}

CDXFdata*	CreateDxfLatheObject(const CDXFdata* pData, float yd)
{
	CDXFdata*	pDataResult = NULL;

	// �n�_�I�_�⒆�S�_��Y�l��������׽���ĵ�޼ު�Đ���
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		{
			DXFLARGV	dxfLine;
			dxfLine.pLayer = pData->GetParentLayer();
			dxfLine.s = pData->GetNativePoint(0);
			dxfLine.e = pData->GetNativePoint(1);
			dxfLine.s.y += yd;
			dxfLine.e.y += yd;
			pDataResult = new CDXFline(&dxfLine);
		}
		break;
	case DXFARCDATA:
		{
			DXFAARGV	dxfArc;
			const CDXFarc* pArc = static_cast<const CDXFarc*>(pData);
			dxfArc.pLayer = pArc->GetParentLayer();
			dxfArc.c  = pArc->GetCenter();
			dxfArc.r  = pArc->GetR();
			dxfArc.sq = pArc->GetStartAngle();	// RAD��OK
			dxfArc.eq = pArc->GetEndAngle();
			dxfArc.c.y += yd;
			CPointF	pts(pArc->GetNativePoint(0)), pte(pArc->GetNativePoint(1));
			pts.y += yd;
			pte.y += yd;
			pDataResult = new CDXFarc(&dxfArc, pArc->GetRoundOrig(), pts, pte);
		}
		break;
	case DXFELLIPSEDATA:
		{
			DXFEARGV	dxfEllipse;
			const CDXFellipse* pEllipse = static_cast<const CDXFellipse*>(pData);
			dxfEllipse.pLayer = pEllipse->GetParentLayer();
			dxfEllipse.c	= pEllipse->GetCenter();
			dxfEllipse.l	= pEllipse->GetLongPoint();
			dxfEllipse.s	= pEllipse->GetShortMagni();
			dxfEllipse.sq	= DEG(pEllipse->GetStartAngle());
			dxfEllipse.eq	= DEG(pEllipse->GetEndAngle());
			dxfEllipse.bRound = pEllipse->GetRoundOrig();
			dxfEllipse.c.y += yd;
			pDataResult = new CDXFellipse(&dxfEllipse);
		}
		break;
	}
	
	return pDataResult;
}
