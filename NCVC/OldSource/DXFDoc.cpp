// DXFDoc.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFChild.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "DXFDoc.h"
#include "DXFView.h"
#include "NCDoc.h"
#include "DxfEditOrgDlg.h"
#include "DxfAutoWorkingDlg.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx1.h"
#include "MakeNCDlgEx2.h"
#include "ThreadDlg.h"

#include <math.h>
#include <float.h>
#include <string.h>

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

/*	--- 廃止
// m_ptPaper[1] へのﾃﾞﾌｫﾙﾄ引数(A4ｻｲｽﾞ)
static	const	int		xExt = 420;
static	const	int		yExt = 297;
*/
// 原点を示すﾃﾞﾌｫﾙﾄの半径
static	const	double	dOrgR = 10.0;

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc

IMPLEMENT_DYNCREATE(CDXFDoc, CDocument)

BEGIN_MESSAGE_MAP(CDXFDoc, CDocument)
	//{{AFX_MSG_MAP(CDXFDoc)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_EDIT_DXFORG, OnEditOrigin)
	ON_COMMAND(ID_EDIT_DXFSHAPE, OnEditShape)
	ON_COMMAND(ID_EDIT_SHAPE_AUTO, OnEditAutoShape)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DXFSHAPE, OnUpdateEditShape)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SORTSHAPE, OnUpdateEditShaping)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHAPE_AUTO, OnUpdateEditShaping)
	//}}AFX_MSG_MAP
	// NC生成ﾒﾆｭｰ
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_DXF2NCD, ID_FILE_DXF2NCD_EX2, OnUpdateFileDXF2NCD)
	ON_COMMAND_RANGE(ID_FILE_DXF2NCD, ID_FILE_DXF2NCD_EX2, OnFileDXF2NCD)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc クラスの構築/消滅

CDXFDoc::CDXFDoc()
{
	int		i;
	// 初期状態はｴﾗｰ, ｽﾚｯﾄﾞは継続
	m_bShape = m_bReady = FALSE;
	m_bThread = m_bReload = TRUE;
	m_nShapePattern = 0;
	// ﾃﾞｰﾀ数初期化
	for ( i=0; i<SIZEOF(m_nDataCnt); i++ )
		m_nDataCnt[i] = 0;
	for ( i=0; i<SIZEOF(m_pCircle); i++ )
		m_pCircle[i] = NULL;
	// ｵﾌﾞｼﾞｪｸﾄ矩形の初期化
	m_rcMax.left = m_rcMax.top = DBL_MAX;
	m_rcMax.right = m_rcMax.bottom = -DBL_MAX;
	// ｵﾘｼﾞﾅﾙ原点の初期化
	m_ptOrgOrig = HUGE_VAL;
	// 増分割り当てサイズ
	m_obDXFArray.SetSize(0, 1024);
	m_obStartArray.SetSize(0, 64);
	m_obMoveArray.SetSize(0, 64);
	m_obDXFTextArray.SetSize(0, 64);
	m_obStartTextArray.SetSize(0, 64);
	m_obMoveTextArray.SetSize(0, 64);
	m_obCommentArray.SetSize(0, 64);
	m_obLayer.SetSize(0, 64);
}

CDXFDoc::~CDXFDoc()
{
	int		i;
	for ( i=0; i<m_obDXFArray.GetSize(); i++ )
		delete	m_obDXFArray[i];
	for ( i=0; i<m_obStartArray.GetSize(); i++ )
		delete	m_obStartArray[i];
	for ( i=0; i<m_obMoveArray.GetSize(); i++ )
		delete	m_obMoveArray[i];
	for ( i=0; i<m_obDXFTextArray.GetSize(); i++ )
		delete	m_obDXFTextArray[i];
	for ( i=0; i<m_obStartTextArray.GetSize(); i++ )
		delete	m_obStartTextArray[i];
	for ( i=0; i<m_obMoveTextArray.GetSize(); i++ )
		delete	m_obMoveTextArray[i];
	for ( i=0; i<m_obCommentArray.GetSize(); i++ )
		delete	m_obCommentArray[i];

	for ( i=0; i<SIZEOF(m_pCircle); i++ ) {
		if ( m_pCircle[i] )
			delete	m_pCircle[i];
	}
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

BOOL CDXFDoc::RouteCmdToAllViews
	(CView* pActiveView, UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	CView*	pView;

	for ( POSITION pos = GetFirstViewPosition(); pos; ) {
		pView = GetNextView(pos);
		if ( pView != pActiveView ) {
			if ( pView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
				return TRUE;
		}
	}
	return FALSE;
}

void CDXFDoc::DataOperation(CDXFpoint* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obDXFArray.Add(pData);
		break;
	case DXFINS:
		m_obDXFArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		RemoveSub(m_obDXFArray[nIndex]);	// 現在ﾃﾞｰﾀの削除と関連ﾃﾞｰﾀの減算
		m_obDXFArray.SetAt(nIndex, pData);
		break;
	}
	m_nDataCnt[DXFPOINTDATA]++;
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation(CDXFline* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obDXFArray.Add(pData);
		break;
	case DXFINS:
		m_obDXFArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		RemoveSub(m_obDXFArray[nIndex]);
		m_obDXFArray.SetAt(nIndex, pData);
		break;
	}
	m_nDataCnt[DXFLINEDATA]++;
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation(CDXFcircle* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obDXFArray.Add(pData);
		break;
	case DXFINS:
		m_obDXFArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		RemoveSub(m_obDXFArray[nIndex]);
		m_obDXFArray.SetAt(nIndex, pData);
		break;
	}
	m_nDataCnt[DXFCIRCLEDATA]++;
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation(CDXFarc* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obDXFArray.Add(pData);
		break;
	case DXFINS:
		m_obDXFArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		RemoveSub(m_obDXFArray[nIndex]);
		m_obDXFArray.SetAt(nIndex, pData);
		break;
	}
	m_nDataCnt[DXFARCDATA]++;
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation(CDXFellipse* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obDXFArray.Add(pData);
		break;
	case DXFINS:
		m_obDXFArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		RemoveSub(m_obDXFArray[nIndex]);
		m_obDXFArray.SetAt(nIndex, pData);
		break;
	}
	m_nDataCnt[DXFELLIPSEDATA]++;
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation(CDXFtext* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obDXFTextArray.Add(pData);
		break;
	case DXFINS:
		m_obDXFTextArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		RemoveSub(m_obDXFTextArray[nIndex]);
		m_obDXFTextArray.SetAt(nIndex, pData);
		break;
	}
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation(CDXFpolyline* pData, int nIndex, ENDXFOPERATION enOperation)
{
	switch ( enOperation ) {
	case DXFADD:
		m_obDXFArray.Add(pData);
		break;
	case DXFINS:
		m_obDXFArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		RemoveSub(m_obDXFArray[nIndex]);
		m_obDXFArray.SetAt(nIndex, pData);
		break;
	}
	m_nDataCnt[DXFLINEDATA]		+= pData->GetObjectCount(0);
	m_nDataCnt[DXFARCDATA]		+= pData->GetObjectCount(1);
	m_nDataCnt[DXFELLIPSEDATA]	+= pData->GetObjectCount(2);
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation(LPDXFPARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	// 例外ｽﾛｰは上位でｷｬｯﾁ
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer);
	CDXFpoint*	pPoint = new CDXFpoint(lpArgv);
	DataOperation(pPoint);
}

void CDXFDoc::DataOperation(LPDXFLARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer);
	CDXFline*	pLine = new CDXFline(lpArgv);
	DataOperation(pLine);
}

void CDXFDoc::DataOperation(LPDXFCARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer);
	CDXFcircle*	pCircle = new CDXFcircle(lpArgv);
	DataOperation(pCircle);
}

