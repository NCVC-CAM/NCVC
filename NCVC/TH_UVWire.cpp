// TH_UVWire.cpp
// ���C�����H�@�ɂ�����UV�I�u�W�F�N�g�̐���
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;

#define	IsThread()	pParent->IsThreadContinue()

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp

static	void		InitialVariable(const CNCdata*, LPNCARGV, float);
static	CNCdata*	CreateUVobj(const CNCdata*, LPNCARGV, const CPoint3F&);
static	BOOL		SetArgvCornerRobject(LPNCARGV, CNCblock*, CNCdata*, CNCdata*, float);

//////////////////////////////////////////////////////////////////////

UINT UVWire_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("UVWire_Thread() Start\n");
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CThreadDlg*		pParent = pParam->pParent;
	ASSERT(pDoc);
	ASSERT(pParent);

	INT_PTR		i, nLoopCnt = pDoc->GetNCsize();
	int			nGcode,
				nTaper = 0,	// 0:ð�߷�ݾ� 1: ð�ߎw������ 2:ð�ߏ�����
				nSign1, nSign2,
				nResult = IDOK;
	BOOL		bResult;
	float		dT,			// ܰ�����
				dTaper,		// T�l
				dT1, dT2,	// ð�ߊp�x�w���ɂ��̾�ė�
				z,			// UV����Z�l
				k;			// UV���̺��R�a
	CPointF		pt;
	CPoint3F	ptOffset;
	optional<CPointF>	ptResult;
	DWORD		dwValFlags;
	CNCblock*	pBlock;
	CNCdata*	pData1;
	CNCdata*	pData2;
	CNCdata*	pDataR;
	CNCdata*	pDataFirst = NULL;
	const CNCread*	pRead1;
	const CNCread*	pRead2;

	NCARGV	ncArgv;		// InitialVariable()������

	{
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_UVTAPER_NCD));
		pParent->SetFaseMessage(strMsg);
		// ܰ�����
		CRect3F	rc(pDoc->GetWorkRectOrg());
		z  = rc.low + rc.high;
		dT = fabs(rc.high);		// CNCDoc::DataOperation()
	}
	pParent->m_ctReadProgress.SetRange32(0, (int)nLoopCnt);
#ifdef _DEBUG
	printf("GetNCsize()=%Id Work=%f\n", nLoopCnt, z);
#endif

	// �ϐ��������̂��߂ɁA�ŏ��� G_TYPE ��޼ު�Ă�����
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pDataR = pDoc->GetNCdata(i);
		if ( pDataR->GetGtype() == G_TYPE ) {
			InitialVariable(pDataR, &ncArgv, z);
			break;
		}
	}

try {
	pDataFirst = pDataR = new CNCdata(&ncArgv);

	// --- ���C�����[�v
	while ( i<nLoopCnt && IsThread() ) {

		// �Ώ۵�޼ު�Č���ٰ��
		for ( ; i<nLoopCnt && nTaper==0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
				pParent->m_ctReadProgress.SetPos((int)i);		// ��۸�ڽ�ް
			pData1 = pDoc->GetNCdata(i);
			if ( pData1->GetGtype() != G_TYPE )
				continue;
			nGcode = pData1->GetGcode();
			pRead1 = pData1->GetReadData();
			// ð��Ӱ������
			if ( pRead1->m_pTaper ) {
				// ð�ߊp�x�� TH_NCRead.cpp �������ς�
				if ( pData1->GetType() == NCDLINEDATA ) {
					dTaper = pRead1->m_pTaper->dTaper;	// T�l�ۑ�
					dT1 = dT2 = dT * tan(fabs(dTaper));
					nSign1 = nSign2 = pRead1->m_pTaper->nTaper;
					if ( dTaper < 0 )
						nSign1 = nSign2 = -nSign1;
					nTaper++;	// 0 -> 1 ð��Ӱ�ޏ�����(break)
				}
				else {
					// ð�ߊJ�n��޼ު�Ĵװ
					pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTLINE);
				}
			}
			else {
				dwValFlags = pData1->GetValFlags();
				ptOffset   = pData1->GetOffsetPoint();
				// �P��UV�w������
				ncArgv.nc.nLine  = pData1->GetBlockLineNo();
				ncArgv.nc.nGcode = nGcode;
				ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
				ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y|NCD_Z);
				ncArgv.nc.dValue[NCA_Z] = z;
				if ( dwValFlags & (NCD_X|NCD_Y|NCD_U|NCD_V) ) {
					pt = pData1->GetEndPoint();
					if ( !(dwValFlags & NCD_R) ) {
						// XY���I�_����̕΍�
						if ( dwValFlags & NCD_U )
							pt.x += pData1->GetValue(NCA_U);
						if ( dwValFlags & NCD_V )
							pt.y += pData1->GetValue(NCA_V);
					}
				}
				else {
					// �O���޼ު�Ă̏I�_(1������~�ʕ��)
					pt = pDataR->GetEndPoint();
				}
				ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
				ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
				if ( pData1->GetType() == NCDARCDATA ) {
					pt = static_cast<const CNCcircle *>(pData1)->GetOrg();
					if ( dwValFlags&(NCD_K|NCD_L) && !(dwValFlags&NCD_R) ) {
						// XY�����S����̕΍�
						if ( dwValFlags & NCD_K )
							pt.x += pData1->GetValue(NCA_K);
						if ( dwValFlags & NCD_L )
							pt.y += pData1->GetValue(NCA_L);
					}
					pt -= pDataR->GetEndPoint();
					ncArgv.nc.dValue[NCA_I] = pt.x;
					ncArgv.nc.dValue[NCA_J] = pt.y;
					ncArgv.nc.dwValFlags |= (NCD_I|NCD_J);
				}
				// UV����޼ު�Đ���
				pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
				pData1->SetWireObj(pDataR);
			}
		} // End of search loop
