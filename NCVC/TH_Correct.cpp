// TH_Correct.cpp
// �␳�v�Z
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;

#define	IsThread()	pParent->IsThreadContinue()

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp

static	int			IsCorrectCheck(const CNCdata*, const CNCdata*, int);
static	BOOL		IsPlaneCheck(const CNCdata*, const CNCdata*, CNCarray&);
static	int			GetInsideOutside(const CNCdata*, const CNCdata*, DWORD);
static	CPointF		CalcPerpendicularPoint(const CPointF&, const CPoint3F&, float, int, ENPLANE);
static	optional<CPointF>	SetCorrectCancel(const CNCdata*, CNCdata*);
static	CNCdata*	CreateNCobj(const CNCdata*, const CPointF&, float = HUGE_VALF);
static	void		SetErrorCode(CNCDoc*, CNCdata*, int);

//////////////////////////////////////////////////////////////////////
//	�␳���W�̌v�Z�گ��
//////////////////////////////////////////////////////////////////////

UINT CorrectCalc_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("CorrectCalc_Thread() Start\n");
	CPoint3F	ptDBG;
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CThreadDlg*		pParent = pParam->pParent;
	ASSERT(pDoc);
	ASSERT(pParent);

	const CMachineOption* pMCopt = AfxGetNCVCApp()->GetMachineOption();
	INT_PTR		i=0, j, nLoopCnt = pDoc->GetNCsize();
	int			nCorrect = 0,	// 0:�␳��ݾ� 1: �␳��ۯ����� 2:�␳������
				nCorrectType = pMCopt->GetInt(MC_INT_CORRECTTYPE),
				nSign1, nSign2,
				k,			// 90����]���������(�n�_���)
				nResult = IDOK;
	BOOL		bFirst;
	float		dToolD, dToolD_abs;
	optional<float>	dToolResult;
	CPointF		pt1, pt2, pt3, pt4;
	optional<CPointF>	ptResult;
	DWORD		dwValFlags;
	CNCdata*	pData1;
	CNCdata*	pData2;
	CNCdata*	pData1c = NULL;
	CNCdata*	pData2c = NULL;
	CNCdata*	pData;
	CNCdata*	pDataResult;
	CNCarray	obNdata;	// ���ꕽ�ʂ̈ړ��𔺂�Ȃ����Ԃ̵�޼ު��

	{
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_CORRECT_NCD));
		pParent->SetFaseMessage(strMsg);
	}
	pParent->m_ctReadProgress.SetRange32(0, (int)nLoopCnt);
#ifdef _DEBUG
	printf("GetNCsize()=%Id\n", nLoopCnt);
#endif