void CDXFDoc::DataOperation(LPDXFAARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer);
	CDXFarc*	pArc = new CDXFarc(lpArgv);
	DataOperation(pArc);
}

void CDXFDoc::DataOperation(LPDXFEARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer);
	CDXFellipse*	pEllipse = new CDXFellipse(lpArgv);
	DataOperation(pEllipse);
}

void CDXFDoc::DataOperation(LPDXFTARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer);
	CDXFtext*	pText = new CDXFtext(lpArgv);
	DataOperation(pText);
}

void CDXFDoc::DataOperation_STR(CDXFline* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obStartArray.Add(pData);
		break;
	case DXFINS:
		m_obStartArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		delete	m_obStartArray[nIndex];
		m_obStartArray.SetAt(nIndex, pData);
		break;
	}
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation_STR(CDXFtext* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obStartTextArray.Add(pData);
		break;
	case DXFINS:
		m_obStartTextArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		delete	m_obStartTextArray[nIndex];
		m_obStartTextArray.SetAt(nIndex, pData);
		break;
	}
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation_STR(CDXFpolyline* pData, int nIndex, ENDXFOPERATION enOperation)
{
	switch ( enOperation ) {
	case DXFADD:
		m_obStartArray.Add(pData);
		break;
	case DXFINS:
		m_obStartArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		delete	m_obStartArray[nIndex];
		m_obStartArray.SetAt(nIndex, pData);
		break;
	}
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation_STR(LPDXFLARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer, DXFSTRLAYER);
	CDXFline*	pLine = new CDXFline(lpArgv);
	DataOperation_STR(pLine);
}

void CDXFDoc::DataOperation_STR(LPDXFTARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer, DXFSTRLAYER);
	CDXFtext*	pText = new CDXFtext(lpArgv);
	DataOperation_STR(pText);
}

void CDXFDoc::DataOperation_MOV(CDXFline* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obMoveArray.Add(pData);
		break;
	case DXFINS:
		m_obMoveArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		delete	m_obMoveArray[nIndex];
		m_obMoveArray.SetAt(nIndex, pData);
		break;
	}
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation_MOV(CDXFtext* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obMoveTextArray.Add(pData);
		break;
	case DXFINS:
		m_obMoveTextArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		delete	m_obMoveTextArray[nIndex];
		m_obMoveTextArray.SetAt(nIndex, pData);
		break;
	}
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation_MOV(CDXFpolyline* pData, int nIndex, ENDXFOPERATION enOperation)
{
	switch ( enOperation ) {
	case DXFADD:
		m_obMoveArray.Add(pData);
		break;
	case DXFINS:
		m_obMoveArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		delete	m_obMoveArray[nIndex];
		m_obMoveArray.SetAt(nIndex, pData);
		break;
	}
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation_MOV(LPDXFLARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer, DXFMOVLAYER);
	CDXFline*	pLine = new CDXFline(lpArgv);
	DataOperation_MOV(pLine);
}

void CDXFDoc::DataOperation_MOV(LPDXFTARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer, DXFMOVLAYER);
	CDXFtext*	pText = new CDXFtext(lpArgv);
	DataOperation_MOV(pText);
}