#ifdef _DEBUG
		if ( i < nLoopCnt )
			printf("TaperMode StartBlock=(%d):%s Taper=%f Sign=%d\n",
				pData1->GetBlockLineNo()+1, 
				LPCTSTR(pDoc->GetNCblock(pData1->GetBlockLineNo())->GetStrGcode()),
				DEG(dTaper), nSign1);
#endif

		// ð�ߏ���ٰ��
		for ( ; i<nLoopCnt && nTaper>0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )
				pParent->m_ctReadProgress.SetPos((int)i);
			pData2 = pDoc->GetNCdata(i);
			if ( pData2->GetGtype() != G_TYPE )
				continue;
			pRead2 = pData2->GetReadData();
			//
			if ( pRead2->m_pTaper && dTaper!=pRead2->m_pTaper->dTaper ) {
				dTaper = pRead2->m_pTaper->dTaper;
				dT2 = dT * tan(fabs(dTaper));
				if ( dTaper != 0.0f ) {	// T0 �̎��͕����ێ�
					nSign2 = pRead2->m_pTaper->nTaper;
					if ( dTaper < 0 )
						nSign2 = -nSign2;
				}
			}
			// 
			ncArgv.nc.nLine  = pData1->GetBlockLineNo();
			ncArgv.nc.nGcode = pData1->GetGcode();
			ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
			ncArgv.nc.dwValFlags |= NCD_Z;
			ncArgv.nc.dValue[NCA_Z] = z;
			//
			if ( nTaper == 1 ) {
				// ð�ߏ����J�n����
				ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dT1, nSign1);
				if ( ptResult ) {
					pt = *ptResult;	// pData2 �̎n�_
					ptOffset = pData1->GetOffsetPoint();
					ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
					ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
					ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
					pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
					pData1->SetWireObj(pDataR);
				}
				else {
					// ��{�I�� CalcPerpendicularPoint() �Ŵװ�͂Ȃ����O�̂���
					pBlock = pDoc->GetNCblock(pData2->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
				}
				nTaper++;	// 1 -> 2
			}
			else if ( !pRead2->m_pTaper || (pRead2->m_pTaper && pRead2->m_pTaper->bTonly) ) {
				// �����ް�(pData2)�ŏI����T���ޒP��
				if ( pData2->GetType() == NCDLINEDATA ) {
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dT1, nSign1);
					if ( ptResult ) {
						pt = *ptResult;	// pData1 �̏I�_
						ptOffset = pData1->GetOffsetPoint();
						ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
						ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
						ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
						pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
						pData1->SetWireObj(pDataR);
					}
					else {
						// ��{�I�� CalcPerpendicularPoint() �Ŵװ�͂Ȃ����O�̂���
						pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
						pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
					}
				}
				else {
					// ð��Ӱ�ޏI�����͒������
					pBlock = pDoc->GetNCblock(pData2->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ENDCIRCLE);
				}
				nTaper = pRead2->m_pTaper && pRead2->m_pTaper->bTonly ? 1 : -1;	// �J�n���ォbreak
			}
			else {
				dwValFlags = pData1->GetValFlags();
				if ( dwValFlags&(NCD_U|NCD_V) && !(dwValFlags&NCD_R) ) {
					// ð���޸�َw��
					pt = pData1->GetEndPoint();
					if ( dwValFlags & NCD_U )
						pt.x += pData1->GetValue(NCA_U);
					if ( dwValFlags & NCD_V )
						pt.y += pData1->GetValue(NCA_V);
					ptResult = pt;
				}
				else {
					// ð��Ӱ�ޒ��͌a�␳�Ɠ����v�Z(��S�������ӁF���̕���)
					ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dT1, dT2, nSign2>0);
				}
				if ( ptResult ) {
					pt = *ptResult;
					ptOffset = pData1->GetOffsetPoint();
					ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
					ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
					ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
					if ( pData1->GetType() == NCDARCDATA ) {
						pt = static_cast<const CNCcircle *>(pData1)->GetOrg();
						if ( dwValFlags&(NCD_K|NCD_L) && !(dwValFlags&NCD_R) ) {
							if ( dwValFlags & NCD_K )
								pt.x += pData1->GetValue(NCA_K);
							if ( dwValFlags & NCD_L )
								pt.y += pData1->GetValue(NCA_L);
						}
						pt -= pDataR->GetEndPoint();
						ncArgv.nc.dValue[NCA_I] = pt.x;
						ncArgv.nc.dValue[NCA_J] = pt.y;
						ncArgv.nc.dwValFlags |= (NCD_I|NCD_J);
					}
					pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
					pData1->SetWireObj(pDataR);
				}
				else {
					pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
				}
			}
			// ����ٰ�߃w
			pData1 = pData2;
			pRead1 = pRead2;
			nSign1 = nSign2;
			dT1    = dT2;
		} // End of taper loop

		if ( nTaper < 0 ) {
			// ð��Ӱ�ނ̍ŏI����
			ncArgv.nc.nLine  = pData2->GetBlockLineNo();
			ncArgv.nc.nGcode = pData2->GetGcode();
			ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
			ncArgv.nc.dwValFlags |= (pData2->GetValFlags()&(NCD_X|NCD_Y)) | NCD_Z;
			pt = pData2->GetEndPoint();
			ptOffset = pData2->GetOffsetPoint();
			ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
			ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
			ncArgv.nc.dValue[NCA_Z] = z;
			pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
			pData2->SetWireObj(pDataR);
			nTaper++;	// -1 -> 0
		}

	} // End of Main(while) loop

	// ------------------------------
	// ���R����ٰ��
	//		G60 �Ɨ����R��K�l���w��
	//		G61�`G63 �~�����R
	// ------------------------------
	i = 0;
	pParent->m_ctReadProgress.SetPos((int)i);		// ��۸�ڽ�ް

	while ( i<nLoopCnt && IsThread() ) {

		// �Ώ۵�޼ު�Ă̌���
		for ( nTaper=0; i<nLoopCnt && nTaper==0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )
				pParent->m_ctReadProgress.SetPos((int)i);
			pData1 = pDoc->GetNCdata(i);
			dwValFlags = pData1->GetValFlags();
			if ( pData1->IsCutCode() && dwValFlags&NCD_R )
				nTaper++;				// break
		}
		if ( i >= nLoopCnt )	// nTaper == 0
			break;

		pDataR = NULL;
		pData2 = pDoc->GetNCdata(i);
		pRead1 = pData1->GetReadData();
		pRead2 = pData2->GetReadData();
		dT1 = pRead1->m_pTaper ? dT*tan(fabs(pRead1->m_pTaper->dTaper)) : 0.0f;
		dT2 = pRead2->m_pTaper ? dT*tan(fabs(pRead2->m_pTaper->dTaper)) : 0.0f;