try {

	while ( i<nLoopCnt && IsThread() ) {

		// �␳�������K�v�ȵ�޼ު�Ă�����(��P���[�v�͂����܂œǂݔ�΂�)
		for ( dToolD=HUGE_VALF, bFirst=TRUE; i<nLoopCnt && nCorrect==0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
				pParent->m_ctReadProgress.SetPos((int)i);		// ��۸�ڽ�ް
			pData1 = pDoc->GetNCdata(i);
			dwValFlags = pData1->GetValFlags();
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
			if ( pData1->GetGtype() != G_TYPE )
				continue;
			//
			if ( dwValFlags & NCD_CORRECT ) {
				if ( dToolD == HUGE_VALF ) {
					if ( bFirst ) {
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_CORRECTSTART);
						bFirst = FALSE;
					}
					continue;
				}
				pData1c = pData1->NC_CopyObject();	// ����
				if ( !pData1c ) {
					SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_CORRECTOBJECT);
					continue;
				}
				pData1->AddCorrectObject(pData1c);
				dToolD_abs = fabs(dToolD);
				nCorrect = 1;	// �␳ٰ�߂�(break)
#ifdef _DEBUG
				printf("Gcode=%d X=%.3f Y=%.3f Z=%.3f\n", pData1->GetGcode(),
					pData1->GetEndValue(NCA_X),
					pData1->GetEndValue(NCA_Y),
					pData1->GetEndValue(NCA_Z) );
#endif
			}
		}

		// �␳�J�n��ۯ�������
		if ( i<nLoopCnt && IsThread() ) {
			if ( pData1->GetType() != NCDLINEDATA ) {
				SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_CORRECTLINE);
				nCorrect = 0;	// �␳ٰ�߂ɓ���Ȃ�
			}
			else {
				nSign1 = pData1->CalcOffsetSign();
				if ( dwValFlags & NCD_CORRECT_R )
					nSign1 = -nSign1;
				if ( dToolD < 0 )
					nSign1 = -nSign1;
			}
		}

		// ----------
		// �␳ٰ��
		for ( ; i<nLoopCnt && nCorrect>0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )
				pParent->m_ctReadProgress.SetPos((int)i);
			pData2 = pDoc->GetNCdata(i);
			if ( pData2->GetGtype() != G_TYPE )
				continue;
			dwValFlags = pData1->GetValFlags();
			// �␳������ݾ�Ӱ��
			if ( !(pData2->GetValFlags() & NCD_CORRECT) )
				nCorrect = 0;	// �����ް�(pData2)�ŏI��
			if ( pData2->GetValFlags() & NCD_D )
				SetErrorCode(pDoc, pData2, IDS_ERR_NCBLK_CORRECTING);
			// �␳�ް�����
			k = IsCorrectCheck(pData1, pData2, nCorrect);
			if ( k ) {
				SetErrorCode(pDoc, pData2, k);
				pData1 = pData2;
				nSign1 = pData1->CalcOffsetSign();	// �����Čv�Z
				if ( dwValFlags & NCD_CORRECT_R )
					nSign1 = -nSign1;
				if ( dToolD < 0 )
					nSign1 = -nSign1;
				continue;
			}
			// ��������(���ꕽ�ʂɓ����̂Ȃ���޼ު�Ă�obNdata�ɗ��߂Č��)
			if ( !IsPlaneCheck(pData1, pData2, obNdata) )
				continue;	// pData1�͂��̂܂�
#ifdef _DEBUG
			printf("Gcode=%d X=%.3f Y=%.3f Z=%.3f\n", pData2->GetGcode(),
				pData2->GetEndValue(NCA_X),
				pData2->GetEndValue(NCA_Y),
				pData2->GetEndValue(NCA_Z) );