void CDXFDoc::DataOperation_COM(CDXFtext* pData, int nIndex, ENDXFOPERATION enOperation)
{
	ASSERT( pData );
	switch ( enOperation ) {
	case DXFADD:
		m_obCommentArray.Add(pData);
		break;
	case DXFINS:
		m_obCommentArray.InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		delete	m_obCommentArray[nIndex];
		m_obCommentArray.SetAt(nIndex, pData);
		break;
	}
	SetMaxRect(pData);
}

void CDXFDoc::DataOperation_COM(LPDXFTARGV lpArgv, int nIndex, ENDXFOPERATION enOperation)
{
	lpArgv->pLayer = m_mpLayer.AddLayerMap(lpArgv->strLayer, DXFCOMLAYER);
	CDXFtext*	pText = new CDXFtext(lpArgv);
	DataOperation_COM(pText);
}

void CDXFDoc::RemoveAt(int nIndex, int nCnt)
{
	int	nLoop = min(m_obDXFArray.GetSize(), nIndex+nCnt);
	for ( int i=nIndex; i<nLoop; i++ )
		RemoveSub(m_obDXFArray[i]);	// ﾃﾞｰﾀの削除と関連ﾃﾞｰﾀの減算
	m_obDXFArray.RemoveAt(nIndex, nCnt);
}

void CDXFDoc::RemoveAtText(int nIndex, int nCnt)
{
	int	nLoop = min(m_obDXFTextArray.GetSize(), nIndex+nCnt);
	for ( int i=nIndex; i<nLoop; i++ )
		RemoveSub(m_obDXFTextArray[i]);	// ﾃﾞｰﾀの削除と関連ﾃﾞｰﾀの減算
	m_obDXFTextArray.RemoveAt(nIndex, nCnt);
}

void CDXFDoc::RemoveAt_STR(int nIndex, int nCnt)
{
	int	nLoop = min(m_obStartArray.GetSize(), nIndex+nCnt);
	for ( int i=nIndex; i<nLoop; i++ )
		delete	m_obStartArray[i];
	m_obStartArray.RemoveAt(nIndex, nCnt);
}

void CDXFDoc::RemoveAtText_STR(int nIndex, int nCnt)
{
	int	nLoop = min(m_obStartTextArray.GetSize(), nIndex+nCnt);
	for ( int i=nIndex; i<nLoop; i++ )
		delete	m_obStartTextArray[i];
	m_obStartTextArray.RemoveAt(nIndex, nCnt);
}

void CDXFDoc::RemoveAt_MOV(int nIndex, int nCnt)
{
	int	nLoop = min(m_obMoveArray.GetSize(), nIndex+nCnt);
	for ( int i=nIndex; i<nLoop; i++ )
		delete	m_obMoveArray[i];
	m_obMoveArray.RemoveAt(nIndex, nCnt);
}

void CDXFDoc::RemoveAtText_MOV(int nIndex, int nCnt)
{
	int	nLoop = min(m_obMoveTextArray.GetSize(), nIndex+nCnt);
	for ( int i=nIndex; i<nLoop; i++ )
		delete	m_obMoveTextArray[i];
	m_obMoveTextArray.RemoveAt(nIndex, nCnt);
}

void CDXFDoc::RemoveAt_COM(int nIndex, int nCnt)
{
	int	nLoop = min(m_obCommentArray.GetSize(), nIndex+nCnt);
	for ( int i=nIndex; i<nLoop; i++ )
		delete	m_obCommentArray[i];
	m_obCommentArray.RemoveAt(nIndex, nCnt);
}

void CDXFDoc::AllChangeFactor(double f)
{
	int		i;
	f *= LOMETRICFACTOR;

	for ( i=0; i<m_obDXFArray.GetSize(); i++ )
		m_obDXFArray[i]->DrawTuning(f);
	for ( i=0; i<m_obDXFTextArray.GetSize(); i++ )
		m_obDXFTextArray[i]->DrawTuning(f);
	for ( i=0; i<m_obStartArray.GetSize(); i++ )
		m_obStartArray[i]->DrawTuning(f);
	for ( i=0; i<m_obStartTextArray.GetSize(); i++ )
		m_obStartTextArray[i]->DrawTuning(f);
	for ( i=0; i<m_obMoveArray.GetSize(); i++ )
		m_obMoveArray[i]->DrawTuning(f);
	for ( i=0; i<m_obMoveTextArray.GetSize(); i++ )
		m_obMoveTextArray[i]->DrawTuning(f);
	for ( i=0; i<m_obCommentArray.GetSize(); i++ )
		m_obCommentArray[i]->DrawTuning(f);

	for ( i=0; i<SIZEOF(m_pCircle); i++ ) {
		if ( m_pCircle[i] )
			m_pCircle[i]->DrawTuning(f);
	}
}

void CDXFDoc::SetCutterOrigin(const CPointD& pt, double r, BOOL bRedraw)
{
	if ( m_pCircle[DXFCIRCLEORG] ) {
		delete	m_pCircle[DXFCIRCLEORG];
		m_pCircle[DXFCIRCLEORG] = NULL;
	}

	try {
		m_pCircle[DXFCIRCLEORG] = new CDXFcircleEx(DXFORGDATA, NULL, pt, r);
	}
	catch (CMemoryException* e) {
		if ( m_pCircle[DXFCIRCLEORG] )
			delete	m_pCircle[DXFCIRCLEORG];
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		m_bReady = FALSE;
		return;
	}
	SetMaxRect(m_pCircle[DXFCIRCLEORG]);
	if ( bRedraw )
		UpdateAllViews(NULL, UAV_DXFORGUPDATE, (CObject *)m_pCircle[DXFCIRCLEORG]);
}

