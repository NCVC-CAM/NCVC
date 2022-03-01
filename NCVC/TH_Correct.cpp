// TH_Correct.cpp
// 補正計算
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
//	補正座標の計算ｽﾚｯﾄﾞ
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
	int			nCorrect = 0,	// 0:補正ｷｬﾝｾﾙ 1: 補正ﾌﾞﾛｯｸ直後 2:補正処理中
				nCorrectType = pMCopt->GetInt(MC_INT_CORRECTTYPE),
				nSign1, nSign2,
				k,			// 90°回転させる方向(始点を基準)
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
	CNCarray	obNdata;	// 同一平面の移動を伴わない狭間のｵﾌﾞｼﾞｪｸﾄ

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

		// 補正処理が必要なｵﾌﾞｼﾞｪｸﾄを検索(第１ループはそこまで読み飛ばし)
		for ( dToolD=HUGE_VALF, bFirst=TRUE; i<nLoopCnt && nCorrect==0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				pParent->m_ctReadProgress.SetPos((int)i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			pData1 = pDoc->GetNCdata(i);
			dwValFlags = pData1->GetValFlags();
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
				pData1c = pData1->NC_CopyObject();	// 複製
				if ( !pData1c ) {
					SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_CORRECTOBJECT);
					continue;
				}
				pData1->AddCorrectObject(pData1c);
				dToolD_abs = fabs(dToolD);
				nCorrect = 1;	// 補正ﾙｰﾌﾟへ(break)
#ifdef _DEBUG
				printf("Gcode=%d X=%.3f Y=%.3f Z=%.3f\n", pData1->GetGcode(),
					pData1->GetEndValue(NCA_X),
					pData1->GetEndValue(NCA_Y),
					pData1->GetEndValue(NCA_Z) );
#endif
			}
		}

		// 補正開始ﾌﾞﾛｯｸのﾁｪｯｸ
		if ( i<nLoopCnt && IsThread() ) {
			if ( pData1->GetType() != NCDLINEDATA ) {
				SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_CORRECTLINE);
				nCorrect = 0;	// 補正ﾙｰﾌﾟに入らない
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
		// 補正ﾙｰﾌﾟ
		for ( ; i<nLoopCnt && nCorrect>0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )
				pParent->m_ctReadProgress.SetPos((int)i);
			pData2 = pDoc->GetNCdata(i);
			if ( pData2->GetGtype() != G_TYPE )
				continue;
			dwValFlags = pData1->GetValFlags();
			// 補正処理ｷｬﾝｾﾙﾓｰﾄﾞ
			if ( !(pData2->GetValFlags() & NCD_CORRECT) )
				nCorrect = 0;	// 次のﾃﾞｰﾀ(pData2)で終了
			if ( pData2->GetValFlags() & NCD_D )
				SetErrorCode(pDoc, pData2, IDS_ERR_NCBLK_CORRECTING);
			// 補正ﾃﾞｰﾀﾁｪｯｸ
			k = IsCorrectCheck(pData1, pData2, nCorrect);
			if ( k ) {
				SetErrorCode(pDoc, pData2, k);
				pData1 = pData2;
				nSign1 = pData1->CalcOffsetSign();	// 符号再計算
				if ( dwValFlags & NCD_CORRECT_R )
					nSign1 = -nSign1;
				if ( dToolD < 0 )
					nSign1 = -nSign1;
				continue;
			}
			// 平面ﾁｪｯｸ(同一平面に動きのないｵﾌﾞｼﾞｪｸﾄはobNdataに溜めて後回し)
			if ( !IsPlaneCheck(pData1, pData2, obNdata) )
				continue;	// pData1はそのまま
#ifdef _DEBUG
			printf("Gcode=%d X=%.3f Y=%.3f Z=%.3f\n", pData2->GetGcode(),
				pData2->GetEndValue(NCA_X),
				pData2->GetEndValue(NCA_Y),
				pData2->GetEndValue(NCA_Z) );
#endif
			// ｵﾌｾｯﾄ符号計算(G41(左)基準に計算)
			nSign2 = pData2->CalcOffsetSign();	// pData1の終点=>pData2の始点
			if ( dwValFlags & NCD_CORRECT_R ) {
				nSign2 = -nSign2;	// G42の場合は符号反転
				k = -1;			// -90°回転
			}
			else
				k = 1;			// +90°回転
			if ( dToolD < 0 ) {
				nSign2 = -nSign2;	// 工具径がﾏｲﾅｽの場合は符号反転
				k = -k;
			}

			// 補正座標計算
			pData2c = pData2->NC_CopyObject();	// 複製
			if ( !pData2c ) {
				SetErrorCode(pDoc, pData2, IDS_ERR_NCBLK_CORRECTOBJECT);
				pData1 = pData2;
				nSign1 = nSign2;
				continue;
			}

			// 内側か外側(鋭角・鈍角)か？
			switch ( GetInsideOutside(pData1, pData2, dwValFlags) ) {
			// ---
			case 0:		// 内側
				switch ( nCorrect ) {
				case 0:		// ｵﾌｾｯﾄｷｬﾝｾﾙ
					// 終点を垂直ｵﾌｾｯﾄ
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
					break;
				case 1:		// ｵﾌｾｯﾄｽﾀｰﾄ
					// 開始ﾌﾞﾛｯｸでは次のｵﾌﾞｼﾞｪｸﾄの始点を垂直にｵﾌｾｯﾄ
					ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
					break;
				case 2:		// ｵﾌｾｯﾄﾓｰﾄﾞ
					// ｵﾌｾｯﾄ分移動させた交点
					ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dToolD_abs, dToolD_abs, k>0);
					break;
				}
				// 結果確認
				if ( ptResult ) {
					pt1 = *ptResult;
					// 交点計算や垂直ｵﾌｾｯﾄ点が新たな
					pData1c->SetCorrectPoint(ENDPOINT,   pt1, dToolD_abs*nSign1);	// pData1の終点
					pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign2);	// pData2の始点
				}
				else
					SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
				break;
			// ---
			case 1:		// 外側鋭角
				if ( nCorrect==0 && nCorrectType==MC_TYPE_A ) {			// ｵﾌｾｯﾄｷｬﾝｾﾙ かつ ﾀｲﾌﾟA
					// 終点を垂直ｵﾌｾｯﾄ
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
					if ( ptResult ) {
						pt1 = *ptResult;
						pData1c->SetCorrectPoint(ENDPOINT,   pt1, dToolD_abs*nSign1);
						pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign2);
					}
					else
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
				}
				else if ( nCorrect==1 && nCorrectType==MC_TYPE_A ) {	// 開始ﾌﾞﾛｯｸ かつ ﾀｲﾌﾟA
					// 開始ﾌﾞﾛｯｸでは次のｵﾌﾞｼﾞｪｸﾄの始点を垂直にｵﾌｾｯﾄ
					ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
					if ( ptResult ) {
						pt1 = *ptResult;
						pData1c->SetCorrectPoint(ENDPOINT,   pt1, dToolD_abs*nSign1);
						pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign2);
					}
					else
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
				}
				else {		// 補正中 または ﾀｲﾌﾟB
					// 終点を垂直ｵﾌｾｯﾄ
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
					if ( ptResult )
						pt1 = *ptResult;
					else {
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
						break;
					}
					// pt1を原点にpData1の終点を90°回転
					pt2 = CalcPerpendicularPoint(pt1, pData1->GetEndPoint(), dToolD_abs, k, pData1->GetPlane());
					// 最終点はpData2の始点を垂直ｵﾌｾｯﾄ
					ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
					if ( ptResult )
						pt4 = *ptResult;
					else {
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
						break;
					}
					// pt4を原点にpData2の始点を90°回転
					pt3 = CalcPerpendicularPoint(pt4, pData2->GetStartPoint(), dToolD_abs, -k, pData2->GetPlane());
					// pData1の終点を補正
					pData1c->SetCorrectPoint(ENDPOINT, pt1, dToolD_abs*nSign1);
					// それぞれの点でｵﾌﾞｼﾞｪｸﾄ生成
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
						pData2->AddCorrectObject(pDataResult);	// 次のｵﾌﾞｼﾞｪｸﾄに登録
					// pData2の始点を補正
					pData2c->SetCorrectPoint(STARTPOINT, pt4, dToolD_abs*nSign2);
				}
				break;
			// ---
			case 2:		// 外側鈍角
				switch ( nCorrect ) {
				case 0:		// ｵﾌｾｯﾄｷｬﾝｾﾙ
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
				case 1:		// ｵﾌｾｯﾄｽﾀｰﾄ
					if ( nCorrectType == MC_TYPE_A ) {	// ﾀｲﾌﾟA
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
					else {						// ﾀｲﾌﾟB
						// ｵﾌｾｯﾄ分移動させた交点(円弧は接線)
						ptResult = pData1->CalcOffsetIntersectionPoint2(pData2, dToolD_abs, k>0);
						if ( ptResult )
							pt2 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// pData1の終点を補正
						ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
						if ( ptResult )
							pt1 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						pData1c->SetCorrectPoint(ENDPOINT, pt1, dToolD_abs*nSign1);
						// ｵﾌｾｯﾄ交点までｵﾌﾞｼﾞｪｸﾄ生成
						if ( pDataResult = CreateNCobj(pData1c, pt2) ) {	// pt1 -> pt2
							pData1->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
						else
							pData = pData1c;
						// 次が円弧の場合は
						if ( pData2->GetType() == NCDARCDATA ) {
							// 始点を垂直ｵﾌｾｯﾄ
							ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
							if ( ptResult )
								pt3 = *ptResult;
							else {
								SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
								break;
							}
							// ｵﾌｾｯﾄ交点からここまでｵﾌﾞｼﾞｪｸﾄ生成
							if ( pDataResult = CreateNCobj(pData, pt3) )	// pt2 -> pt3
								pData1->AddCorrectObject(pDataResult);
						}
						else
							pt3 = pt2;
						// pData2の始点を補正
						pData2c->SetCorrectPoint(STARTPOINT, pt3, dToolD_abs*nSign2);
					}
					break;
				case 2:		// ｵﾌｾｯﾄﾓｰﾄﾞ
					// ｵﾌｾｯﾄ分移動させた交点(円弧は接線)
					ptResult = pData1->CalcOffsetIntersectionPoint2(pData2, dToolD_abs, k>0);
					if ( ptResult )
						pt1 = *ptResult;
					else {
						SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
						break;
					}
					// 終点補正
					if ( pData1->GetType() == NCDARCDATA ) {
						// 終点を垂直ｵﾌｾｯﾄ
						ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dToolD_abs, k);
						if ( ptResult )
							pt2 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// pData1の終点を補正
						pData1c->SetCorrectPoint(ENDPOINT, pt2, dToolD_abs*nSign1);
						// ｵﾌｾｯﾄ交点までｵﾌﾞｼﾞｪｸﾄ生成
						if ( pDataResult = CreateNCobj(pData1c, pt1) ) {
							pData1->AddCorrectObject(pDataResult);
							pData = pDataResult;
						}
						else
							pData = pData1c;
					}
					else {
						// pData1の終点を補正
						pData1c->SetCorrectPoint(ENDPOINT, pt1, dToolD_abs*nSign1);
						// 次のｵﾌﾞｼﾞｪｸﾄ生成用
						pData = pData1c;
					}
					// 始点補正
					if ( pData2->GetType() == NCDARCDATA ) {
						// 始点を垂直ｵﾌｾｯﾄ
						ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dToolD_abs, k);
						if ( ptResult )
							pt3 = *ptResult;
						else {
							SetErrorCode(pDoc, pData1, IDS_ERR_NCBLK_INTERSECTION);
							break;
						}
						// ｵﾌｾｯﾄ交点からここまでｵﾌﾞｼﾞｪｸﾄ生成
						if ( pDataResult = CreateNCobj(pData, pt3) )
							pData1->AddCorrectObject(pDataResult);
						// pData2の始点を補正
						pData2c->SetCorrectPoint(STARTPOINT, pt3, dToolD_abs*nSign1);
					}
					else {
						// pData2の始点を補正
						pData2c->SetCorrectPoint(STARTPOINT, pt1, dToolD_abs*nSign1);
					}
					break;
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
			// 次のﾙｰﾌﾟへ
/*
			// G68との組み合わせで不具合発生
			// なくて正常（何のための処理か思い出せない）
			switch ( nCorrect ) {
			case 0:
				// G40ｷｬﾝｾﾙで軸指定がない終点座標を補正
				ptResult = SetCorrectCancel(pData1c, pData2c);
				if ( ptResult ) {
					// 続く正規ｵﾌﾞｼﾞｪｸﾄ座標を調整
					for ( j=i+1; j<nLoopCnt && IsThread() ; j++ ) {
						pData1 = pDoc->GetNCdata(j);
						if ( pData1->GetGtype() == G_TYPE ) {
							if ( pData1->GetType() != NCDLINEDATA )
								break;
							// 次の始点と終点を補正座標値に変更
							pData1->SetCorrectPoint(STARTPOINT, *ptResult, 0);
							ptResult = SetCorrectCancel(pData2c, pData1);
							if ( ptResult ) {
								// 終点が変更されれば、さらに次の始点も更新
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
	// 同じ平面移動が無い場合(等しい場合を含む)はあとで処理
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

	// ｵﾌﾞｼﾞｪｸﾄ間の角度を求める(-180°～180°)
	// 精度よすぎてダメな場合が多いので、小数第１位で四捨五入
	float	dAngle = RoundUp(DEG(pData1->CalcBetweenAngle(pData2)/100.0f)) * 100.0f;
	
	// 内側か外側(鋭角・鈍角)か？(180°では内側処理)
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

	return nResult;		// 0:内側, 1:外側鋭角, 2:外側鈍角
}

optional<CPointF> SetCorrectCancel(const CNCdata* pData1, CNCdata* pData2)
{
	DWORD	dwValFlags = pData2->GetValFlags();
	CPointF	pt;
	BOOL	bChange = FALSE;

	// G40ｷｬﾝｾﾙで軸指定がない終点座標を補正
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
		pData2->SetCorrectPoint(ENDPOINT, pt, 0);	// 直線補間のみ
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

	// 必要な初期ﾊﾟﾗﾒｰﾀのｾｯﾄ
	ncArgv.bAbs			= TRUE;
	ncArgv.bG98			= pData->GetG98();
	ncArgv.dFeed		= pData->GetFeed();
	ncArgv.dEndmill		= pData->GetEndmill();
	ncArgv.nEndmillType	= pData->GetEndmillType();
	ncArgv.nc.nLine		= pData->GetBlockLineNo();
	ncArgv.nc.nGtype	= G_TYPE;
	ncArgv.nc.nGcode	= pData->GetGcode() > 0 ? 1 : 0;
	ncArgv.nc.enPlane	= pData->GetPlane();
	// !!! G68回転後の座標で計算しているので !!!
	// !!! ncArgv.g68 にセットしてはいけない !!!
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
	// 座標値のｾｯﾄ
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
	pDataResult = new CNCline(pData, &ncArgv, CPoint3F());	// オフセットが２重に足される
	ASSERT( pDataResult );

	return pDataResult;
}

void SetErrorCode(CNCDoc* pDoc, CNCdata* pData, int nID)
{
	// ｵﾌﾞｼﾞｪｸﾄｴﾗｰをﾌﾞﾛｯｸにも適用
	int	nLine = pData->GetBlockLineNo();
	if ( nLine < pDoc->GetNCBlockSize() )
		pDoc->GetNCblock(nLine)->SetNCBlkErrorCode(nID);
}