#endif
			// �̾�ĕ����v�Z(G41(��)��Ɍv�Z)
			nSign2 = pData2->CalcOffsetSign();	// pData1�̏I�_=>pData2�̎n�_
			if ( dwValFlags & NCD_CORRECT_R ) {
				nSign2 = -nSign2;	// G42�̏ꍇ�͕������]
				k = -1;			// -90����]
			}
			else
				k = 1;			// +90����]
			if ( dToolD < 0 ) {
				nSign2 = -nSign2;	// �H��a��ϲŽ�̏ꍇ�͕������]
				k = -k;
			}

			// �␳���W�v�Z
			pData2c = pData2->NC_CopyObject();	// ����
			if ( !pData2c ) {
				SetErrorCode(pDoc, pData2, IDS_ERR_NCBLK_CORRECTOBJECT);
				pData1 = pData2;
				nSign1 = nSign2;
				continue;
			}

			// �������O��(�s�p�E�݊p)���H
			switch ( GetInsideOutside(pData1, pData2, dwValFlags) ) {
			// ---
			case 0:		// ����
				switch ( nCorrect ) {
				case 0:		// �̾�ķ�ݾ�
					// �I�_�𐂒��̾��
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
					break;
				case 1:		// �̾�Ľ���
					// �J�n��ۯ��ł͎��̵�޼ު�Ă̎n�_�𐂒��ɵ̾��
					ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
					break;
				case 2:		// �̾��Ӱ��
					// �̾�ĕ��ړ���������_
					ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dToolD_abs, dToolD_abs, k>0);
					break;
				}
				// ���ʊm�F
				if ( ptResult ) {
					pt1 = *ptResult;
					// ��_�v�Z�␂���̾�ē_���V����
					pData1c->SetCorrectPoint(ENDPOINT,   pt1, dToolD_abs*nSign1);	// pData1�̏I�_
					pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign2);	// pData2�̎n�_
				}
				else
					SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
				break;
			// ---
			case 1:		// �O���s�p
				if ( nCorrect==0 && nCorrectType==MC_TYPE_A ) {			// �̾�ķ�ݾ� ���� ����A
					// �I�_�𐂒��̾��
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
					if ( ptResult ) {
						pt1 = *ptResult;
						pData1c->SetCorrectPoint(ENDPOINT,   pt1, dToolD_abs*nSign1);
						pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign2);
					}
					else
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
				}
				else if ( nCorrect==1 && nCorrectType==MC_TYPE_A ) {	// �J�n��ۯ� ���� ����A
					// �J�n��ۯ��ł͎��̵�޼ު�Ă̎n�_�𐂒��ɵ̾��
					ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
					if ( ptResult ) {
						pt1 = *ptResult;
						pData1c->SetCorrectPoint(ENDPOINT,   pt1, dToolD_abs*nSign1);
						pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign2);
					}
					else
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
				}
				else {		// �␳�� �܂��� ����B
					// �I�_�𐂒��̾��
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
					if ( ptResult )
						pt1 = *ptResult;
					else {
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
						break;
					}
					// pt1�����_��pData1�̏I�_��90����]
					pt2 = CalcPerpendicularPoint(pt1, pData1->GetEndPoint(), dToolD_abs, k, pData1->GetPlane());
					// �ŏI�_��pData2�̎n�_�𐂒��̾��
					ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
					if ( ptResult )
						pt4 = *ptResult;
					else {
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
						break;
					}
					// pt4�����_��pData2�̎n�_��90����]
					pt3 = CalcPerpendicularPoint(pt4, pData2->GetStartPoint(), dToolD_abs, -k, pData2->GetPlane());
					// pData1�̏I�_��␳
					pData1c->SetCorrectPoint(ENDPOINT, pt1, dToolD_abs*nSign1);
					// ���ꂼ��̓_�ŵ�޼ު�Đ���
					if ( pDataResult = CreateNCobj(pData1c, pt2) ) {	// pt1 -> pt2
						pData1->AddCorrectObject(pDataResult);
						pData = pDataResult;
					}
					else
						pData = pData1c;
					if ( pDataResult = CreateNCobj(pData, pt3) ) {		// pt2 -> pt3
						pData1->AddCorrectObject(pDataResult);
						pData = pDataResult;
					}
					if ( pDataResult = CreateNCobj(pData, pt4) )		// pt3 -> pt4
						pData2->AddCorrectObject(pDataResult);	// ���̵�޼ު�Ăɓo�^
					// pData2�̎n�_��␳
					pData2c->SetCorrectPoint(STARTPOINT, pt4, dToolD_abs*nSign2);
				}
				break;
			// ---
			case 2:		// �O���݊p
				switch ( nCorrect ) {
				case 0:		// �̾�ķ�ݾ�
					if ( nCorrectType == MC_TYPE_A ) {
						ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
						if ( ptResult ) {
							pt1 = *ptResult;
							pData1c->SetCorrectPoint(ENDPOINT,   pt1, dToolD_abs*nSign1);
							pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign2);
						}
						else
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
					}
					else {
						ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						ptResult = pData1->CalcOffsetIntersectionPoint2(pData2, dToolD_abs, k>0);
						if ( ptResult )
							pt2 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
						if ( ptResult )
							pt3 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						pData1c->SetCorrectPoint(ENDPOINT, pt1, dToolD_abs*nSign1);
						if ( pDataResult = CreateNCobj(pData1c, pt2) )
							pData1->AddCorrectObject(pDataResult);
						pData2c->SetCorrectPoint(STARTPOINT, pt3, dToolD_abs*nSign2);
					}
					break;
				case 1:		// �̾�Ľ���
					if ( nCorrectType == MC_TYPE_A ) {	// ����A
						ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						pData1c->SetCorrectPoint(ENDPOINT,   pt1, dToolD_abs*nSign1);
						pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign2);
					}
					else {						// ����B
						// �̾�ĕ��ړ���������_(�~�ʂ͐ڐ�)
						ptResult = pData1->CalcOffsetIntersectionPoint2(pData2, dToolD_abs, k>0);
						if ( ptResult )
							pt2 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// pData1�̏I�_��␳
						ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						pData1c->SetCorrectPoint(ENDPOINT, pt1, dToolD_abs*nSign1);
						// �̾�Č�_�܂ŵ�޼ު�Đ���
						if ( pDataResult = CreateNCobj(pData1c, pt2) ) {	// pt1 -> pt2
							pData1->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
						else
							pData = pData1c;
						// �����~�ʂ̏ꍇ��
						if ( pData2->GetType() == NCDARCDATA ) {
							// �n�_�𐂒��̾��
							ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
							if ( ptResult )
								pt3 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							// �̾�Č�_���炱���܂ŵ�޼ު�Đ���
							if ( pDataResult = CreateNCobj(pData, pt3) )	// pt2 -> pt3
								pData1->AddCorrectObject(pDataResult);
						}
						else
							pt3 = pt2;
						// pData2�̎n�_��␳
						pData2c->SetCorrectPoint(STARTPOINT, pt3, dToolD_abs*nSign2);
					}
					break;
				case 2:		// �̾��Ӱ��
					// �̾�ĕ��ړ���������_(�~�ʂ͐ڐ�)
					ptResult = pData1->CalcOffsetIntersectionPoint2(pData2, dToolD_abs, k>0);
					if ( ptResult )
						pt1 = *ptResult;
					else {
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
						break;
					}
					// �I�_�␳
					if ( pData1->GetType() == NCDARCDATA ) {
						// �I�_�𐂒��̾��
						ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
						if ( ptResult )
							pt2 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// pData1�̏I�_��␳
						pData1c->SetCorrectPoint(ENDPOINT, pt2, dToolD_abs*nSign1);
						// �̾�Č�_�܂ŵ�޼ު�Đ���
						if ( pDataResult = CreateNCobj(pData1c, pt1) ) {
							pData1->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
						else
							pData = pData1c;
					}
					else {
						// pData1�̏I�_��␳
						pData1c->SetCorrectPoint(ENDPOINT, pt1, dToolD_abs*nSign1);
						// ���̵�޼ު�Đ����p
						pData = pData1c;
					}
					// �n�_�␳
					if ( pData2->GetType() == NCDARCDATA ) {
						// �n�_�𐂒��̾��
						ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
						if ( ptResult )
							pt3 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// �̾�Č�_���炱���܂ŵ�޼ު�Đ���
						if ( pDataResult = CreateNCobj(pData, pt3) )
							pData1->AddCorrectObject(pDataResult);
						// pData2�̎n�_��␳
						pData2c->SetCorrectPoint(STARTPOINT, pt3, dToolD_abs*nSign1);
					}
					else {
						// pData2�̎n�_��␳
						pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign1);
					}
					break;
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
					pData1 = obNdata[j];
					if ( pDataResult = CreateNCobj(pData, pt1, pData1->GetEndPoint()[k]) ) {
						pData1->AddCorrectObject(pDataResult);
						pData = pDataResult;
					}
				}
				obNdata.RemoveAll();
			}