void CDXFDoc::SetStartOrigin(const CPointD& pt, double r, BOOL bRedraw)
{
	// 加工開始位置指示ﾚｲﾔの開始円だけ特別にここでﾚｲﾔ情報の登録を行う
	CString	strLayer( AfxGetNCVCApp()->GetDXFOption()->GetReadLayer(DXFSTRLAYER) );

	if ( m_pCircle[DXFCIRCLESTA] ) {
		delete	m_pCircle[DXFCIRCLESTA];
		m_pCircle[DXFCIRCLESTA] = NULL;
		m_mpLayer.DelLayerMap(strLayer);
	}

	try {
		m_pCircle[DXFCIRCLESTA] = new CDXFcircleEx(DXFSTADATA,
			m_mpLayer.AddLayerMap(strLayer, DXFSTRLAYER), pt, r);
	}
	catch (CMemoryException* e) {
		if ( m_pCircle[DXFCIRCLESTA] )
			delete	m_pCircle[DXFCIRCLESTA];
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return;
	}
	SetMaxRect(m_pCircle[DXFCIRCLESTA]);
	if ( bRedraw )
		UpdateAllViews(NULL, UAV_DXFORGUPDATE, (CObject *)m_pCircle[DXFCIRCLESTA]);
}

BOOL CDXFDoc::GetEditOrgPoint(LPCTSTR lpctStr, CPointD& pt)
{
	extern	LPCTSTR	gg_szCat;
	LPTSTR	lpszNum = NULL;
	LPTSTR	lpsztok;
	BOOL	bResult = TRUE;
	
	try {
		lpszNum = new TCHAR[lstrlen(lpctStr)+1];
		lpsztok = strtok(lstrcpy(lpszNum, lpctStr), gg_szCat);
		for ( int i=0; i<2 && lpsztok; i++ ) {
			switch ( i ) {
			case 0:
				pt.x = atof(lpsztok);	break;
			case 1:
				pt.y = atof(lpsztok);	break;
			}
			lpsztok = strtok(NULL, gg_szCat);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}

	if ( lpszNum )
		delete[]	lpszNum;

	return bResult;
}

double CDXFDoc::GetSelectViewPointGap
	(const CPointD& pt, const CRectD& rcView, CDXFdata** pDataResult)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SelectSwitchObject()", DBG_CYAN);
#endif
	CDXFdata*	pData;
	double		dGap, dGapMin = HUGE_VAL;
	CRectD		rc;

	*pDataResult = NULL;
	// 全てのｵﾌﾞｼﾞｪｸﾄと指定座標の距離を取得
	for ( int i=0; i<m_obDXFArray.GetSize(); i++ ) {
		pData = m_obDXFArray[i];
		// 現在の表示ｴﾘｱか
		if ( rc.IntersectRect(rcView, pData->GetMaxRect()) ) {
			// 表示ﾚｲﾔか否か
			if ( pData->GetLayerData()->IsViewLayer() ) {
				dGap = pData->GetSelectPointGap(pt);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					*pDataResult = pData;
				}
			}
		}
	}

	return dGapMin;
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc 診断

#ifdef _DEBUG
void CDXFDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CDXFDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}

