// TH_UVWire.cpp
// ���C�����H�@�ɂ�����UV�I�u�W�F�N�g�̐���
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

#define	IsThread()	pParent->IsThreadContinue()

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp

static	void	InitialVariable(const CNCdata*, LPNCARGV, double);
static	CNCdata*	CreateUVobj(const CNCdata*, LPNCARGV, const CPoint3D&);

//////////////////////////////////////////////////////////////////////

UINT UVWire_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("UVWire_Thread()\nStart", DBG_BLUE);
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CThreadDlg*		pParent = pParam->pParent;
	ASSERT(pDoc);
	ASSERT(pParent);

	int			i, nLoopCnt = pDoc->GetNCsize(), nGcode,
				nTaper = 0,	// 0:ð�߷�ݾ� 1: ð�ߎw������ 2:ð�ߏ�����
				nResult = IDOK;
	double		dZ,			// ܰ�����==UV����Z�l
				dTaper,		// T�l
				dT;			// ð�ߊp�x�w���ɂ��̾�ė�
	CPointD		pt;
	optional<CPointD>	ptResult;
	DWORD		dwValFlags;
	CNCblock*	pBlock;
	CNCdata*	pData1;
	CNCdata*	pData2;
	CNCdata*	pDataFirst = NULL;
	CNCdata*	pData;
	const CNCread*	pRead1;
	const CNCread*	pRead2;

	NCARGV	ncArgv;

	{
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_UVTAPER_NCD));
		pParent->SetFaseMessage(strMsg);
		// ܰ�����
		CRect3D	rc(pDoc->GetWorkRectOrg());
		dZ = rc.high - rc.low;
	}
	pParent->m_ctReadProgress.SetRange32(0, nLoopCnt);
#ifdef _DEBUG
	dbg.printf("GetNCsize()=%d Work=%f", nLoopCnt, dZ);
#endif

	// �ϐ��������̂��߂ɁA�ŏ��� G_TYPE ��޼ު�Ă�����
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pData = pDoc->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE ) {
			InitialVariable(pData, &ncArgv, dZ);
			break;
		}
	}