#ifdef _DEBUG
			ptDBG = pData2c->GetStartPoint();
			printf("--> CorrectPoint X=%.3f Y=%.3f Z=%.3f\n",
				ptDBG.x, ptDBG.y, ptDBG.z);
#endif
			// ����ٰ�߂�
/*
			// G68�Ƃ̑g�ݍ��킹�ŕs�����
			// �Ȃ��Đ���i���̂��߂̏������v���o���Ȃ��j
			switch ( nCorrect ) {
			case 0:
				// G40��ݾقŎ��w�肪�Ȃ��I�_���W��␳
				ptResult = SetCorrectCancel(pData1c, pData2c);
				if ( ptResult ) {
					// �������K��޼ު�č��W�𒲐�
					for ( j=i+1; j<nLoopCnt && IsThread() ; j++ ) {
						pData1 = pDoc->GetNCdata(j);
						if ( pData1->GetGtype() == G_TYPE ) {
							if ( pData1->GetType() != NCDLINEDATA )
								break;
							// ���̎n�_�ƏI�_��␳���W�l�ɕύX
							pData1->SetCorrectPoint(STARTPOINT, *ptResult, 0);
							ptResult = SetCorrectCancel(pData2c, pData1);
							if ( ptResult ) {
								// �I�_���ύX�����΁A����Ɏ��̎n�_���X�V
								for ( j++; j<nLoopCnt && IsThread(); j++ ) {
									pData1 = pDoc->GetNCdata(j);
									if ( pData1->GetGtype() == G_TYPE ) {
										if ( pData1->GetType() == NCDLINEDATA )
											pData1->SetCorrectPoint(STARTPOINT, *ptResult, 0);
										break;
									}
								}
							}
							break;
						}
					}
				}
				break;
			case 1:
				nCorrect++;		// 1 -> 2
				break;
			}
*/
			if ( nCorrect == 1 ) nCorrect++;	// 1 -> 2
			pData2->AddCorrectObject(pData2c);
			pData1  = pData2;
			nSign1  = nSign2;
			pData1c = pData2c;
		} // End of Loop
	} // End of Main(while) loop

}
catch (CMemoryException* e) {
	AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
	e->Delete();
	nResult = IDCANCEL;
}

	pParent->m_ctReadProgress.SetPos((int)nLoopCnt);
	pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