void CDXFDoc::SerializeInfo(void)
{
	CMagaDbg	dbg("CDXFDoc::SerializeInfo()");

	dbg.printf("m_rcMax left =%f right =%f", m_rcMax.left, m_rcMax.right);
	dbg.printf("        top  =%f bottom=%f", m_rcMax.top, m_rcMax.bottom);
	dbg.printf("        Width=%f Height=%f", m_rcMax.Width(), m_rcMax.Height());
	dbg.printf("        CenterPoint() x=%f y=%f", m_rcMax.CenterPoint().x, m_rcMax.CenterPoint().y);
	if ( m_pCircle[DXFCIRCLEORG] ) {
		dbg.printf("m_ptOrg x=%f y=%f R=%f",
			m_pCircle[DXFCIRCLEORG]->GetCenter().x, m_pCircle[DXFCIRCLEORG]->GetCenter().y,
			m_pCircle[DXFCIRCLEORG]->GetR() );
	}
	else {
		dbg.printf("m_ptOrg ???");
	}
	if ( m_pCircle[DXFCIRCLESTA] ) {
		dbg.printf("m_ptStart x=%f y=%f",
			m_pCircle[DXFCIRCLESTA]->GetCenter().x, m_pCircle[DXFCIRCLESTA]->GetCenter().y);
	}
	CLayerData*	pLayer;
	CString		strLayer;
	for ( POSITION pos = m_mpLayer.GetStartPosition(); pos; ) {
		m_mpLayer.GetNextAssoc(pos, strLayer, pLayer);
		dbg.printf("LayerName=%s Cnt=%d", pLayer->GetStrLayer(), pLayer->GetCount());
	}
	dbg.printf("m_obDXFArray.GetSize()=%d", m_obDXFArray.GetSize());
	dbg.printf("m_obStartArray.GetSize()=%d", m_obStartArray.GetSize());
	dbg.printf("m_obMoveArray.GetSize()=%d", m_obMoveArray.GetSize());
	dbg.printf("m_obCommentArray.GetSize()=%d", m_obCommentArray.GetSize());
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc クラスのオーバライド関数

BOOL CDXFDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	BOOL	bResult;
	PFNNCVCSERIALIZEFUNC	pSerialFunc = AfxGetNCVCApp()->GetSerializeFunc();

	if ( pSerialFunc ) {
		// ｱﾄﾞｲﾝｼﾘｱﾙ関数を保存．ﾌｧｲﾙ変更通知などに使用
		m_pfnSerialFunc = pSerialFunc;	// DocBase.h
		// ｱﾄﾞｲﾝのｼﾘｱﾙ関数を呼び出し
		bResult = (*pSerialFunc)(this, lpszPathName);
		// ｼﾘｱﾙ関数の初期化
		AfxGetNCVCApp()->SetSerializeFunc((PFNNCVCSERIALIZEFUNC)NULL);
	}
	else {
		// 通常のｼﾘｱﾙ関数呼び出し
		bResult = CDocument::OnOpenDocument(lpszPathName);
	}

	if ( bResult ) {
#ifdef _DEBUG
		SerializeInfo();
#endif
		// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
		POSITION	pos = GetFirstViewPosition();
		CFrameWnd*	pFrame = GetNextView(pos)->GetParentFrame();
		CDocBase::OnOpenDocument(lpszPathName, pFrame);
		// 切削ﾃﾞｰﾀがないとき
		if ( m_obDXFArray.GetSize() < 1 ) {
			AfxMessageBox(IDS_ERR_DXFDATACUT, MB_OK|MB_ICONEXCLAMATION);
		}
		// 原点ﾃﾞｰﾀがないとき(矩形の上下に注意)
		else if ( !m_pCircle[DXFCIRCLEORG] ) {
			CPointD	pt(HUGE_VAL);
			switch ( AfxGetNCVCApp()->GetDXFOption()->GetDxfFlag(DXFOPT_ORGTYPE) ) {
			case 1:	// 右上
				pt = m_rcMax.BottomRight();
				break;
			case 2:	// 右下
				pt.x = m_rcMax.right;	pt.y = m_rcMax.top;
				break;
			case 3:	// 左上
				pt.x = m_rcMax.left;	pt.y = m_rcMax.bottom;
				break;
			case 4:	// 左下
				pt = m_rcMax.TopLeft();
				break;
			case 5:	// 中央
				pt = m_rcMax.CenterPoint();
				break;
			default:
				AfxMessageBox(IDS_ERR_DXFDATAORG, MB_OK|MB_ICONEXCLAMATION);
				break;
			}
			if ( pt != HUGE_VAL ) {
				SetCutterOrigin(pt, dOrgR);
				m_bReady = TRUE;	// 生成OK!
			}
		}
		else {
			m_ptOrgOrig = m_pCircle[DXFCIRCLEORG]->GetCenter();	// ﾌｧｲﾙからのｵﾘｼﾞﾅﾙ原点を保存
			m_bReady = TRUE;	// 生成OK!
		}
	}

	// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ初期化
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);

	return bResult;
}

BOOL CDXFDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの終了
	CDocBase::OnCloseDocument();

	if ( GetPathName().CompareNoCase(lpszPathName) != 0 ) {
		CDXFDoc* pDoc = AfxGetNCVCApp()->GetAlreadyDXFDocument(lpszPathName);
		ASSERT( pDoc != this );
		if ( pDoc )
			pDoc->OnCloseDocument();	// 既に開いているﾄﾞｷｭﾒﾝﾄを閉じる
	}

	// 保存処理
	BOOL bResult = CDocument::OnSaveDocument(lpszPathName);

	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
	if ( bResult ) {
		POSITION	pos = GetFirstViewPosition();
		CFrameWnd*	pFrame = GetNextView(pos)->GetParentFrame();
		CDocBase::OnOpenDocument(lpszPathName, pFrame);
	}
	return bResult;
}

void CDXFDoc::OnCloseDocument() 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFDoc::OnCloseDocument()\nStart", DBG_BLUE);
#endif
	// ﾛｯｸｱﾄﾞｲﾝのﾁｪｯｸ
	if ( !IsLockThread() ) {	// CDocBase
#ifdef _DEBUG
		dbg.printf("AddinLock FALSE");
#endif
		return;
	}
	// 処理中のｽﾚｯﾄﾞを中断させる
	CDocBase::OnCloseDocument();	// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ
	m_bThread = FALSE;
	m_csRestoreCircleType.Lock();
	m_csRestoreCircleType.Unlock();
#ifdef _DEBUG
	dbg.printf("m_csRestoreCircleType Unlock OK");
#endif

	CDocument::OnCloseDocument();
}

void CDXFDoc::ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault) 
{
	if ( e->IsKindOf(RUNTIME_CLASS(CUserException)) ) {
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		return;	// 標準ｴﾗｰﾒｯｾｰｼﾞを出さない
	}
	CDocument::ReportSaveLoadException(lpszPathName, e, bSaving, nIDPDefault);
}

