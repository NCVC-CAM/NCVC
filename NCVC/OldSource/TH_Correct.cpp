// TH_Correct.cpp
// 補正計算
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
//	補正座標の計算ｽﾚｯﾄﾞ
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
				nCorrect = 0,	// 0:通常 1: 補正ﾌﾞﾛｯｸ直後 2:補正処理中
				nCorrectType = pMCopt->GetCorrectType(),
				nSign1, nSign2,
				k,			// 90°回転させる方向(始点を基準)
				nIO,		// 0:内側, 1:外側鋭角, 2:外側鈍角
				nResult = IDOK;
	double		dToolD, dToolD_abs, dAngle;
	optional<double>	dToolResult;
	CPoint3D	ptValS,		// 前回ｵﾌﾞｼﾞｪｸﾄの始点
				ptValO,		// 前回ｵﾌﾞｼﾞｪｸﾄの終点(今回ｵﾌﾞｼﾞｪｸﾄの始点)
				ptValE;		// 今回ｵﾌﾞｼﾞｪｸﾄの終点
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
				obNdata;	// 同一平面の移動を伴わない狭間のｵﾌﾞｼﾞｪｸﾄ

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

			// 補正処理が必要なｵﾌﾞｼﾞｪｸﾄを検索(第１ループはそこまで読み飛ばし)
			for ( ; i<nLoopCnt && nCorrect==0 && IsThread(); i++ ) {
				if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
					pParent->m_ctReadProgress.SetPos(i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
				pData1 = pDoc->GetNCdata(i);
				dwValFlags = pData1->GetValFlags();
				if ( dwValFlags & NCD_CORRECT ) {
					// 工具情報の取得
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
						continue;	// 工具情報無ければ読み飛ばし
					//
					dToolD_abs = fabs(dToolD);
					ptValS = pData1->GetStartPoint();
					pData1c = pData1->CopyObject();	// 複製
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

			// 補正開始ﾌﾞﾛｯｸのﾁｪｯｸ
			if ( pData1->GetType() != NCDLINEDATA ) {
				SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_CORRECTSTART);
				nCorrect = 0;	// 補正ﾙｰﾌﾟに入らない
			}

			// 補正ﾙｰﾌﾟ
			for ( ; i<nLoopCnt && nCorrect>0 && IsThread(); i++ ) {
				if ( (i & 0x003f) == 0 )
					pParent->m_ctReadProgress.SetPos(i);
				pData2 = pDoc->GetNCdata(i);
				dwValFlags = pData1->GetValFlags();
				// 補正ﾃﾞｰﾀﾁｪｯｸ
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
				// 同じ平面移動が無い場合(等しい場合を含む)はあとで処理
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
				// ｵﾌｾｯﾄ符号計算(G41(左)基準に計算)
				nSign1 = pData1->CalcOffsetSign(ptValS);	// pData1の始点
				nSign2 = pData2->CalcOffsetSign(ptValO);	// pData1の終点=>pData2の始点
				if ( nSign1==0 || nSign2==0 ) {
					SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_GTYPE);
					pData1 = pData2;
					continue;
				}
				if ( dwValFlags & NCD_CORRECT_R ) {
					nSign1 = -nSign1;	// G42の場合は符号反転
					nSign2 = -nSign2;
					k = -1;			// -90°回転
				}
				else
					k = 1;			// +90°回転
				if ( dToolD < 0 ) {
					nSign1 = -nSign1;	// 工具径がﾏｲﾅｽの場合は符号反転
					nSign2 = -nSign2;
					k = -k;
				}
				// ｵﾌﾞｼﾞｪｸﾄ間の角度を求める(-180°～180°)
				dAngle = pData1->CalcBetweenAngle(ptValS, pData2);
				// 内側か外側(鋭角・鈍角)か？(180°では内側処理)
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
				// 補正座標計算
				pData2c = pData2->CopyObject();	// 複製
				switch ( nIO ) {
				case 0:		// 内側
					// 補正処理中か開始ﾌﾞﾛｯｸか
					if ( nCorrect > 1 ) {
						// ｵﾌｾｯﾄ分移動させた交点
						ptResult = pData1c->CalcOffsetIntersectionPoint(ptValS, pData2c, dToolD_abs, nSign1, nSign2);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
					}
					else {
						// 開始ﾌﾞﾛｯｸでは次のｵﾌﾞｼﾞｪｸﾄの始点を垂直にｵﾌｾｯﾄ
						ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
					}
					// 交点計算や垂直ｵﾌｾｯﾄ点が新たな
					pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);	// pData1の終点
					pData2c->SetCorrectPoint(TRUE,  pt1, dToolD_abs*nSign2);	// pData2の始点
					break;
				case 1:		// 外側鋭角
					if ( nCorrect == 1 && nCorrectType == 0 ) {	// 開始ﾌﾞﾛｯｸ かつ ﾀｲﾌﾟA
						// 開始ﾌﾞﾛｯｸでは次のｵﾌﾞｼﾞｪｸﾄの始点を垂直にｵﾌｾｯﾄ
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
					else {		// 補正中 または ﾀｲﾌﾟB
						// 終点を垂直ｵﾌｾｯﾄ
						ptResult = pData1c->CalcPerpendicularPoint(ptValO, ptValS, dToolD_abs, FALSE, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// pt1を原点にpData1の終点を90°回転
						pt2 = CalcPerpendicularPoint(pt1, ptValO, dToolD_abs, k, pData1c->GetPlane());
						// 最終点はpData2の始点を垂直ｵﾌｾｯﾄ
						ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
						if ( ptResult )
							pt4 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// pt4を原点にpData2の始点を90°回転
						pt3 = CalcPerpendicularPoint(pt4, ptValO, dToolD_abs, -k, pData2c->GetPlane());
						// pData1の終点を補正
						pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);
						// それぞれの点でｵﾌﾞｼﾞｪｸﾄ生成
						if ( pDataResult = CreateNCobj(pData1c, pt2) ) {	// pt1 -> pt2
							pData1->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
						if ( pDataResult = CreateNCobj(pData, pt3) ) {		// pt2 -> pt3
							pData1->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
						if ( pDataResult = CreateNCobj(pData, pt4) ) {		// pt3 -> pt4
							pData2->AddCorrectObject(pDataResult);	// 次のｵﾌﾞｼﾞｪｸﾄに登録
							pData = pDataResult;
						}
						// pData2の始点を補正
						pData2c->SetCorrectPoint(TRUE, pt4, dToolD_abs*nSign2);
					}
					break;
				case 2:		// 外側鈍角
					if ( nCorrect > 1 ) {
						// ｵﾌｾｯﾄ分移動させた交点(円弧は接線)
						ptResult = pData1c->CalcOffsetIntersectionPoint2(ptValS, pData2c, dToolD_abs, nSign1, nSign2);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// 終点補正
						if ( pData1c->GetType() == NCDARCDATA ) {
							// 終点を垂直ｵﾌｾｯﾄ
							ptResult = pData1c->CalcPerpendicularPoint(ptValO, ptValS, dToolD_abs, FALSE, k);
							if ( ptResult )
								pt2 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							// pData1の終点を補正
							pData1c->SetCorrectPoint(FALSE, pt2, dToolD_abs*nSign1);
							// ｵﾌｾｯﾄ交点までｵﾌﾞｼﾞｪｸﾄ生成
							if ( pDataResult = CreateNCobj(pData1c, pt1) ) {
								pData1->AddCorrectObject(pDataResult);
								pData = pDataResult;
							}
						}
						else {
							// pData1の終点を補正
							pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);
							// 次のｵﾌﾞｼﾞｪｸﾄ生成用
							pData = pData1c;
						}
						// 始点補正
						if ( pData2c->GetType() == NCDARCDATA ) {
							// 始点を垂直ｵﾌｾｯﾄ
							ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
							if ( ptResult )
								pt3 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							// ｵﾌｾｯﾄ交点からここまでｵﾌﾞｼﾞｪｸﾄ生成
							if ( pDataResult = CreateNCobj(pData, pt3) ) {
								pData1->AddCorrectObject(pDataResult);
								pData = pDataResult;
							}
							// pData2の始点を補正
							pData2c->SetCorrectPoint(TRUE, pt3, dToolD_abs*nSign1);
						}
						else {
							// pData2の始点を補正
							pData2c->SetCorrectPoint(TRUE, pt1, dToolD_abs*nSign1);
						}
					}
					else {
						if ( nCorrectType == 0 ) {	// ﾀｲﾌﾟA
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
						else {						// ﾀｲﾌﾟB
							// ｵﾌｾｯﾄ分移動させた交点(円弧は接線)
							ptResult = pData1c->CalcOffsetIntersectionPoint2(ptValS, pData2c, dToolD_abs, nSign1, nSign2);
							if ( ptResult )
								pt2 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							// pData1の終点を補正
							ptResult = pData1c->CalcPerpendicularPoint(ptValO, ptValS, dToolD_abs, FALSE, k);
							if ( ptResult )
								pt1 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							pData1c->SetCorrectPoint(FALSE, pt1, dToolD_abs*nSign1);
							// ｵﾌｾｯﾄ交点までｵﾌﾞｼﾞｪｸﾄ生成
							if ( pDataResult = CreateNCobj(pData1c, pt2) ) {	// pt1 -> pt2
								pData1->AddCorrectObject(pDataResult);
								pData = pDataResult;
							}
							// 次が円弧の場合は
							if ( pData2c->GetType() == NCDARCDATA ) {
								// 始点を垂直ｵﾌｾｯﾄ
								ptResult = pData2c->CalcPerpendicularPoint(ptValO, ptValE, dToolD_abs, TRUE, k);
								if ( ptResult )
									pt3 = *ptResult;
								else {
									SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
									break;
								}
								// ｵﾌｾｯﾄ交点からここまでｵﾌﾞｼﾞｪｸﾄ生成
								if ( pDataResult = CreateNCobj(pData, pt3) ) {	// pt2 -> pt3
									pData1->AddCorrectObject(pDataResult);
									pData = pDataResult;
								}
							}
							else
								pt3 = pt2;
							// pData2の始点を補正
							pData2c->SetCorrectPoint(TRUE, pt3, dToolD_abs*nSign2);
						}
					}
					break;
				}
				// 狭間ﾃﾞｰﾀの補正ｵﾌﾞｼﾞｪｸﾄ生成
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
				// 次のﾙｰﾌﾟへ
				pData2->AddCorrectObject(pData2c);
				pData1  = pData2;
				pData1c = pData2c;
				ptValS  = ptValO;
				nCorrect = pData2->GetValFlags() & NCD_CORRECT ? 2 : 0;	// 補正ﾙｰﾌﾟ終了？
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
	// 任意座標を原点に90°回転
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

	// 必要な初期ﾊﾟﾗﾒｰﾀのｾｯﾄ
	ncArgv.bAbs			= TRUE;
	ncArgv.dFeed		= pData->GetFeed();
	ncArgv.nc.nLine		= pData->GetStrLine();
	ncArgv.nc.nGtype	= G_TYPE;
	ncArgv.nc.nGcode	= pData->GetGcode() > 0 ? 1 : 0;
	ncArgv.nc.enPlane	= pData->GetPlane();

	// 座標値のｾｯﾄ
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
	// ｵﾌﾞｼﾞｪｸﾄｴﾗｰをﾌﾞﾛｯｸにも適用
	int	nLine = pData->GetStrLine();
	if ( nLine < pDoc->GetNCBlockSize() )
		pDoc->GetNCblock(nLine)->SetNCBlkErrorCode(nID);
}
