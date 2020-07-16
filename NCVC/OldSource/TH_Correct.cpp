// TH_Correct.cpp
// �␳�v�Z
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

#define	IsThread()	pParent->IsThreadContinue()

static	CPointD		CalcPerpendicularPoint(const CPointD&, const CPoint3D&, double, int, ENPLANE);
static	CNCdata*	CreateNCobj(const CNCdata*, const CPointD&, double = HUGE_VAL);
static	void		SetErrorCode(CNCDoc*, CNCdata*, int);

//////////////////////////////////////////////////////////////////////
//	�␳���W�̌v�Z�گ��
//////////////////////////////////////////////////////////////////////

UINT CorrectCalc_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CorrectCalc_Thread()\nStart", DBG_BLUE);
#endif
	LPNCVCTHREADPARAM	pParam = (LPNCVCTHREADPARAM)pVoid;
	CNCDoc*			pDoc = (CNCDoc *)(pParam->pDoc);
	CThreadDlg*		pParent = pParam->pParent;
	ASSERT(pDoc);
	ASSERT(pParent);

	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	int			i=0, j, nLoopCnt = pDoc->GetNCsize(),
				nCorrect = 0,	// 0:�ʏ� 1: �␳��ۯ����� 2:�␳������
				nCorrectType = pMCopt->GetCorrectType(),
				nSign1, nSign2,
				k,			// 90����]���������(�n�_���)
				nIO,		// 0:����, 1:�O���s�p, 2:�O���݊p
				nResult = IDOK;
	double		dToolD, dToolD_abs, dAngle;
	optional<double>	dToolResult;
	CPoint3D	ptValS,		// �O���޼ު�Ă̎n�_
				ptValO,		// �O���޼ު�Ă̏I�_(�����޼ު�Ă̎n�_)
				ptValE;		// �����޼ު�Ă̏I�_
	CPointD		pt1, pt2, pt3, pt4;
	optional<CPointD>	ptResult;
	DWORD		dwValFlags;
	CNCdata*	pData1;
	CNCdata*	pData2;
	CNCdata*	pData1c = NULL;
	CNCdata*	pData2c = NULL;
	CNCdata*	pData;
	CNCdata*	pDataResult;
	CTypedPtrArrayEx<CPtrArray, CNCdata*>
				obNdata;	// ���ꕽ�ʂ̈ړ��𔺂�Ȃ����Ԃ̵�޼ު��

	{
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_CORRECT_NCD));
		pParent->SetFaseMessage(strMsg);
	}
	pParent->m_ctReadProgress.SetRange32(0, nLoopCnt);
#ifdef _DEBUG
	dbg.printf("GetNCsize()=%d", nLoopCnt);