void CDXFDoc::UpdateFrameCounts()
{
	// ﾌﾚｰﾑの数を抑制することでｳｨﾝﾄﾞｳﾀｲﾄﾙの「:1」を付与しないようにする
	CFrameWnd*	pFrame;
	for ( POSITION pos = GetFirstViewPosition(); pos; ) {
		pFrame = GetNextView(pos)->GetParentFrame();
		if ( pFrame )
			pFrame->m_nWindow = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc シリアライズ

void CDXFDoc::Serialize(CArchive& ar)
{
	// DxfSetupReloadのﾁｪｯｸOFF
	m_bReload = FALSE;

	int			i;
	POSITION	pos;
	CString		strLayer;
	CLayerData*	pLayer;

	// ﾍｯﾀﾞｰ情報
	CCAMHead	cam;
	cam.Serialize(ar);
	// ﾚｲﾔ情報
	m_obLayer.RemoveAll();

	if ( ar.IsStoring() ) {
		// ﾚｲﾔ情報 -> ｼﾘｱﾙ番号を付与して通常配列で保存
		for ( i=0, pos=m_mpLayer.GetStartPosition(); pos; i++ ) {
			m_mpLayer.GetNextAssoc(pos, strLayer, pLayer);
			pLayer->SetSerial(i);
			m_obLayer.Add(pLayer);
		}
		m_obLayer.Serialize(ar);
		// 原点
		for ( i=0; i<SIZEOF(m_pCircle); i++ ) {
			if ( m_pCircle[i] ) {
				ar << (BYTE)1;
				ar.WriteObject(m_pCircle[i]);
			}
			else
				ar << (BYTE)0;
		}
		ar << m_ptOrgOrig.x << m_ptOrgOrig.y;
		// 表示状況(ちょぴり危険)
		UpdateAllViews(NULL, UAV_DXFGETVIEWINFO, (CObject *)&m_dxfViewInfo);
		ar << m_dxfViewInfo.ptOrg.x << m_dxfViewInfo.ptOrg.y << m_dxfViewInfo.dFactor;
	}
	else {
		// ﾚｲﾔ情報 -> 通常配列で読み込んでﾏｯﾋﾟﾝｸﾞ
		// 各ｵﾌﾞｼﾞｪｸﾄは m_obLayer の位置から CLayerData* を得る
		m_obLayer.Serialize(ar);
		for ( i=0; i<m_obLayer.GetSize(); i++ ) {
			pLayer = m_obLayer[i];
			m_mpLayer.SetAt(pLayer->GetStrLayer(), pLayer);
		}
		// 原点
		BYTE	bExist;
		for ( i=0; i<SIZEOF(m_pCircle); i++ ) {
			ar >> bExist;
			if ( bExist ) {
				m_pCircle[i] = (CDXFcircleEx *)ar.ReadObject(RUNTIME_CLASS(CDXFcircleEx));
				SetMaxRect(m_pCircle[i]);
			}
		}
		ar >> m_ptOrgOrig.x >> m_ptOrgOrig.y;
		// 表示状況
		ar >> m_dxfViewInfo.ptOrg.x >> m_dxfViewInfo.ptOrg.y >> m_dxfViewInfo.dFactor;
	}
	// 各種CADﾃﾞｰﾀ
	m_obDXFArray.Serialize(ar);			// DXF切削ﾃﾞｰﾀ
	m_obDXFTextArray.Serialize(ar);		// 　〃　(CDXFtext only)
	m_obStartArray.Serialize(ar);		// DXF加工開始位置指示ﾃﾞｰﾀ
	m_obStartTextArray.Serialize(ar);	// 　〃　(CDXFtext only)
	m_obMoveArray.Serialize(ar);		// DXF移動指示ﾃﾞｰﾀ
	m_obMoveTextArray.Serialize(ar);	// 　〃　(CDXFtext only)
	m_obCommentArray.Serialize(ar);		// ｺﾒﾝﾄ文字ﾃﾞｰﾀ(CDXFtext only)
	m_obLayer.RemoveAll();

	// NC生成ﾌｧｲﾙ名
	if ( ar.IsStoring() ) {
		ar << m_strNCFileName;
		return;		//保存はここまで
	}

	ar >> m_strNCFileName;
	// ﾌｧｲﾙﾊﾟｽのﾁｪｯｸ
	CString	strPath, strFile;
	if ( !m_strNCFileName.IsEmpty() ) {
		::Path_Name_From_FullPath(m_strNCFileName, strPath, strFile);
		if ( !::PathFileExists(strPath) )
			m_strNCFileName.Empty();
	}

	// ﾃﾞｰﾀ読み込み後の処理
	CDXFdata*	pData;
	for ( i=0; i<m_obDXFArray.GetSize(); i++ ) {
		pData = m_obDXFArray[i];
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			m_nDataCnt[DXFPOINTDATA]++;
			break;
		case DXFLINEDATA:
			m_nDataCnt[DXFLINEDATA]++;
			break;
		case DXFCIRCLEDATA:
			m_nDataCnt[DXFCIRCLEDATA]++;
			break;
		case DXFARCDATA:
			m_nDataCnt[DXFARCDATA]++;
			break;
		case DXFELLIPSEDATA:
			m_nDataCnt[DXFELLIPSEDATA]++;
			break;
		case DXFPOLYDATA:
			m_nDataCnt[DXFLINEDATA]		+= ((CDXFpolyline *)pData)->GetObjectCount(0);
			m_nDataCnt[DXFARCDATA]		+= ((CDXFpolyline *)pData)->GetObjectCount(1);
			m_nDataCnt[DXFELLIPSEDATA]	+= ((CDXFpolyline *)pData)->GetObjectCount(2);
			break;
		}
		SetMaxRect(pData);
	}
	for ( i=0; i<m_obDXFTextArray.GetSize(); i++ )
		SetMaxRect( m_obDXFTextArray[i] );
	for ( i=0; i<m_obStartArray.GetSize(); i++ )
		SetMaxRect( m_obStartArray[i] );
	for ( i=0; i<m_obMoveArray.GetSize(); i++ )
		SetMaxRect( m_obMoveArray[i] );
	for ( i=0; i<m_obStartTextArray.GetSize(); i++ )
		SetMaxRect( m_obStartTextArray[i] );
	for ( i=0; i<m_obMoveTextArray.GetSize(); i++ )
		SetMaxRect( m_obMoveTextArray[i] );
	for ( i=0; i<m_obCommentArray.GetSize(); i++ )
		SetMaxRect( m_obCommentArray[i] );
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc コマンド

void CDXFDoc::OnEditOrigin() 
{
	DWORD	dwControl = 0;
	if ( !m_pCircle[DXFCIRCLEORG] )
		dwControl |= EDITORG_NUMERIC;
	if ( m_ptOrgOrig == HUGE_VAL )
		dwControl |= EDITORG_ORIGINAL;
	CDxfEditOrgDlg	dlg( dwControl );
	if ( dlg.DoModal() != IDOK )
		return;

	// 原点移動処理
	CPointD	pt(HUGE_VAL), ptOffset;
	switch ( dlg.m_nSelect ) {
	case 0:		// 数値移動
		if ( GetEditOrgPoint(dlg.m_strNumeric, ptOffset) )
			pt = m_pCircle[DXFCIRCLEORG]->GetCenter() + ptOffset;
		break;
	case 1:		// 矩形移動
		switch ( dlg.m_nRectType ) {
		case 0:	// 右上
			pt = m_rcMax.BottomRight();
			break;
		case 1:	// 右下
			pt.x = m_rcMax.right;	pt.y = m_rcMax.top;
			break;
		case 2:	// 左上
			pt.x = m_rcMax.left;	pt.y = m_rcMax.bottom;
			break;
		case 3:	// 左下
			pt = m_rcMax.TopLeft();
			break;
		case 4:	// 中央
			pt = m_rcMax.CenterPoint();
			break;
		}
		break;
	case 2:		// ｵﾘｼﾞﾅﾙ原点に戻す
		pt = m_ptOrgOrig;
		break;
	}

	if ( pt != HUGE_VAL ) {
		SetCutterOrigin(pt, dOrgR, TRUE);
		m_bReady = TRUE;	// 生成OK!
	}
}

void CDXFDoc::OnEditShape() 
{
	// 状況案内ﾀﾞｲｱﾛｸﾞ(検索ｽﾚｯﾄﾞ生成)
	CThreadDlg	dlgThread(ID_EDIT_DXFSHAPE, this);
	if ( dlgThread.DoModal() == IDOK ) {
		m_bShape = TRUE;
		// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳを広げる + DXFView のﾌｨｯﾄﾒｯｾｰｼﾞ送信
		((CDXFChild *)(AfxGetNCVCMainWnd()->MDIGetActive()))->ShowShapeView();
		// DXFShapeView更新
		UpdateAllViews(NULL, UAV_DXFSHAPEUPDATE);
		// 加工指示のﾃﾞﾌｫﾙﾄ
		m_nShapePattern = ID_EDIT_SHAPE_VEC;
	}
}

void CDXFDoc::OnEditAutoShape() 
{
	CDxfAutoWorkingDlg	dlg;
	if ( dlg.DoModal() == IDOK ) {
		// 現在登録されている加工指示を削除
		UpdateAllViews(NULL, UAV_DXFDELWORKING);
		// 自動加工指示
		CThreadDlg	dlgThread(ID_EDIT_SHAPE_AUTO, this, (LPVOID)dlg.m_nAuto);
		if ( dlgThread.DoModal() == IDOK )
			UpdateAllViews(NULL, UAV_DXFAUTOWORKING);
	}
}

void CDXFDoc::OnUpdateEditShape(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_bShape);
}

void CDXFDoc::OnUpdateEditShaping(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bShape);
}