#ifdef _DEBUG
	printf("PostMessage() Finish!\n");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

CPointF	CalcPerpendicularPoint
	(const CPointF& pto, const CPoint3F& ptVal, float r, int k, ENPLANE enPlane)
{
	CPointF	pt;
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
	float	q = pt.arctan();
	CPointF	pt1(r*cos(q), r*sin(q));
	CPointF	pt2(-pt1.y*k, pt1.x*k);
	pt2 += pto;

	return pt2;
}

int IsCorrectCheck(const CNCdata* pData1, const CNCdata* pData2, int nCorrect)
{
	int	nResult = 0;

	if ( pData2->GetGtype()!=G_TYPE || 
			(pData2->GetType()!=NCDLINEDATA && pData2->GetType()!=NCDARCDATA) ) {
		nResult = IDS_ERR_NCBLK_GTYPE;
	}
	else if ( pData1->GetPlane() != pData2->GetPlane() ) {
		nResult = IDS_ERR_NCBLK_PLANE;
	}
	else if ( nCorrect==0 && pData2->GetType()==NCDARCDATA ) {
		nResult = IDS_ERR_NCBLK_ENDCIRCLE;
	}

	return nResult;
}

BOOL IsPlaneCheck(const CNCdata* pData1, const CNCdata* pData2, CNCarray& obNdata)
{
	// �������ʈړ��������ꍇ(�������ꍇ���܂�)�͂��Ƃŏ���
	switch ( pData1->GetPlane() ) {
	case XY_PLANE:
		switch ( pData2->GetType() ) {
		case NCDLINEDATA:
			if ( !(pData2->GetValFlags() & (NCD_X|NCD_Y)) ) {
				obNdata.Add(const_cast<CNCdata*>(pData2));
				return FALSE;
			}
			break;
		case NCDARCDATA:
			if ( !(pData2->GetValFlags() & (NCD_R|NCD_I|NCD_J)) ) {
				obNdata.Add(const_cast<CNCdata*>(pData2));
				return FALSE;
			}
			break;
		}
		break;
	case XZ_PLANE:
		switch ( pData2->GetType() ) {
		case NCDLINEDATA:
			if ( !(pData2->GetValFlags() & (NCD_X|NCD_Z)) ) {
				obNdata.Add(const_cast<CNCdata*>(pData2));
				return FALSE;
			}
			break;
		case NCDARCDATA:
			if ( !(pData2->GetValFlags() & (NCD_R|NCD_I|NCD_K)) ) {
				obNdata.Add(const_cast<CNCdata*>(pData2));
				return FALSE;
			}
			break;
		}
		break;
	case YZ_PLANE:
		switch ( pData2->GetType() ) {
		case NCDLINEDATA:
			if ( !(pData2->GetValFlags() & (NCD_Y|NCD_Z)) ) {
				obNdata.Add(const_cast<CNCdata*>(pData2));
				return FALSE;
			}
			break;
		case NCDARCDATA:
			if ( !(pData2->GetValFlags() & (NCD_R|NCD_J|NCD_K)) ) {
				obNdata.Add(const_cast<CNCdata*>(pData2));
				return FALSE;
			}
			break;
		}
		break;
	}

	if ( pData2->GetType()==NCDLINEDATA &&
			pData1->GetEndPoint()==pData2->GetEndPoint() ) {
		obNdata.Add(const_cast<CNCdata*>(pData2));
		return FALSE;
	}

	return TRUE;
}