#endif

	try {

		while ( i<nLoopCnt && IsThread() ) {

			// �␳�������K�v�ȵ�޼ު�Ă�����(��P���[�v�͂����܂œǂݔ�΂�)
			for ( ; i<nLoopCnt && nCorrect==0 && IsThread(); i++ ) {
				if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
					pParent->m_ctReadProgress.SetPos(i);		// ��۸�ڽ�ް
				pData1 = pDoc->GetNCdata(i);
				dwValFlags = pData1->GetValFlags();
				if ( dwValFlags & NCD_CORRECT ) {
					// �H����̎擾
					if ( dwValFlags & NCD_D ) {
						dToolResult = pMCopt->GetToolD( (int)(pData1->GetValue(NCA_D)) );
						if ( dToolResult )
							dToolD = *dToolResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_CORRECT);
							continue;
						}
					}
					else
						continue;	// �H���񖳂���Γǂݔ�΂�
					//
					dToolD_abs = fabs(dToolD);
					ptValS = pData1->GetStartPoint();
					pData1c = pData1->CopyObject();	// ����
					pData1->AddCorrectObject(pData1c);
					nCorrect = 1;	// break
#ifdef _DEBUG
					dbg.printf("Gcode=%d X=%.3f Y=%.3f Z=%.3f", pData1->GetGcode(),
						pData1->GetEndValue(NCA_X),
						pData1->GetEndValue(NCA_Y),
						pData1->GetEndValue(NCA_Z) );
#endif
				}
			}

			// �␳�J�n��ۯ�������
			if ( pData1->GetType() != NCDLINEDATA ) {
				SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_CORRECTSTART);
				nCorrect = 0;	// �␳ٰ�߂ɓ���Ȃ�
			}

			// �␳ٰ��
			for ( ; i<nLoopCnt && nCorrect>0 && IsThread(); i++ ) {
				if ( (i & 0x003f) == 0 )
					pParent->m_ctReadProgress.SetPos(i);
				pData2 = pDoc->GetNCdata(i);
				dwValFlags = pData1->GetValFlags();
				// �␳�ް�����
				if ( pData2->GetGtype()!=G_TYPE || 
						(pData2->GetType()!=NCDLINEDATA && pData2->GetType()!=NCDARCDATA) ) {
					SetErrorCode(pDoc, pData2, IDS_ERR_NCBLK_GTYPE);
					pData1 = pData2;
					continue;
				}
				if ( pData1->GetPlane() != pData2->GetPlane() ) {
					SetErrorCode(pDoc, pData2, IDS_ERR_NCBLK_PLANE);
					pData1 = pData2;
					continue;
				}
				// �������ʈړ��������ꍇ(�������ꍇ���܂�)�͂��Ƃŏ���
				switch ( pData1->GetPlane() ) {
				case XY_PLANE:
					switch ( pData2->GetType() ) {
					case NCDLINEDATA:
						if ( !(pData2->GetValFlags() & (NCD_X|NCD_Y)) ) {
							obNdata.Add(pData2);
							continue;
						}
						break;
					case NCDARCDATA:
						if ( !(pData2->GetValFlags() & (NCD_R|NCD_I|NCD_J)) ) {
							obNdata.Add(pData2);
							continue;
						}
						break;
					}
					break;
				case XZ_PLANE:
					switch ( pData2->GetType() ) {
					case NCDLINEDATA:
						if ( !(pData2->GetValFlags() & (NCD_X|NCD_Z)) ) {
							obNdata.Add(pData2);
							continue;
						}
						break;
					case NCDARCDATA:
						if ( !(pData2->GetValFlags() & (NCD_R|NCD_I|NCD_K)) ) {
							obNdata.Add(pData2);
							continue;
						}
						break;
					}
					break;
				case YZ_PLANE:
					switch ( pData2->GetType() ) {
					case NCDLINEDATA:
						if ( !(pData2->GetValFlags() & (NCD_Y|NCD_Z)) ) {
							obNdata.Add(pData2);
							continue;
						}
						break;
					case NCDARCDATA:
						if ( !(pData2->GetValFlags() & (NCD_R|NCD_J|NCD_K)) ) {
							obNdata.Add(pData2);
							continue;
						}
						break;
					}
					break;
				}
				if ( pData2->GetType()==NCDLINEDATA &&
						pData1->GetEndPoint()==pData2->GetEndPoint() ) {
					obNdata.Add(pData2);
					continue;
				}
#ifdef _DEBUG
				dbg.printf("Gcode=%d X=%.3f Y=%.3f Z=%.3f", pData2->GetGcode(),
					pData2->GetEndValue(NCA_X),
					pData2->GetEndValue(NCA_Y),
					pData2->GetEndValue(NCA_Z) );
#endif
				//
				ptValO = pData1->GetEndPoint();
				ptValE = pData2->GetEndPoint();
				// �̾�ĕ����v�Z(G41(��)��Ɍv�Z)
				nSign1 = pData1->CalcOffsetSign(ptValS);	// pData1�̎n�_
				nSign2 = pData2->CalcOffsetSign(ptValO);	// pData1�̏I�_=>pData2�̎n�_
				if ( nSign1==0 || nSign2==0 ) {
					SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_GTYPE);
					pData1 = pData2;
					continue;
				}
				if ( dwValFlags & NCD_CORRECT_R ) {
					nSign1 = -nSign1;	// G42�̏ꍇ�͕������]
					nSign2 = -nSign2;
					k = -1;			// -90����]
				}
				else
					k = 1;			// +90����]
				if ( dToolD < 0 ) {
					nSign1 = -nSign1;	// �H��a��ϲŽ�̏ꍇ�͕������]
					nSign2 = -nSign2;
					k = -k;
				}
				// ��޼ު�ĊԂ̊p�x�����߂�(-180���`180��)
				dAngle = pData1->CalcBetweenAngle(ptValS, pData2);
				// �������O��(�s�p�E�݊p)���H(180���ł͓�������)
				if ( fabs(dAngle)*DEG+EPS > 180.0 ) {
					nIO = 0;
				}
				else if ( dAngle > 0 ) {
					if ( dwValFlags & NCD_CORRECT_R )
						nIO = 0;
					else
						nIO = dAngle < 90.0*RAD ? 1 : 2;
				}
				else {
					if ( dwValFlags & NCD_CORRECT_L )
						nIO = 0;
					else
						nIO = dAngle > -90.0*RAD ? 1 : 2;
				}
				// �␳���W�v�Z
				pData2c = pData2->CopyObject();	// ����
				switch ( nIO ) {
				case 0:		// ����
					// �␳���������J�n��ۯ���
					if ( nCorrect > 1 ) {
						// �̾�ĕ��ړ���������_
						ptResult = pData1c->CalcOffsetIntersectionPoint(ptValS, pData2c, dToolD_abs, nSign1, nSign2);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
					}
					else {
						// �J�n��ۯ��ł͎��̵�޼ު�Ă̎n�_�𐂒��ɵ̾��
						ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
					}
					// ��_�v�Z�␂���̾�ē_���V����
					pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);	// pData1�̏I�_
					pData2c->SetCorrectPoint(TRUE,  pt1, dToolD_abs*nSign2);	// pData2�̎n�_
					break;
				case 1:		// �O���s�p
					if ( nCorrect == 1 && nCorrectType == 0 ) {	// �J�n��ۯ� ���� ����A
						// �J�n��ۯ��ł͎��̵�޼ު�Ă̎n�_�𐂒��ɵ̾��
						ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);
						pData2c->SetCorrectPoint(TRUE,  pt1, dToolD_abs*nSign2);
					}
					else {		// �␳�� �܂��� ����B
						// �I�_�𐂒��̾��
						ptResult = pData1c->CalcPerpendicularPoint(ptValO, ptValS, dToolD_abs, FALSE, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// pt1�����_��pData1�̏I�_��90����]
						pt2 = CalcPerpendicularPoint(pt1, ptValO, dToolD_abs, k, pData1c->GetPlane());
						// �ŏI�_��pData2�̎n�_�𐂒��̾��
						ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
						if ( ptResult )
							pt4 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// pt4�����_��pData2�̎n�_��90����]
						pt3 = CalcPerpendicularPoint(pt4, ptValO, dToolD_abs, -k, pData2c->GetPlane());
						// pData1�̏I�_��␳
						pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);
						// ���ꂼ��̓_�ŵ�޼ު�Đ���
						if ( pDataResult = CreateNCobj(pData1c, pt2) ) {	// pt1 -> pt2
							pData1->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
						if ( pDataResult = CreateNCobj(pData, pt3) ) {		// pt2 -> pt3
							pData1->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
						if ( pDataResult = CreateNCobj(pData, pt4) ) {		// pt3 -> pt4
							pData2->AddCorrectObject(pDataResult);	// ���̵�޼ު�Ăɓo�^
							pData = pDataResult;
						}
						// pData2�̎n�_��␳
						pData2c->SetCorrectPoint(TRUE, pt4, dToolD_abs*nSign2);
					}
					break;
				case 2:		// �O���݊p
					if ( nCorrect > 1 ) {
						// �̾�ĕ��ړ���������_(�~�ʂ͐ڐ�)
						ptResult = pData1c->CalcOffsetIntersectionPoint2(ptValS, pData2c, dToolD_abs, nSign1, nSign2);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// �I�_�␳
						if ( pData1c->GetType() == NCDARCDATA ) {
							// �I�_�𐂒��̾��
							ptResult = pData1c->CalcPerpendicularPoint(ptValO, ptValS, dToolD_abs, FALSE, k);
							if ( ptResult )
								pt2 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							// pData1�̏I�_��␳
							pData1c->SetCorrectPoint(FALSE, pt2, dToolD_abs*nSign1);
							// �̾�Č�_�܂ŵ�޼ު�Đ���
							if ( pDataResult = CreateNCobj(pData1c, pt1) ) {
								pData1->AddCorrectObject(pDataResult);
								pData = pDataResult;
							}
						}
						else {
							// pData1�̏I�_��␳
							pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);
							// ���̵�޼ު�Đ����p
							pData = pData1c;
						}
						// �n�_�␳
						if ( pData2c->GetType() == NCDARCDATA ) {
							// �n�_�𐂒��̾��
							ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
							if ( ptResult )
								pt3 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							// �̾�Č�_���炱���܂ŵ�޼ު�Đ���
							if ( pDataResult = CreateNCobj(pData, pt3) ) {
								pData1->AddCorrectObject(pDataResult);
								pData = pDataResult;
							}
							// pData2�̎n�_��␳
							pData2c->SetCorrectPoint(TRUE, pt3, dToolD_abs*nSign1);
						}
						else {
							// pData2�̎n�_��␳
							pData2c->SetCorrectPoint(TRUE, pt1, dToolD_abs*nSign1);
						}
					}
					else {
						if ( nCorrectType == 0 ) {	// ����A
							ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
							if ( ptResult )
								pt1 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);
							pData2c->SetCorrectPoint(TRUE,  pt1, dToolD_abs*nSign2);
						}
						else {						// ����B
							// �̾�ĕ��ړ���������_(�~�ʂ͐ڐ�)
							ptResult = pData1c->CalcOffsetIntersectionPoint2(ptValS, pData2c, dToolD_abs, nSign1, nSign2);
							if ( ptResult )
								pt2 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							// pData1�̏I�_��␳
							ptResult = pData1c->CalcPerpendicularPoint(ptValO, ptValS, dToolD_abs, FALSE, k);
							if ( ptResult )
								pt1 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);
							// �̾�Č�_�܂ŵ�޼ު�Đ���
							if ( pDataResult = CreateNCobj(pData1c, pt2) ) {	// pt1 -> pt2
								pData1->AddCorrectObject(pDataResult);
								pData = pDataResult;
							}
							// �����~�ʂ̏ꍇ��
							if ( pData2c->GetType() == NCDARCDATA ) {
								// �n�_�𐂒��̾��
								ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
								if ( ptResult )
									pt3 = *ptResult;
								else {
									SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
									break;
								}
								// �̾�Č�_���炱���܂ŵ�޼ު�Đ���
								if ( pDataResult = CreateNCobj(pData, pt3) ) {	// pt2 -> pt3
									pData1->AddCorrectObject(pDataResult);
									pData = pDataResult;
								}
							}
							else
								pt3 = pt2;
							// pData2�̎n�_��␳
							pData2c->SetCorrectPoint(TRUE, pt3, dToolD_abs*nSign2);
						}
					}
					break;
				}
				// �����ް��̕␳��޼ު�Đ���
				if ( !obNdata.IsEmpty() ) {
					switch ( pData1->GetPlane() ) {
					case XY_PLANE:
						pt1 = pData1->GetEndCorrectPoint().GetXY();
						k = NCA_Z;
						break;
					case XZ_PLANE:
						pt1 = pData1->GetEndCorrectPoint().GetXZ();
						k = NCA_Y;
						break;
					case YZ_PLANE:
						pt1 = pData1->GetEndCorrectPoint().GetYZ();
						k = NCA_X;
						break;
					}
					pData = pData1->GetEndCorrectObject();
					for ( j=0; j<obNdata.GetSize(); j++ ) {
						if ( pDataResult = CreateNCobj(pData, pt1, obNdata[j]->GetEndPoint()[k]) ) {
							obNdata[j]->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
					}
					obNdata.RemoveAll();
				}
#ifdef _DEBUG
				ptValS = pData2c->GetStartPoint();	// !!!ATTENTION!!!
				dbg.printf("--> CorrectPoint X=%.3f Y=%.3f Z=%.3f",
					ptValS.x, ptValS.y, ptValS.z);
#endif
				// ����ٰ�߂�
				pData2->AddCorrectObject(pData2c);
				pData1  = pData2;
				pData1c = pData2c;
				ptValS  = ptValO;
				nCorrect = pData2->GetValFlags() & NCD_CORRECT ? 2 : 0;	// �␳ٰ�ߏI���H
			} // End of Loop
		} // End of Main(while) loop
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	pParent->m_ctReadProgress.SetPos(nLoopCnt);
	pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