void CDXFDoc::OnUpdateFileDXF2NCD(CCmdUI* pCmdUI) 
{
	BOOL	bEnable = TRUE;
	// ｴﾗｰﾃﾞｰﾀの場合はNC変換ﾒﾆｭｰを無効にする
	if ( !m_bReady || !m_pCircle[DXFCIRCLEORG] )
		bEnable = FALSE;
	else {
		// 単一ﾚｲﾔの場合は拡張生成をoffにする
		int		nCnt = 0;
		CString	strLayer;
		CLayerData*	pLayer;
		for ( POSITION pos = m_mpLayer.GetStartPosition(); pos; ) {
			m_mpLayer.GetNextAssoc(pos, strLayer, pLayer);
			if ( pLayer->IsCutType() )
				nCnt++;
		}
		if ( pCmdUI->m_nID!=ID_FILE_DXF2NCD && nCnt<=1 )
			bEnable = FALSE;
	}

	pCmdUI->Enable(bEnable);
}

void CDXFDoc::OnFileDXF2NCD(UINT nID) 
{
	BOOL	bNCView;

	switch ( nID ) {
	case ID_FILE_DXF2NCD:		// 単一条件
	{
		CMakeNCDlg		dlg(this);
		if ( dlg.DoModal() != IDOK )
			return;
		m_strNCFileName	= dlg.m_strNCFileName;
		bNCView = dlg.m_bNCView;
	}
		break;

	case ID_FILE_DXF2NCD_EX1:	// ﾚｲﾔごとの複数条件
	{
		CMakeNCDlgEx1	dlg(this);
		if ( dlg.DoModal() != IDOK )
			return;
		m_strNCFileName	= dlg.m_strNCFileName;
		bNCView = dlg.m_bNCView;
	}
		break;

	case ID_FILE_DXF2NCD_EX2:	// ﾚｲﾔごとのZ座標
	{
		CMakeNCDlgEx2	dlg(this);
		if ( dlg.DoModal() != IDOK )
			return;
		m_strNCFileName	= dlg.m_strNCFileName;
		bNCView = dlg.m_bNCView;
	}
		break;

	default:	// 保険
		return;
	}

	CNCDoc*		pDoc;
	CLayerData*	pLayer;
	POSITION	pos;
	CString		strLayer;
	BOOL		bAllOut;
	// すでに開いているﾄﾞｷｭﾒﾝﾄなら閉じるｱﾅｳﾝｽ
	if ( nID == ID_FILE_DXF2NCD ) {
		pDoc = AfxGetNCVCApp()->GetAlreadyNCDocument(m_strNCFileName);
		if ( pDoc )
			pDoc->OnCloseDocument();
	}
	else {
		bAllOut = FALSE;
		for ( pos = m_mpLayer.GetStartPosition(); pos; ) {
			m_mpLayer.GetNextAssoc(pos, strLayer, pLayer);
			if ( pLayer->IsCutType() && pLayer->IsViewLayer() ) {
				if ( pLayer->IsPartOut() ) {
					pDoc = AfxGetNCVCApp()->GetAlreadyNCDocument(pLayer->GetNCFile());
					if ( pDoc )
						pDoc->OnCloseDocument();
				}
				else
					bAllOut = TRUE;
			}
		}
		if ( bAllOut ) {
			pDoc = AfxGetNCVCApp()->GetAlreadyNCDocument(m_strNCFileName);
			if ( pDoc )
				pDoc->OnCloseDocument();
		}
	}

	// CDXFCircleの穴加工対象ﾃﾞｰﾀを元に戻すｽﾚｯﾄﾞの終了待ち
	m_csRestoreCircleType.Lock();
	m_csRestoreCircleType.Unlock();

	// 状況案内ﾀﾞｲｱﾛｸﾞ(変換ｽﾚｯﾄﾞ生成)
	CThreadDlg	dlgThread(ID_FILE_DXF2NCD, this, (LPVOID)nID);
	int nRet = dlgThread.DoModal();

	// CDXFCircleの穴加工対象ﾃﾞｰﾀを元に戻す
	AfxBeginThread(RestoreCircleTypeThread, this,
		/*THREAD_PRIORITY_LOWEST*/
		THREAD_PRIORITY_IDLE
		/*THREAD_PRIORITY_BELOW_NORMAL*/);

	if ( nRet!=IDOK || !bNCView )
		return;

	// NC生成後のﾃﾞｰﾀを開く
	if ( nID == ID_FILE_DXF2NCD ) {
		AfxGetNCVCApp()->OpenDocumentFile(m_strNCFileName);
	}
	else {
		bAllOut = FALSE;	// m_strNCFileName も開くかどうか
		for ( pos = m_mpLayer.GetStartPosition(); pos; ) {
			m_mpLayer.GetNextAssoc(pos, strLayer, pLayer);
			if ( pLayer->IsCutType() && pLayer->IsViewLayer() && pLayer->GetLayerFlags()==0 ) {
				if ( pLayer->IsPartOut() )
					AfxGetNCVCApp()->OpenDocumentFile(pLayer->GetNCFile());
				else
					bAllOut = TRUE;
			}
		}
		if ( bAllOut )
			AfxGetNCVCApp()->OpenDocumentFile(m_strNCFileName);
	}
}