#ifdef _DEBUG
		printf("--- CornerR\n");
		printf("pData1 Line=%d\n", pData1->GetBlockLineNo()+1);
		printf("pData2 Line=%d\n", pData2->GetBlockLineNo()+1);
#endif
		ncArgv.nSpindle		= pData1->GetSpindle();
		ncArgv.dFeed		= pData1->GetFeed();
		ncArgv.dEndmill		= pData1->GetEndmill();
		ncArgv.nEndmillType	= pData1->GetEndmillType();
		ncArgv.bG98			= pData1->GetG98();
		ncArgv.nc.nLine		= pData1->GetBlockLineNo();
		ncArgv.nc.nGtype	= G_TYPE;
		ncArgv.nc.enPlane	= pData1->GetPlane();
//		memcpy(&(ncArgv.g68),   &(pData1->GetReadData()->m_g68),   sizeof(G68ROUND));
//		memcpy(&(ncArgv.taper), &(pData1->GetReadData()->m_taper), sizeof(TAPER));
		if ( pData1->GetReadData()->m_pG68 ) {
			ncArgv.g68.bG68		= TRUE;
			ncArgv.g68.enPlane	= pData1->GetReadData()->m_pG68->enPlane;
			ncArgv.g68.dRound	= pData1->GetReadData()->m_pG68->dRound;
			for ( int ii=0; ii<SIZEOF(ncArgv.g68.dOrg); ii++ )
				ncArgv.g68.dOrg[ii] = pData1->GetReadData()->m_pG68->dOrg[ii];
		}
		else {
			ncArgv.g68.bG68 = FALSE;
		}
		if ( pData1->GetReadData()->m_pTaper ) {
			ncArgv.taper.nTaper	= pData1->GetReadData()->m_pTaper->nTaper;
			ncArgv.taper.dTaper	= pData1->GetReadData()->m_pTaper->dTaper;
			ncArgv.taper.nDiff	= pData1->GetReadData()->m_pTaper->nDiff;
			ncArgv.taper.bTonly	= pData1->GetReadData()->m_pTaper->bTonly;
		}
		else {
			ncArgv.taper.nTaper = 0;
		}
		ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
		ncArgv.nc.dwValFlags |= NCD_Z;
		ncArgv.nc.dValue[NCA_Z] = pData1->GetValue(NCA_Z);

		// --- XY���̺��R ����
		bResult = SetArgvCornerRobject( &ncArgv,
					pDoc->GetNCblock(pData1->GetBlockLineNo()),
					pData1, pData2, dwValFlags&NCD_R ? pData1->GetValue(NCA_R) : 0.0f );
		if ( bResult ) {
			// XY���̺��R��޼ު�Ă�}��
			pDataR = pDoc->DataOperation(pData1, &ncArgv, i, NCINS);
		}
		if ( !pDataR )
			continue;	// XY���ɺ��R��޼ު�Ă��Ȃ���UV���̏����͂��Ȃ�
#ifdef _DEBUG
		printf("XY corner OK\n");
#endif
		i++;			// ��޼ު�Ă�}�������̂ŁA���Ē���
		nLoopCnt++;

		// --- UV���̺��R ����(pDataR�ɑ΂���GetValue(NCA_R)�͎g���Ȃ�)
		if ( pRead1->m_pTaper ) {
			if ( pRead1->m_pTaper->nDiff == 0 ) {
				// G60
				// ð���޸�ق�K�Ƌ�ʂ��邽�߂�R�Ƒg�ݍ��킹���Ƃ������㉺�Ɨ����R
				k = dwValFlags&NCD_R ? pData1->GetValue(dwValFlags&NCD_K ? NCA_K : NCA_R) : 0.0f;
			}
			else {
				k = pDataR->GetType() == NCDARCDATA ?
						static_cast<const CNCcircle *>(pDataR)->GetR() : 0.0f;
				switch ( pRead1->m_pTaper->nDiff ) {
				case 1:		// G61
					k += dT1<dT2 ? dT1 : dT2;
					break;
				case 2:		// G62
					k += ( dT1 + dT2 ) / 2.0f;
					break;
				case 3:		// G63
					k += dT1>dT2 ? dT1 : dT2;
					break;
				default:
					k = 0.0f;
				}
			}
		}

		// UV���̺��R ��޼ު�Đ���
		ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
		ncArgv.nc.dwValFlags |= NCD_Z;
		ncArgv.nc.dValue[NCA_Z] = z;
		//
		bResult = SetArgvCornerRobject(&ncArgv,
					pDoc->GetNCblock(pData1->GetBlockLineNo()),
					pData1->GetWireObj(), pData2->GetWireObj(), k);
		if ( bResult ) {
			pData2 = CreateUVobj(pData1->GetWireObj(), &ncArgv, ptOffset);
			pData1 = pDataR->GetWireObj();
			if ( pData1 )
				delete	pData1;
			pDataR->SetWireObj(pData2);
#ifdef _DEBUG
			printf("UV corner OK\n");
#endif
		}

	} // End of UV Corner Loop

}
catch (CMemoryException* e) {
	AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
	e->Delete();
	nResult = IDCANCEL;
}

	if ( pDataFirst )
		delete	pDataFirst;

	pParent->m_ctReadProgress.SetPos((int)nLoopCnt);
	pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