#ifdef _DEBUG
	dbg.printf("PostMessage() Finish!");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

CPointD	CalcPerpendicularPoint
	(const CPointD& pto, const CPoint3D& ptVal, double r, int k, ENPLANE enPlane)
{
	CPointD	pt;
	// �C�Ӎ��W�����_��90����]
	switch ( enPlane ) {
	case XY_PLANE:
		pt = ptVal.GetXY() - pto;
		break;
	case XZ_PLANE:
		pt = ptVal.GetXZ() - pto;
		break;
	case YZ_PLANE:
		pt = ptVal.GetYZ() - pto;
		break;
	}
	double	q = atan2(pt.y, pt.x);
	CPointD	pt1(r*cos(q), r*sin(q));
	CPointD	pt2(-pt1.y*k, pt1.x*k);
	pt2 += pto;

	return pt2;
}

CNCdata* CreateNCobj
	(const CNCdata* pData, const CPointD& pt, double dValue/*=HUGE_VAL*/)
{
	NCARGV	ncArgv;
	ZeroMemory(&ncArgv, sizeof(NCARGV));

	CPoint3D	ptSrc(pData->GetEndPoint());
	BOOL		bCreate = TRUE;
	CNCdata*	pDataResult = NULL;

	// �K�v�ȏ������Ұ��̾��
	ncArgv.bAbs			= TRUE;
	ncArgv.dFeed		= pData->GetFeed();
	ncArgv.nc.nLine		= pData->GetStrLine();
	ncArgv.nc.nGtype	= G_TYPE;
	ncArgv.nc.nGcode	= pData->GetGcode() > 0 ? 1 : 0;
	ncArgv.nc.enPlane	= pData->GetPlane();

	// ���W�l�̾��
	switch ( pData->GetPlane() ) {
	case XY_PLANE:
		if ( pt == ptSrc.GetXY() ) {
			bCreate = FALSE;
			break;
		}
		ncArgv.nc.dValue[NCA_X] = pt.x;
		ncArgv.nc.dValue[NCA_Y] = pt.y;
		ncArgv.nc.dwValFlags = NCD_X|NCD_Y;
		if ( dValue != HUGE_VAL ) {
			ncArgv.nc.dValue[NCA_Z] = dValue;
			ncArgv.nc.dwValFlags |= NCD_Z;
		}
		break;
	case XZ_PLANE:
		if ( pt == ptSrc.GetXZ() ) {
			bCreate = FALSE;
			break;
		}
		ncArgv.nc.dValue[NCA_X] = pt.x;
		ncArgv.nc.dValue[NCA_Z] = pt.y;
		ncArgv.nc.dwValFlags = NCD_X|NCD_Z;
		if ( dValue != HUGE_VAL ) {
			ncArgv.nc.dValue[NCA_Y] = dValue;
			ncArgv.nc.dwValFlags |= NCD_Y;
		}
		break;
	case YZ_PLANE:
		if ( pt == ptSrc.GetYZ() ) {
			bCreate = FALSE;
			break;
		}
		ncArgv.nc.dValue[NCA_Y] = pt.x;
		ncArgv.nc.dValue[NCA_Z] = pt.y;
		ncArgv.nc.dwValFlags = NCD_Y|NCD_Z;
		if ( dValue != HUGE_VAL ) {
			ncArgv.nc.dValue[NCA_X] = dValue;
			ncArgv.nc.dwValFlags |= NCD_X;
		}
		break;
	}

	if ( bCreate ) {
		ncArgv.nc.dwValFlags |= (pData->GetValFlags() & NCD_CORRECT);
		pDataResult = new CNCline(pData, &ncArgv);
		pData = pDataResult;
	}

	return pDataResult;
}

void SetErrorCode(CNCDoc* pDoc, CNCdata* pData, int nID)
{
	// ��޼ު�Ĵװ����ۯ��ɂ��K�p
	int	nLine = pData->GetStrLine();
	if ( nLine < pDoc->GetNCBlockSize() )
		pDoc->GetNCblock(nLine)->SetNCBlkErrorCode(nID);
}