void CDXFDoc::OnFileSave() 
{
	CString	strExt( AfxGetNCVCApp()->GetDocExtString(TYPE_DXF) );
	// 拡張子が .cam か否か
	if ( strExt.CompareNoCase(GetPathName().Right(4)) == 0 )
		OnSaveDocument(GetPathName());
	else
		OnFileSaveAs();		// 名前を付けて保存
}

void CDXFDoc::OnFileSaveAs() 
{
 	extern	LPCTSTR	gg_szWild;		// "*.";
	// ﾌﾟﾚｰｽﾊﾞｰ付きﾌｧｲﾙ保存ﾀﾞｲｱﾛｸﾞを出すためにｵｰﾊﾞｰﾗｲﾄﾞ
	CString	strExt, strFilter, strPath, strFile;
	VERIFY(strExt.LoadString(IDS_CAM_FILTER));
	strExt = strExt.Left(3);
	strPath = gg_szWild + strExt;	// *.cam
	strFilter.Format(IDS_CAM_FILTER, strPath, strPath);
	::Path_Name_From_FullPath(GetPathName(), strPath, strFile, FALSE);
	strFile = strFile+ '.' + strExt;
	if ( ::NC_FileDlgCommon(-1, strFilter, strFile, strPath, FALSE,
				OFN_HIDEREADONLY|OFN_PATHMUSTEXIST) == IDOK ) {
		if ( OnSaveDocument(strFile) )
			SetPathName(strFile);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc サブスレッド

UINT CDXFDoc::RestoreCircleTypeThread(LPVOID pParam)
{
	CDXFDoc* pDoc = (CDXFDoc *)pParam;
	pDoc->m_csRestoreCircleType.Lock();		// ｽﾚｯﾄﾞ終了までﾛｯｸ
#ifdef _DEBUG
	CMagaDbg	dbg("RestoreCircleFlagThread()\nStart", TRUE, DBG_RED);
#endif
	ENDXFTYPE	enType;
	for ( int i=0; i<pDoc->m_obDXFArray.GetSize() && pDoc->m_bThread; i++ ) {
		enType = pDoc->m_obDXFArray[i]->GetType();
		if ( enType != pDoc->m_obDXFArray[i]->GetMakeType() )
			pDoc->m_obDXFArray[i]->ChangeMakeType(enType);
	}
	pDoc->m_csRestoreCircleType.Unlock();

	return 0;
}