int GetInsideOutside(const CNCdata* pData1, const CNCdata* pData2, DWORD dwValFlags)
{
	int		nResult;

	// ��޼ު�ĊԂ̊p�x�����߂�(-180���`180��)
	// ���x�悷���ă_���ȏꍇ�������̂ŁA������P�ʂŎl�̌ܓ�
	float	dAngle = RoundUp(DEG(pData1->CalcBetweenAngle(pData2)/100.0f)) * 100.0f;
	
	// �������O��(�s�p�E�݊p)���H(180���ł͓�������)
	if ( fabs(dAngle) >= 180.0f ) {
		nResult = 0;
	}
	else if ( dAngle > 0 ) {
		if ( dwValFlags & NCD_CORRECT_R )
			nResult = 0;
		else
			nResult = dAngle < 90.0f ? 1 : 2;
	}
	else {
		if ( dwValFlags & NCD_CORRECT_L )
			nResult = 0;
		else
			nResult = dAngle > -90.0f ? 1 : 2;
	}

	return nResult;		// 0:����, 1:�O���s�p, 2:�O���݊p
}

optional<CPointF> SetCorrectCancel(const CNCdata* pData1, CNCdata* pData2)
{
	DWORD	dwValFlags = pData2->GetValFlags();
	CPointF	pt;
	BOOL	bChange = FALSE;

	// G40��ݾقŎ��w�肪�Ȃ��I�_���W��␳
	switch ( pData1->GetPlane() ) {
	case XY_PLANE:
		if ( dwValFlags & g_dwSetValFlags[NCA_X] )
			pt.x = pData2->GetEndValue(NCA_X);
		else {
			pt.x = pData1->GetEndValue(NCA_X);
			bChange = TRUE;
		}
		if ( dwValFlags & g_dwSetValFlags[NCA_Y] )
			pt.y = pData2->GetEndValue(NCA_Y);
		else {
			pt.y = pData1->GetEndValue(NCA_Y);
			bChange = TRUE;
		}
		break;
	case XZ_PLANE:
		if ( dwValFlags & g_dwSetValFlags[NCA_X] )
			pt.x = pData2->GetEndValue(NCA_X);
		else {
			pt.x = pData1->GetEndValue(NCA_X);
			bChange = TRUE;
		}
		if ( dwValFlags & g_dwSetValFlags[NCA_Z] )
			pt.y = pData2->GetEndValue(NCA_Z);
		else {
			pt.y = pData1->GetEndValue(NCA_Z);
			bChange = TRUE;
		}
		break;
	case YZ_PLANE:
		if ( dwValFlags & g_dwSetValFlags[NCA_Y] )
			pt.x = pData2->GetEndValue(NCA_Y);
		else {
			pt.x = pData1->GetEndValue(NCA_Y);
			bChange = TRUE;
		}
		if ( dwValFlags & g_dwSetValFlags[NCA_Z] )
			pt.y = pData2->GetEndValue(NCA_Z);
		else {
			pt.y = pData1->GetEndValue(NCA_Z);
			bChange = TRUE;
		}
		break;
	}

	if ( bChange ) {
		pData2->SetCorrectPoint(ENDPOINT, pt, 0);	// ������Ԃ̂�
		return pt;
	}

	return optional<CPointF>();
}