#ifdef _DEBUG
	printf("PostMessage() Finish!\n");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
// �⏕�֐�

BOOL SetArgvCornerRobject
	(LPNCARGV pArgv, CNCblock* pBlock, CNCdata* pData1, CNCdata* pData2, float k)
{
	if ( !pData1 || !pData2 )
		return FALSE;

	// TH_NCRead.cpp MakeChamferingObject()
	CNCdata*	pDataResult = NULL;
	BOOL		bResult;
	float		r1, r2, pa, pb;
	CPointF		pts, pte, pto;
	CPoint3F	ptOffset(pData1->GetOffsetPoint());
	optional<CPointF>	ptResult;

	// �v�Z�J�n
	// ��ŰR�̏ꍇ�́C�ʎ��ɑ�������C�l�̌v�Z
	tie(bResult, pto, r1, r2) = pData1->CalcRoundPoint(pData2, k);
	if ( !bResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_INTERSECTION);
		return FALSE;
	}
	else if ( r1<NCMIN && r2<NCMIN ) {
		// �␳�̕K�v�Ȃ�
		return FALSE;
	}

	pto -= ptOffset;

	// pData1(�O�̵�޼ު��)�̏I�_��␳
	if ( r1 < NCMIN )
		pts = pData1->GetPlaneValue(pData1->GetEndPoint());
	else {
		ptResult = pData1->SetChamferingPoint(FALSE, r1);
		if ( !ptResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
			return FALSE;
		}
		pts = *ptResult;
	}
	pts -= ptOffset.GetXY();
	// pData2(���̵�޼ު��)�̎n�_��␳
	if ( r2 < NCMIN )
		pte = pData2->GetPlaneValue(pData2->GetStartPoint());
	else {
		ptResult = pData2->SetChamferingPoint(TRUE, r2);
		if ( !ptResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
			return FALSE;
		}
		pte = *ptResult;
	}
	pte -= ptOffset.GetXY();

	// ���W�l�̾��
	pArgv->nc.dValue[NCA_X] = pte.x;
	pArgv->nc.dValue[NCA_Y] = pte.y;
	pArgv->nc.dwValFlags = NCD_X|NCD_Y;
	if ( k < NCMIN ) {
		pArgv->nc.nGcode = 1;
	}
	else {
		// ���߂���ŰR�̒��S(pto)�����]�������v�Z
		pts -= pto;		pte -= pto;
		if ( (pa=pts.arctan()) < 0.0f )
			pa += PI2;
		if ( (pb=pte.arctan()) < 0.0f )
			pb += PI2;
		if ( fabs(pa-pb) > PI ) {
			if ( pa > pb )
				pa -= PI2;
			else
				pb -= PI2;
		}
		pArgv->nc.nGcode = pa > pb ? 2 : 3;
		// IJ�w��
		pArgv->nc.dValue[NCA_I] = -pts.x;
		pArgv->nc.dValue[NCA_J] = -pts.y;
		pArgv->nc.dwValFlags |= (NCD_I|NCD_J);
	}

	return TRUE;
}

CNCdata* CreateUVobj(const CNCdata* pData, LPNCARGV pArgv, const CPoint3F& ptOffset)
{
	CNCdata*	pDataResult = NULL;

	switch ( pArgv->nc.nGcode ) {
	case 0:		// ����
	case 1:
		// ��޼ު�Đ���
		pDataResult = new CNCline(pData, pArgv, ptOffset);
		break;
	case 2:		// �~��
	case 3:
		pDataResult = new CNCcircle(pData, pArgv, ptOffset, NCMAKEWIRE);
		break;
	default:
		pDataResult = new CNCdata(pData, pArgv, ptOffset);
	}

	return pDataResult;
}

void InitialVariable(const CNCdata* pData, LPNCARGV pArgv, float z)
{
	ZeroMemory(pArgv, sizeof(NCARGV));

	pArgv->bAbs			= TRUE;
	pArgv->nc.nGtype	= G_TYPE;
	pArgv->nc.nGcode	= pData->GetGcode();
	pArgv->nc.enPlane	= pData->GetPlane();

	CPoint3F	pts(pData->GetStartPoint());
	pArgv->nc.dValue[NCA_X] = pts.x;
	pArgv->nc.dValue[NCA_Y] = pts.y;
	pArgv->nc.dValue[NCA_Z] = z;
}