try {
	pDataFirst = pData = new CNCdata(&ncArgv);

	// --- ���C�����[�v
	while ( i<nLoopCnt && IsThread() ) {

		// �Ώ۵�޼ު�Č���ٰ��
		for ( ; i<nLoopCnt && nTaper==0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
				pParent->m_ctReadProgress.SetPos(i);		// ��۸�ڽ�ް
			pData1 = pDoc->GetNCdata(i);
			if ( pData1->GetGtype() != G_TYPE )
				continue;
			nGcode = pData1->GetGcode();
			pRead1 = pData1->GetReadData();
			// ð��Ӱ������
			if ( pRead1->m_taper.nTaper!=0 && pRead1->m_taper.dTaper!=0.0 ) {
				if ( fabs(pRead1->m_taper.dTaper) <= 45.0 ) {
					if ( pData1->GetType() == NCDLINEDATA ) {
						dTaper = pRead1->m_taper.dTaper;	// T�l�ۑ�
						dT = dZ * tan(dTaper);
						nTaper++;	// 0 -> 1 ð��Ӱ�ޏ�����(break)
					}
					else {
						// ð�ߊJ�n��޼ު�Ĵװ
						pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
						pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTSTART);
					}
				}
				// ð�ߊp�x�װ�� TH_NCRead.cpp �ŏ����ς�
			}
//			else if ( 0<=nGcode && nGcode<=3 ) {
			else {
				// �P��UV�w������
				ncArgv.nc.nLine  = pData1->GetBlockLineNo();
				ncArgv.nc.nGcode = nGcode;
				ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
				ncArgv.nc.dwValFlags |= NCD_Z;
				ncArgv.nc.dValue[NCA_Z] = dZ;
				dwValFlags = pData1->GetValFlags();
				if ( dwValFlags & (NCD_U|NCD_V) ) {
					// UV�׸ނ�XY�׸ނɕϊ�(3bit�E���)
					ncArgv.nc.dwValFlags |= (dwValFlags&(NCD_U|NCD_V)) >> NCXYZ;
					ncArgv.nc.dValue[NCA_X] = pData1->GetValue(NCA_U);
					ncArgv.nc.dValue[NCA_Y] = pData1->GetValue(NCA_V);
				}
				else {
					ncArgv.nc.dwValFlags |= (dwValFlags&(NCD_X|NCD_Y));
					ncArgv.nc.dValue[NCA_X] = pData1->GetValue(NCA_X);
					ncArgv.nc.dValue[NCA_Y] = pData1->GetValue(NCA_Y);
				}
				if ( 2<=nGcode && nGcode<=3 ) {
					// �~�ʕ�ԏ��
					if ( dwValFlags & (NCD_K|NCD_L) ) {
						if ( dwValFlags & NCD_K ) {
							ncArgv.nc.dwValFlags |= NCD_I;
							ncArgv.nc.dValue[NCA_I] = pData1->GetValue(NCA_K);
						}
						if ( dwValFlags & NCD_L ) {
							ncArgv.nc.dwValFlags |= NCD_J;
							ncArgv.nc.dValue[NCA_J] = pData1->GetValue(NCA_L);
						}
					}
					else {
						ncArgv.nc.dwValFlags |= (dwValFlags&(NCD_I|NCD_J));
						ncArgv.nc.dValue[NCA_I] = pData1->GetValue(NCA_I);
						ncArgv.nc.dValue[NCA_J] = pData1->GetValue(NCA_J);
					}
				}
				// UV����޼ު�Đ���
				pData = CreateUVobj(pData, &ncArgv, pData1->GetOffsetPoint());
				pData1->SetWireObj(pData);
			}
		}
#ifdef _DEBUG
		dbg.printf("TaperMode StartBlock=%s Taper=%f",
			pDoc->GetNCblock(pData1->GetBlockLineNo())->GetStrGcode(), DEG(dTaper));
#endif
		// ð�ߏ���ٰ��
		for ( ; i<nLoopCnt && nTaper>0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )
				pParent->m_ctReadProgress.SetPos(i);
			pData2 = pDoc->GetNCdata(i);
			if ( pData2->GetGtype() != G_TYPE )
				continue;
			pRead2 = pData2->GetReadData();
			// 
			ncArgv.nc.nLine  = pData1->GetBlockLineNo();
			ncArgv.nc.nGcode = pData1->GetGcode();
			ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
			ncArgv.nc.dwValFlags |= NCD_Z;
			ncArgv.nc.dValue[NCA_Z] = dZ;
			//
			if ( nTaper == 1 ) {
				// ð�ߏ����J�n����
				ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dT, pRead2->m_taper.nTaper);
				if ( ptResult ) {
					pt = *ptResult;	// pData1 �̏I�_
					ncArgv.nc.dValue[NCA_X] = pt.x;
					ncArgv.nc.dValue[NCA_Y] = pt.y;
				}
				else {
					// ��{�I�� CalcPerpendicularPoint() �Ŵװ�͂Ȃ����O�̂���
					pBlock = pDoc->GetNCblock(pData2->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
				}
				ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
				pData = CreateUVobj(pData, &ncArgv, pData1->GetOffsetPoint());
				pData1->SetWireObj(pData);
				nTaper++;	// 1 -> 2
			}
			else if ( pRead2->m_taper.nTaper == 0 ) {
				if ( pData2->GetType() == NCDLINEDATA ) {
					// �����ް�(pData2)�ŏI��
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dT, pRead1->m_taper.nTaper);
					if ( ptResult ) {
						pt = *ptResult;	// pData1 �̏I�_
						ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
						ncArgv.nc.dValue[NCA_X] = pt.x;
						ncArgv.nc.dValue[NCA_Y] = pt.y;
						pData = CreateUVobj(pData, &ncArgv, pData1->GetOffsetPoint());
						pData1->SetWireObj(pData);
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
				nTaper = -1;		// break
			}
			else {
				// ð��Ӱ�ޒ��͌a�␳�Ɠ����v�Z
				ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dT, pRead2->m_taper.nTaper);
				if ( ptResult ) {
					pt = *ptResult;
					ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
					ncArgv.nc.dValue[NCA_X] = pt.x;
					ncArgv.nc.dValue[NCA_Y] = pt.y;
					pData = CreateUVobj(pData, &ncArgv, pData1->GetOffsetPoint());
					pData1->SetWireObj(pData);
				}
				else {
					pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
				}
			}
			// ����ٰ�߃w
			pData1 = pData2;
			pRead1 = pRead2;
		}
		if ( nTaper < 0 ) {
			// ð��Ӱ�ނ̍ŏI����
			ncArgv.nc.nLine  = pData2->GetBlockLineNo();
			ncArgv.nc.nGcode = pData2->GetGcode();
			ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
			ncArgv.nc.dwValFlags |= (pData1->GetValFlags()&(NCD_X|NCD_Y)) | NCD_Z;
			ncArgv.nc.dValue[NCA_X] = pData2->GetValue(NCA_X);
			ncArgv.nc.dValue[NCA_Y] = pData2->GetValue(NCA_Y);
			ncArgv.nc.dValue[NCA_Z] = dZ;
			pData = CreateUVobj(pData, &ncArgv, pData2->GetOffsetPoint());
			pData2->SetWireObj(pData);
			nTaper++;	// -1 -> 0
		}
	} // End of Main(while) loop

}
catch (CMemoryException* e) {
	AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
	e->Delete();
	nResult = IDCANCEL;
}

	if ( pDataFirst )
		delete	pDataFirst;

	pParent->m_ctReadProgress.SetPos(nLoopCnt);
	pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
#ifdef _DEBUG
	dbg.printf("PostMessage() Finish!");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
// �⏕�֐�

CNCdata* CreateUVobj(const CNCdata* pData, LPNCARGV pArgv, const CPoint3D& ptOffset)
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
		pDataResult = new CNCcircle(pData, pArgv, ptOffset);
		break;
	default:
		pDataResult = new CNCdata(pData, pArgv, ptOffset);
	}

	return pDataResult;
}

void InitialVariable(const CNCdata* pData, LPNCARGV pArgv, double dZ)
{
	ZeroMemory(pArgv, sizeof(NCARGV));

	pArgv->nc.nGtype	= G_TYPE;
	pArgv->nc.nGcode	= pData->GetGcode();
	pArgv->nc.enPlane	= pData->GetPlane();
	pArgv->bAbs			= TRUE;

	CPoint3D	pts(pData->GetStartPoint());
	pArgv->nc.dValue[NCA_X] = pts.x;
	pArgv->nc.dValue[NCA_Y] = pts.y;
	pArgv->nc.dValue[NCA_Z] = dZ;
}