CNCdata* CreateNCobj
	(const CNCdata* pData, const CPointF& pt, float dValue/*=HUGE_VALF*/)
{
	NCARGV	ncArgv;
	ZeroMemory(&ncArgv, sizeof(NCARGV));

	CNCdata*	pDataResult = NULL;

	// �K�v�ȏ������Ұ��̾��
	ncArgv.bAbs			= TRUE;
	ncArgv.bG98			= pData->GetG98();
	ncArgv.dFeed		= pData->GetFeed();
	ncArgv.dEndmill		= pData->GetEndmill();
	ncArgv.nEndmillType	= pData->GetEndmillType();
	ncArgv.nc.nLine		= pData->GetBlockLineNo();
	ncArgv.nc.nGtype	= G_TYPE;
	ncArgv.nc.nGcode	= pData->GetGcode() > 0 ? 1 : 0;
	ncArgv.nc.enPlane	= pData->GetPlane();
	// !!! G68��]��̍��W�Ōv�Z���Ă���̂� !!!
	// !!! ncArgv.g68 �ɃZ�b�g���Ă͂����Ȃ� !!!
/*
	if ( pData->GetReadData()->m_pG68 ) {
		ncArgv.g68.bG68		= TRUE;
		ncArgv.g68.enPlane	= pData->GetReadData()->m_pG68->enPlane;
		ncArgv.g68.dRound	= pData->GetReadData()->m_pG68->dRound;
		for ( int ii=0; ii<SIZEOF(ncArgv.g68.dOrg); ii++ )
			ncArgv.g68.dOrg[ii] = pData->GetReadData()->m_pG68->dOrg[ii];
	}
	else {
		ncArgv.g68.bG68 = FALSE;
	}
*/
	// ���W�l�̾��
	switch ( pData->GetPlane() ) {
	case XY_PLANE:
		ncArgv.nc.dValue[NCA_X] = pt.x;
		ncArgv.nc.dValue[NCA_Y] = pt.y;
		ncArgv.nc.dwValFlags = NCD_X|NCD_Y;
		if ( dValue != HUGE_VALF ) {
			ncArgv.nc.dValue[NCA_Z] = dValue;
			ncArgv.nc.dwValFlags |= NCD_Z;
		}
		break;
	case XZ_PLANE:
		ncArgv.nc.dValue[NCA_X] = pt.x;
		ncArgv.nc.dValue[NCA_Z] = pt.y;
		ncArgv.nc.dwValFlags = NCD_X|NCD_Z;
		if ( dValue != HUGE_VALF ) {
			ncArgv.nc.dValue[NCA_Y] = dValue;
			ncArgv.nc.dwValFlags |= NCD_Y;
		}
		break;
	case YZ_PLANE:
		ncArgv.nc.dValue[NCA_Y] = pt.x;
		ncArgv.nc.dValue[NCA_Z] = pt.y;
		ncArgv.nc.dwValFlags = NCD_Y|NCD_Z;
		if ( dValue != HUGE_VALF ) {
			ncArgv.nc.dValue[NCA_X] = dValue;
			ncArgv.nc.dwValFlags |= NCD_X;
		}
		break;
	}

	ncArgv.nc.dwValFlags |= (pData->GetValFlags() & NCD_CORRECT);
//	pDataResult = new CNCline(pData, &ncArgv, pData->GetOffsetPoint());
	pDataResult = new CNCline(pData, &ncArgv, CPoint3F());	// �I�t�Z�b�g���Q�d�ɑ������
	ASSERT( pDataResult );

	return pDataResult;
}

void SetErrorCode(CNCDoc* pDoc, CNCdata* pData, int nID)
{
	// ��޼ު�Ĵװ����ۯ��ɂ��K�p
	int	nLine = pData->GetBlockLineNo();
	if ( nLine < pDoc->GetNCBlockSize() )
		pDoc->GetNCblock(nLine)->SetNCBlkErrorCode(nID);
}
