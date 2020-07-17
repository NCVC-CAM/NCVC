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
#include "DxfEditOrgDlg.h"
#include "DxfAutoWorkingDlg.h"
#include "NCDoc.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

// 原点を示すﾃﾞﾌｫﾙﾄの半径
static	const	double	ORGRADIUS = 10.0;
// ﾚｲﾔ情報の保存
static	LPCTSTR	g_szLayerToInitComment[] = {
	"##\n",
	"## NCVC:ﾚｲﾔ名と切削条件ﾌｧｲﾙの関係情報ﾌｧｲﾙ\n",
	"## ﾚｲﾔ名，切削対象ﾌﾗｸﾞ(1:対象 0:除外)，切削条件ﾌｧｲﾙ,\n",
	"##\t強制最深Z, 強制最深Zを穴加工にも適用(1:する 0:しない),\n",
	"##\t個別出力(1:する 0:しない), 個別出力ﾌｧｲﾙ名,\n"
	"##\t出力ｼｰｹﾝｽ, 出力ｺﾒﾝﾄ, 出力ｺｰﾄﾞ\n"
};
// ﾚｲﾔの切削順
static	int		LayerCompareFunc_CutNo(CLayerData*, CLayerData*);
// ﾚｲﾔ名の並べ替え
static	int		LayerCompareFunc_Name(CLayerData*, CLayerData*);
// 結合時の面積順並び替え
static	int		BindAreaCompareFunc(LPCADBINDINFO, LPCADBINDINFO);
//
#define	IsBindMode()	m_bDocFlg[DXFDOC_BIND]

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc

IMPLEMENT_DYNCREATE(CDXFDoc, CDocument)

BEGIN_MESSAGE_MAP(CDXFDoc, CDocument)
	//{{AFX_MSG_MAP(CDXFDoc)
	ON_COMMAND(ID_FILE_SAVE, &CDXFDoc::OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, &CDXFDoc::OnFileSaveAs)
	ON_COMMAND(ID_EDIT_DXFORG, &CDXFDoc::OnEditOrigin)
	ON_COMMAND(ID_EDIT_DXFSHAPE, &CDXFDoc::OnEditShape)
	ON_COMMAND(ID_EDIT_SHAPE_AUTO, &CDXFDoc::OnEditAutoShape)
	ON_COMMAND(ID_EDIT_SHAPE_STRICTOFFSET, &CDXFDoc::OnEditStrictOffset)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DXFSHAPE, &CDXFDoc::OnUpdateEditShape)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SORTSHAPE, &CDXFDoc::OnUpdateEditShaping)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHAPE_AUTO, &CDXFDoc::OnUpdateEditShaping)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHAPE_STRICTOFFSET, &CDXFDoc::OnUpdateEditShaping)
	//}}AFX_MSG_MAP
	// NC生成ﾒﾆｭｰ
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_DXF2NCD, ID_FILE_DXF2NCD_WIRE, &CDXFDoc::OnUpdateFileDXF2NCD)
	ON_COMMAND_RANGE(ID_FILE_DXF2NCD, ID_FILE_DXF2NCD_WIRE, &CDXFDoc::OnFileDXF2NCD)
	// 形状加工指示
	ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_SHAPE_SEL, ID_EDIT_SHAPE_POC, &CDXFDoc::OnUpdateShapePattern)
	ON_COMMAND_RANGE(ID_EDIT_SHAPE_SEL, ID_EDIT_SHAPE_POC, &CDXFDoc::OnShapePattern)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc クラスの構築/消滅

CDXFDoc::CDXFDoc()
{
	int		i;

	// ﾌﾗｸﾞ初期設定
	m_bDocFlg.set(DXFDOC_RELOAD);
	m_bDocFlg.set(DXFDOC_THREAD);
	m_nShapeProcessID = 0;
	// ﾃﾞｰﾀ数初期化
	for ( i=0; i<SIZEOF(m_nDataCnt); i++ )
		m_nDataCnt[i] = 0;
	for ( i=0; i<SIZEOF(m_nLayerDataCnt); i++ )
		m_nLayerDataCnt[i] = 0;
	m_pCircle = NULL;
	for ( i=0; i<SIZEOF(m_pLatheLine); i++ )
		m_pLatheLine[i] = NULL;
	// ｵﾌﾞｼﾞｪｸﾄ矩形の初期化
	m_rcMax.SetRectMinimum();
	// 増分割り当てサイズ
	m_obLayer.SetSize(0, 64);
}

CDXFDoc::~CDXFDoc()
{
	int		i;

	if ( m_pCircle )
		delete	m_pCircle;
	for ( i=0; i<SIZEOF(m_pLatheLine); i++ ) {
		if ( m_pLatheLine[i] )
			delete	m_pLatheLine[i];
	}
	for ( i=0; i<m_obLayer.GetSize(); i++ )
		delete	m_obLayer[i];
	for ( i=0; i<m_bindInfo.GetSize(); i++ ) {
		m_bindInfo[i]->pParent->DestroyWindow();
		delete	m_bindInfo[i]->pDoc;
		delete	m_bindInfo[i];
	}
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

BOOL CDXFDoc::RouteCmdToAllViews
	(CView* pActiveView, UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	CView*	pView;
	for ( POSITION pos=GetFirstViewPosition(); pos; ) {
		pView = GetNextView(pos);
		if ( pView != pActiveView ) {
			if ( pView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
				return TRUE;
		}
	}
	return FALSE;
}

void CDXFDoc::RemoveData(CLayerData* pLayer, int nIndex)
{
	CDXFdata*	pData = pLayer->GetDxfData(nIndex);
	ENDXFTYPE	enType = pData->GetType();
	if ( DXFPOINTDATA<=enType && enType<=DXFELLIPSEDATA )
		m_nDataCnt[enType]--;
	else if ( enType == DXFPOLYDATA ) {
		CDXFpolyline* pPoly = static_cast<CDXFpolyline*>(pData);
		m_nDataCnt[DXFLINEDATA]		-= pPoly->GetObjectCount(0);
		m_nDataCnt[DXFARCDATA]		-= pPoly->GetObjectCount(1);
		m_nDataCnt[DXFELLIPSEDATA]	-= pPoly->GetObjectCount(2);
	}
	m_nLayerDataCnt[pLayer->GetLayerType()-1]--;

	if ( enType == DXFTEXTDATA )
		pLayer->RemoveDataText(nIndex);
	else
		pLayer->RemoveData(nIndex);
	delete	pData;
	DelLayerMap(pLayer);
}

void CDXFDoc::DataOperation
	(CDXFdata* pData, ENDXFOPERATION enOperation/*=DXFADD*/, int nIndex/*=-1*/)
{
	CLayerData*	pLayer = pData->GetParentLayer();
	ASSERT(pLayer);
	int	nLayerType = pLayer->GetLayerType();
	ASSERT(nLayerType>=DXFCAMLAYER && nLayerType<=DXFCOMLAYER);
	if ( enOperation == DXFMOD )
		RemoveData(pLayer, nIndex);

	ENDXFTYPE	enType = pData->GetType();
	if ( DXFPOINTDATA<=enType && enType<=DXFELLIPSEDATA )
		m_nDataCnt[enType]++;
	else if ( enType == DXFPOLYDATA ) {
		CDXFpolyline* pPoly = static_cast<CDXFpolyline*>(pData);
		m_nDataCnt[DXFLINEDATA]		+= pPoly->GetObjectCount(0);
		m_nDataCnt[DXFARCDATA]		+= pPoly->GetObjectCount(1);
		m_nDataCnt[DXFELLIPSEDATA]	+= pPoly->GetObjectCount(2);
	}
	pLayer->DataOperation(pData, enOperation, nIndex);
	m_nLayerDataCnt[nLayerType-1]++;
	if ( nLayerType == DXFCAMLAYER )
		SetMaxRect(pData);
}

void CDXFDoc::RemoveAt(LPCTSTR lpszLayer, int nIndex, int nCnt)
{
	CLayerData*	pLayer;
	if ( m_mpLayer.Lookup(lpszLayer, pLayer) ) {
		int	n = pLayer->GetDxfSize() - nIndex;
		nCnt = min(nCnt, n);
		while ( nCnt-- )
			RemoveData( pLayer, nIndex );
	}
}

void CDXFDoc::RemoveAtText(LPCTSTR lpszLayer, int nIndex, int nCnt)
{
	CLayerData*	pLayer;
	if ( m_mpLayer.Lookup(lpszLayer, pLayer) ) {
		int	n = pLayer->GetDxfTextSize() - nIndex;
		nCnt = min(nCnt, n);
		while ( nCnt-- )
			RemoveData( pLayer, nIndex );
	}
}

CLayerData* CDXFDoc::AddLayerMap(const CString& strLayer, int nType/*=DXFCAMLAYER*/)
{
	ASSERT( !strLayer.IsEmpty() );
	CLayerData* pLayer = NULL;
	if ( !m_mpLayer.Lookup(strLayer, pLayer) ) {
		pLayer = new CLayerData(strLayer, nType);
		m_mpLayer.SetAt(strLayer, pLayer);
		m_obLayer.Add(pLayer);
	}
	ASSERT( pLayer );
	return pLayer;
}

void CDXFDoc::DelLayerMap(CLayerData* pLayer)
{
	if ( pLayer->GetDxfSize()<=0 && pLayer->GetDxfTextSize()<=0 ) {
		m_mpLayer.RemoveKey(pLayer->GetStrLayer());
		for ( int i=0; i<m_obLayer.GetSize(); i++ ) {
			if ( pLayer == m_obLayer[i] ) {
				m_obLayer.RemoveAt(i);
				break;
			}
		}
		delete	pLayer;
	}
}

CString CDXFDoc::CheckDuplexFile(const CString& strOrgFile, const CLayerArray* pArray/*=NULL*/)
{
	// ﾚｲﾔ配列情報無し => MakeNCDlgEx[1|2]1明細
	if ( !pArray )
		pArray = &m_obLayer;

	int			i, j;
	const int	nLoop = pArray->GetSize();
	CLayerData*	pLayer;
	CLayerData*	pLayerCmp;
	CString		strLayer, strFile, strCmp;

	// ｵﾘｼﾞﾅﾙﾌｧｲﾙとの重複ﾁｪｯｸと個別出力同士の重複ﾁｪｯｸ
	for ( i=0; i<nLoop; i++ ) {
		pLayer = pArray->GetAt(i);
		strFile = pLayer->GetNCFile();
		if ( !pLayer->IsLayerFlag(LAYER_CUT_TARGET) || !pLayer->IsLayerFlag(LAYER_PART_OUT) || strFile.IsEmpty() )
			continue;
		strLayer = pLayer->GetStrLayer();
		// ｵﾘｼﾞﾅﾙﾌｧｲﾙとの重複ﾁｪｯｸ(上位ﾀﾞｲｱﾛｸﾞのみ)
		if ( !strOrgFile.IsEmpty() ) {
			if ( strOrgFile.CompareNoCase(strFile) == 0 ) {
				AfxMessageBox(IDS_ERR_OVERLAPPINGFILE, MB_OK|MB_ICONEXCLAMATION);
				return strLayer;
			}
		}
		// 個別出力同士の重複ﾁｪｯｸ
		for ( j=i+1; j<nLoop; j++ ) {
			pLayerCmp = pArray->GetAt(j);
			strCmp = pLayerCmp->GetNCFile();
			if ( !pLayerCmp->IsLayerFlag(LAYER_CUT_TARGET) || !pLayerCmp->IsLayerFlag(LAYER_PART_OUT) || strCmp.IsEmpty() )
				continue;
			if ( strFile.CompareNoCase(strCmp) == 0 ) {
				AfxMessageBox(IDS_ERR_OVERLAPPINGFILE, MB_OK|MB_ICONEXCLAMATION);
				return strLayer;
			}
		}
	}

	return CString();
}

BOOL CDXFDoc::ReadLayerMap(LPCTSTR lpszFile)
{
	extern	LPCTSTR		gg_szComma;		// StdAfx.cpp
	int		n;
	BOOL	bResult = TRUE;
	CString	strTmp, strBuf;
	TCHAR	szCurrent[_MAX_PATH];
	CLayerData*	pLayer;

	// ｶﾚﾝﾄﾃﾞｨﾚｸﾄﾘを lpszFile に -> PathSearchAndQualify()
	::GetCurrentDirectory(_MAX_PATH, szCurrent);
	::Path_Name_From_FullPath(lpszFile, strTmp/*strPath*/, strBuf/*dummy*/);
	::SetCurrentDirectory(strTmp);

	try {
		CStdioFile	fp(lpszFile,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		// 読み込みﾙｰﾌﾟ
		while ( fp.ReadString(strTmp) ) {
			strBuf = strTmp.Trim();
			if ( ::NC_IsNullLine(strBuf) )
				continue;
			// ﾚｲﾔ名だけ検索
			n = strBuf.Find(gg_szComma);
			if ( n > 0 ) {
				pLayer = GetLayerData(strBuf.Left(n).Trim());
				if ( pLayer ) {
					// あとはCLayerDataの仕事
					pLayer->SetLayerInfo(strBuf.Mid(n+1));
				}
			}
		}
	}
	catch (CFileException* e) {
		strBuf.Format(IDS_ERR_DXF2NCDINIT_EX, lpszFile);
		AfxMessageBox(strBuf, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}

	// ｶﾚﾝﾄﾃﾞｨﾚｸﾄﾘを元に戻す
	::SetCurrentDirectory(szCurrent);

	return bResult;
}

BOOL CDXFDoc::SaveLayerMap(LPCTSTR lpszFile, const CLayerArray* pLayer/*=NULL*/)
{
	int		i;

	try {
		CLayerArray	obLayer;
		if ( pLayer ) {
			for ( i=0; i<pLayer->GetSize(); i++ )
				obLayer.Add( pLayer->GetAt(i) );
		}
		else
			obLayer.Copy( m_obLayer );
		obLayer.Sort(LayerCompareFunc_Name);	// 保存のためだけに名前順ｿｰﾄ
		//
		CStdioFile	fp(lpszFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
		// ｺﾒﾝﾄ出力
		for ( i=0; i<SIZEOF(g_szLayerToInitComment); i++ )
			fp.WriteString(g_szLayerToInitComment[i]);
		fp.WriteString(g_szLayerToInitComment[0]);
		// ﾃﾞｰﾀ出力
		for ( i=0; i<obLayer.GetSize(); i++ ) {
			if ( obLayer[i]->IsCutType() )
				fp.WriteString( obLayer[i]->FormatLayerInfo(lpszFile) );
		}
		fp.Close();
	}
	catch (CFileException* e) {
		CString	strMsg;
		strMsg.Format(IDS_ERR_DXF2NCDINIT_EX, lpszFile);
		AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

void CDXFDoc::UpdateLayerSequence(void)
{
	m_obLayer.Sort(LayerCompareFunc_CutNo);
}

void CDXFDoc::AllChangeFactor(double f) const
{
	int		i;
	f *= LOMETRICFACTOR;

	for ( i=0; i<m_obLayer.GetSize(); i++ )
		m_obLayer[i]->AllChangeFactor(f);
	if ( m_pCircle )
		m_pCircle->DrawTuning(f);
	for ( i=0; i<SIZEOF(m_pLatheLine); i++ ) {
		if ( m_pLatheLine[i] )
			m_pLatheLine[i]->DrawTuning(f);
	}
}

void CDXFDoc::CreateCutterOrigin(const CPointD& pt, double r, BOOL bRedraw/*=FALSE*/)
{
	CDXFcircleEx*	pCircle = NULL;

	try {
		pCircle = new CDXFcircleEx(DXFORGDATA, NULL, pt, r);
		if ( m_pCircle )
			delete	m_pCircle;
		m_pCircle = pCircle;
	}
	catch (CMemoryException* e) {
		if ( pCircle )
			delete	pCircle;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		m_bDocFlg.reset(DXFDOC_READY);
		return;
	}

	if ( bRedraw )
		UpdateAllViews(NULL, UAV_DXFORGUPDATE, m_pCircle);
}

void CDXFDoc::CreateLatheLine(const CPointD& pts, const CPointD& pte)
{
	int		i;

	for ( i=0; i<SIZEOF(m_pLatheLine); i++ ) {
		if ( !m_pLatheLine[i] )
			break;
	}
	if ( i >= SIZEOF(m_pLatheLine) )
		return;

	try {
		DXFLARGV	dxfLine;
		dxfLine.pLayer = NULL;
		dxfLine.s      = pts;
		dxfLine.e      = pte;
		m_pLatheLine[i] = new CDXFline(&dxfLine);
	}
	catch (CMemoryException* e) {
		if ( m_pLatheLine[i] ) {
			delete	m_pLatheLine[i];
			m_pLatheLine[i] = NULL;
		}
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		m_bDocFlg.reset(DXFDOC_LATHE);
		return;
	}
}

void CDXFDoc::CreateLatheLine(const CDXFline* pLine, LPCDXFBLOCK pBlock)
{
	int		i;

	for ( i=0; i<SIZEOF(m_pLatheLine); i++ ) {
		if ( !m_pLatheLine[i] )
			break;
	}
	if ( i >= SIZEOF(m_pLatheLine) )
		return;

	try {
		m_pLatheLine[i] = new CDXFline(NULL, pLine, pBlock);
	}
	catch (CMemoryException* e) {
		if ( m_pLatheLine[i] ) {
			delete	m_pLatheLine[i];
			m_pLatheLine[i] = NULL;
		}
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		m_bDocFlg.reset(DXFDOC_LATHE);
		return;
	}
}

BOOL CDXFDoc::GetEditOrgPoint(LPCTSTR lpctStr, CPointD& pt)
{
	extern	LPCTSTR	gg_szCat;
	LPTSTR	lpszNum = NULL, lpsztok, lpszcontext;
	BOOL	bResult = TRUE;
	
	// 「X,Y」形式の文字列を分離
	try {
		lpszNum = new TCHAR[lstrlen(lpctStr)+1];
		lpsztok = strtok_s(lstrcpy(lpszNum, lpctStr), gg_szCat, &lpszcontext);
		for ( int i=0; i<2 && lpsztok; i++ ) {
			switch ( i ) {
			case 0:
				pt.x = atof(lpsztok);	break;
			case 1:
				pt.y = atof(lpsztok);	break;
			}
			lpsztok = strtok_s(NULL, gg_szCat, &lpszcontext);
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

tuple<CDXFshape*, CDXFdata*, double> CDXFDoc::GetSelectObject(const CPointD& pt, const CRectD& rcView)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetSelectViewPointGap()", DBG_CYAN);
#endif
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFshape*	pShapeResult = NULL;
	CDXFdata*	pData;
	CDXFdata*	pDataResult  = NULL;
	int			i, j;
	double		dGap, dGapMin = DBL_MAX;
	CRectD		rcShape;

	// 全ての切削ｵﾌﾞｼﾞｪｸﾄから一番近い集合と距離を取得
	for ( i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		if ( !pLayer->IsCutType() || !pLayer->IsLayerFlag(LAYER_VIEW) )
			continue;
		for ( j=0; j<pLayer->GetShapeSize(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			// 表示矩形に少しでもかかっていれば(交差含む)
			if ( rcShape.CrossRect(rcView, pShape->GetMaxRect()) ) {
				dGap = pShape->GetSelectObjectFromShape(pt, &rcView, &pData);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pShapeResult = pShape;
					pDataResult  = pData;
				}
			}
		}
	}

	return make_tuple(pShapeResult, pDataResult, dGapMin);
}

void CDXFDoc::SortBindInfo(void)
{
	m_bindInfo.Sort(BindAreaCompareFunc);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc 診断

#ifdef _DEBUG
void CDXFDoc::AssertValid() const
{
	__super::AssertValid();
}

void CDXFDoc::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}

void CDXFDoc::DbgSerializeInfo(void)
{
	CMagaDbg	dbg("CDXFDoc::DbgSerializeInfo()");

	dbg.printf("m_rcMax left =%f right =%f", m_rcMax.left, m_rcMax.right);
	dbg.printf("        top  =%f bottom=%f", m_rcMax.top, m_rcMax.bottom);
	dbg.printf("        Width=%f Height=%f", m_rcMax.Width(), m_rcMax.Height());
	dbg.printf("        CenterPoint() x=%f y=%f", m_rcMax.CenterPoint().x, m_rcMax.CenterPoint().y);
	if ( m_pCircle ) {
		dbg.printf("m_ptOrg x=%f y=%f R=%f",
			m_pCircle->GetCenter().x, m_pCircle->GetCenter().y,
			m_pCircle->GetR() );
	}
	else {
		dbg.printf("m_ptOrg ???");
	}
	CLayerData*	pLayer;
	for ( int i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		dbg.printf("LayerName=%s DataCnt=%d TextCnt=%d",
			pLayer->GetStrLayer(), pLayer->GetDxfSize(), pLayer->GetDxfTextSize());
	}
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc クラスのオーバライド関数

BOOL CDXFDoc::OnNewDocument()
{
	const CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	// CNCVCApp::OnFileCADbind() CADﾃﾞｰﾀの統合からのみ呼ばれる
	// ﾒﾆｭｰｺﾏﾝﾄﾞの新規作成は無い
	m_rcMax.left = m_rcMax.top = 0;		// zero clear
	m_rcMax.right	= pOpt->GetBindSize(NCA_X);
	m_rcMax.bottom	= pOpt->GetBindSize(NCA_Y);
	// 原点処理
	CPointD		pt;
	switch ( pOpt->GetDxfFlag(DXFOPT_BINDORG) ) {
	case 1:		// 右上
		pt = m_rcMax.BottomRight();
		break;
	case 2:		// 右下
		pt.x = m_rcMax.right;
		pt.y = m_rcMax.top;
		break;
	case 3:		// 左上
		pt.x = m_rcMax.left;
		pt.y = m_rcMax.bottom;
		break;
	case 4:		// 左下
		pt = m_rcMax.TopLeft();
		break;
	default:	// 中央(OnOpenDocumentとは並びが違う)
		pt = m_rcMax.CenterPoint();
		break;
	}
	CreateCutterOrigin(pt, ORGRADIUS);
	SetDocFlag(DXFDOC_BINDPARENT);	// 統合ﾓｰﾄﾞ(親)に設定

	return __super::OnNewDocument();
}

BOOL CDXFDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	BOOL	bResult;
	int		i, nCnt;
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
		bResult = __super::OnOpenDocument(lpszPathName);
	}

	// NC生成可能かどうかのﾁｪｯｸ
	if ( bResult ) {
#ifdef _DEBUG
		DbgSerializeInfo();
#endif
		// 切削ﾃﾞｰﾀがないとき
		for ( i=nCnt=0; i<m_obLayer.GetSize(); i++ )
			nCnt += m_obLayer[i]->GetDxfSize();
		if ( nCnt < 1 ) {
			if ( !IsBindMode() )
				AfxMessageBox(IDS_ERR_DXFDATACUT, MB_OK|MB_ICONEXCLAMATION);
			bResult = FALSE;
		}
		else if ( m_pCircle ) {
			if ( !m_ptOrgOrig )		// ｼﾘｱﾙ情報に含まれている(CAMﾌｧｲﾙ)場合を除く
				m_ptOrgOrig = m_pCircle->GetCenter();	// ﾌｧｲﾙからのｵﾘｼﾞﾅﾙ原点を保存
			m_bDocFlg.set(DXFDOC_READY);	// 生成OK!
		}
		else if ( !IsBindMode() ) {
			// 原点ﾃﾞｰﾀがないとき(矩形の上下に注意)
			optional<CPointD>	pt;
			CPointD				ptRC;	// optional型への代入用
			switch ( AfxGetNCVCApp()->GetDXFOption()->GetDxfFlag(DXFOPT_ORGTYPE) ) {
			case 1:	// 右上
				pt = m_rcMax.BottomRight();
				break;
			case 2:	// 右下
				ptRC.x = m_rcMax.right;
				ptRC.y = m_rcMax.top;
				pt = ptRC;
				break;
			case 3:	// 左上
				ptRC.x = m_rcMax.left;
				ptRC.y = m_rcMax.bottom;
				pt = ptRC;
				break;
			case 4:	// 左下
				pt = m_rcMax.TopLeft();
				break;
			case 5:	// 中央
				pt = m_rcMax.CenterPoint();
				break;
			default:
				AfxMessageBox(IDS_ERR_DXFDATAORG, MB_OK|MB_ICONEXCLAMATION);
				// OnOpenDocument() 戻り値 bResult はそのままで良い
				break;
			}
			if ( pt ) {
				CreateCutterOrigin(*pt, ORGRADIUS);
				m_bDocFlg.set(DXFDOC_READY);	// 生成OK!
			}
		}
	}

	if ( bResult && !IsBindMode() ) {
		if ( m_bDocFlg[DXFDOC_READY] ) {
			// 旋盤ﾃﾞｰﾀ生成可能かどうかのﾁｪｯｸ
			if ( m_pLatheLine[0] && m_pLatheLine[1] ) {
				m_bDocFlg.set(DXFDOC_LATHE);
				// 外径を表す線を[0]に、（最小X座標を持つｵﾌﾞｼﾞｪｸﾄ）
				// 端面を表す線を[1]に
				CPointD	pt1, pt2;
				double	minX = DBL_MAX;
				int		j, n = 0;
				for ( i=0; i<SIZEOF(m_pLatheLine); i++ ) {
					for ( j=0; j<2; j++ ) {
						pt1 = m_pLatheLine[i]->GetNativePoint(j);
						if ( pt1.x < minX ) {
							minX = pt1.x;
							n = 2*i+j;
						}
					}
				}
				if ( n > 1 )
					swap(m_pLatheLine[0], m_pLatheLine[1]);
				// 外径ｵﾌﾞｼﾞｪｸﾄ調整(X値の大きい方を始点に)
				pt1 = m_pLatheLine[0]->GetNativePoint(0);
				pt2 = m_pLatheLine[0]->GetNativePoint(1);
				if ( pt1.x < pt2.x )
					m_pLatheLine[0]->SwapNativePt();
				// 端面ｵﾌﾞｼﾞｪｸﾄ調整(Y値の大きい方を始点に)
				pt1 = m_pLatheLine[1]->GetNativePoint(0);
				pt2 = m_pLatheLine[1]->GetNativePoint(1);
				if ( pt1.y < pt2.y )
					m_pLatheLine[1]->SwapNativePt();
			}
			// ﾜｲﾔ加工機ﾃﾞｰﾀ生成可能かどうかのﾁｪｯｸ
			if ( GetCutLayerCnt() <= 2 )
				m_bDocFlg.set(DXFDOC_WIRE);
		}
		// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
		POSITION	pos = GetFirstViewPosition();
		ASSERT( pos );
		OnOpenDocumentSP(lpszPathName, GetNextView(pos)->GetParentFrame());	// CDocBase
	}

	if ( !IsBindMode() ) {
		// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ初期化
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
	}

	return bResult;
}

BOOL CDXFDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// 各ﾋﾞｭｰに保存通知
	UpdateAllViews(NULL, UAV_FILESAVE);	// CDXFShapeView順序更新

	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの終了
	OnCloseDocumentSP();	// CDocBase

	if ( GetPathName().CompareNoCase(lpszPathName) != 0 ) {
		CDocument* pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_DXF, lpszPathName);
		if ( pDoc )
			pDoc->OnCloseDocument();	// 既に開いているﾄﾞｷｭﾒﾝﾄを閉じる
	}

	// 保存処理
	BOOL bResult = __super::OnSaveDocument(lpszPathName);

	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
	if ( bResult ) {
		POSITION	pos = GetFirstViewPosition();
		ASSERT( pos );
		OnOpenDocumentSP(lpszPathName, GetNextView(pos)->GetParentFrame());
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
	OnCloseDocumentSP();	// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ
	m_bDocFlg.reset(DXFDOC_THREAD);
	m_csRestoreCircleType.Lock();
	m_csRestoreCircleType.Unlock();
#ifdef _DEBUG
	dbg.printf("m_csRestoreCircleType Unlock OK");
#endif

	__super::OnCloseDocument();
}

void CDXFDoc::ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault) 
{
	if ( e->IsKindOf(RUNTIME_CLASS(CUserException)) ) {
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		return;	// 標準ｴﾗｰﾒｯｾｰｼﾞを出さない
	}
	__super::ReportSaveLoadException(lpszPathName, e, bSaving, nIDPDefault);
}

void CDXFDoc::SetModifiedFlag(BOOL bModified)
{
	CString	strTitle( GetTitle() );
	if ( UpdateModifiedTitle(bModified, strTitle) )		// DocBase.cpp
		SetTitle(strTitle);
	__super::SetModifiedFlag(bModified);
}

BOOL CDXFDoc::SaveModified()
{
	BOOL	bResult;
	CString	strExt( AfxGetNCVCApp()->GetDocExtString(TYPE_DXF) ),
			strOrigPath( GetPathName() );

	// CADﾃﾞｰﾀで更新があった場合
	if ( IsModified() && strExt.CompareNoCase(strOrigPath.Right(4)) != 0 ) {
		CString	strPath, strName, strMsg;
		::Path_Name_From_FullPath(strOrigPath, strPath, strName);
		strMsg.Format(IDS_ANA_VALUECHANGE, strName);
		switch ( AfxMessageBox(strMsg, MB_YESNOCANCEL|MB_ICONQUESTION) ) {
		case IDCANCEL:
			bResult = FALSE;
			break;
		case IDYES:
			OnFileSaveAs();		// camで保存問い合わせ
			// through
		default:	// case IDNO:
			bResult = TRUE;
			break;
		}
	}
	else {
		// ﾃﾞﾌｫﾙﾄの保存問い合わせ処理
		bResult = __super::SaveModified();
	}

	return bResult;
}

void CDXFDoc::UpdateFrameCounts()
{
	// ﾌﾚｰﾑの数を抑制することでｳｨﾝﾄﾞｳﾀｲﾄﾙの「:1」を付与しないようにする
	CFrameWnd*	pFrame;
	for ( POSITION pos=GetFirstViewPosition(); pos; ) {
		pFrame = GetNextView(pos)->GetParentFrame();
		if ( pFrame ) {
			pFrame->m_nWindow = 0;
			pFrame->OnUpdateFrameTitle(TRUE);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc シリアライズ

void CDXFDoc::Serialize(CArchive& ar)
{
	extern	DWORD	g_dwCamVer;		// NCVC.cpp

	// DxfSetupReloadのﾁｪｯｸOFF
	m_bDocFlg.reset(DXFDOC_RELOAD);

	int			i, j, nLoopCnt;
	BYTE		nCnt = 0;
	BOOL		bReady, bShape;
	CPointD		pt;
	CLayerData*	pLayer;

	// ﾍｯﾀﾞｰ情報
	CCAMHead	cam;
	cam.Serialize(ar);

	if ( ar.IsStoring() ) {
		// 各種状態
		bReady = m_bDocFlg[DXFDOC_READY];
		bShape = m_bDocFlg[DXFDOC_SHAPE];
		ar << bReady << bShape;
		ar << m_AutoWork.nSelect << m_AutoWork.dOffset << m_AutoWork.bAcuteRound <<
			m_AutoWork.nLoopCnt << m_AutoWork.nScanLine << m_AutoWork.bCircleScroll;
		// NC生成ﾌｧｲﾙ名
		ar << m_strNCFileName;
		// 原点ｵﾌﾞｼﾞｪｸﾄ
		if ( m_pCircle ) {
			ar << (BYTE)1;
			ar.WriteObject(m_pCircle);
		}
		else
			ar << (BYTE)0;
		// CADｵﾘｼﾞﾅﾙ原点
		if ( m_ptOrgOrig ) {
			ar << (BYTE)1;
			pt = *m_ptOrgOrig;
			ar << pt.x << pt.y;
		}
		else
			ar << (BYTE)0;
		// 旋盤原点
		if ( m_pLatheLine[0] )
			nCnt++;
		if ( m_pLatheLine[1] )
			nCnt++;
		ar << nCnt;
		for ( i=0; i<SIZEOF(m_pLatheLine); i++ ) {
			if ( m_pLatheLine[i] )
				ar.WriteObject(m_pLatheLine[i]);
		}
		// ﾚｲﾔ情報(兼DXFﾃﾞｰﾀ)の保存
		m_obLayer.Serialize(ar);
		return;		//保存はここまで
	}

	// 各種状態
	ar >> bReady >> bShape;
	m_bDocFlg.set(DXFDOC_READY, bReady);
	m_bDocFlg.set(DXFDOC_SHAPE, bShape);
	if ( g_dwCamVer > NCVCSERIALVERSION_1600 ) {	// Ver1.70～
		ar >> m_AutoWork.nSelect >> m_AutoWork.dOffset >> m_AutoWork.bAcuteRound >>
			m_AutoWork.nLoopCnt >> m_AutoWork.nScanLine >> m_AutoWork.bCircleScroll;
	}
	else {
		ar >> m_AutoWork.dOffset;
		if ( g_dwCamVer > NCVCSERIALVERSION_1503 )	// Ver1.10～
			ar >> m_AutoWork.bAcuteRound;
	}
	// NC生成ﾌｧｲﾙ名
	ar >> m_strNCFileName;
	// 原点ｵﾌﾞｼﾞｪｸﾄ
	ar >> nCnt;
	if ( nCnt > 0 )
		m_pCircle = static_cast<CDXFcircleEx*>(ar.ReadObject(RUNTIME_CLASS(CDXFcircleEx)));
	// CADｵﾘｼﾞﾅﾙ原点
	if ( g_dwCamVer > NCVCSERIALVERSION_1505 ) // Ver1.10a～
		ar >> nCnt;
	else
		nCnt = 1;		// 強制読み込み
	if ( nCnt > 0 ) {
		ar >> pt.x >> pt.y;
		m_ptOrgOrig = pt;
	}
	// 旋盤原点
	if ( g_dwCamVer > NCVCSERIALVERSION_1700 ) {	// Ver2.30～
		ar >> nCnt;
		for ( i=0; i<nCnt && i<SIZEOF(m_pLatheLine); i++ )
			m_pLatheLine[i] = static_cast<CDXFline*>(ar.ReadObject(RUNTIME_CLASS(CDXFline)));
		for ( ; i<SIZEOF(m_pLatheLine); i++ )
			m_pLatheLine[i] = NULL;
	}

	// ﾚｲﾔ情報(兼DXFﾃﾞｰﾀ)の読み込み
	m_obLayer.Serialize(ar);

	// --- ﾃﾞｰﾀ読み込み後の処理

	// NC生成ﾌｧｲﾙのﾊﾟｽﾁｪｯｸ
	if ( !m_strNCFileName.IsEmpty() ) {
		CString	strPath, strFile;
		::Path_Name_From_FullPath(m_strNCFileName, strPath, strFile);
		if ( !::PathFileExists(strPath) )
			m_strNCFileName.Empty();
	}

	// ﾚｲﾔ情報処理
	CDXFdata*	pData;
	ENDXFTYPE	enType;
	for ( i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		// 通常配列からﾚｲﾔ名のﾏｯﾋﾟﾝｸﾞ
		m_mpLayer.SetAt(pLayer->GetStrLayer(), pLayer);
		// ﾃﾞｰﾀのｶｳﾝﾄと占有領域の設定
		nLoopCnt = pLayer->GetDxfSize();
		m_nLayerDataCnt[pLayer->GetLayerType()-1] += nLoopCnt;
		for ( j=0; j<nLoopCnt; j++ ) {
			pData = pLayer->GetDxfData(j);
			enType = pData->GetType();
			if ( DXFPOINTDATA<=enType && enType<=DXFELLIPSEDATA )
				m_nDataCnt[enType]++;
			else if ( enType == DXFPOLYDATA ) {
				CDXFpolyline*	pPoly = static_cast<CDXFpolyline*>(pData);
				m_nDataCnt[DXFLINEDATA]		+= pPoly->GetObjectCount(0);
				m_nDataCnt[DXFARCDATA]		+= pPoly->GetObjectCount(1);
				m_nDataCnt[DXFELLIPSEDATA]	+= pPoly->GetObjectCount(2);
			}
			if ( pLayer->GetLayerType() == DXFCAMLAYER )
				SetMaxRect(pData);
		}
		if ( pLayer->GetLayerType() == DXFCAMLAYER ) {
			for ( j=0; j<pLayer->GetDxfTextSize(); j++ )
				SetMaxRect( pLayer->GetDxfTextData(j) );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc コマンド

void CDXFDoc::OnEditOrigin() 
{
	DWORD	dwControl = 0;
	if ( !m_pCircle )
		dwControl |= EDITORG_NUMERIC;
	if ( !m_ptOrgOrig )
		dwControl |= EDITORG_ORIGINAL;
	CDxfEditOrgDlg	dlg( dwControl );
	if ( dlg.DoModal() != IDOK )
		return;

	// 原点移動処理
	optional<CPointD>	pt;
	CPointD				ptOffset;
	switch ( dlg.m_nSelect ) {
	case 0:		// 数値移動
		if ( GetEditOrgPoint(dlg.m_strNumeric, ptOffset) )
			pt = m_pCircle->GetCenter() + ptOffset;
		break;
	case 1:		// 矩形移動
		switch ( dlg.m_nRectType ) {
		case 0:	// 右上
			pt = m_rcMax.BottomRight();
			break;
		case 1:	// 右下
			ptOffset.x = m_rcMax.right;
			ptOffset.y = m_rcMax.top;
			pt = ptOffset;
			break;
		case 2:	// 左上
			ptOffset.x = m_rcMax.left;
			ptOffset.y = m_rcMax.bottom;
			pt = ptOffset;
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

	if ( pt ) {
		CreateCutterOrigin(*pt, ORGRADIUS, TRUE);
		m_bDocFlg.set(DXFDOC_READY);	// 生成OK!
	}
}

void CDXFDoc::OnEditShape() 
{
	// 状況案内ﾀﾞｲｱﾛｸﾞ(検索ｽﾚｯﾄﾞ生成)
	CThreadDlg	dlgThread(ID_EDIT_DXFSHAPE, this);
	if ( dlgThread.DoModal() == IDOK ) {
		m_bDocFlg.set(DXFDOC_SHAPE);
		// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳを広げる + DXFView のﾌｨｯﾄﾒｯｾｰｼﾞ送信
		static_cast<CDXFChild *>(AfxGetNCVCMainWnd()->MDIGetActive())->ShowShapeView();
		// DXFShapeView更新
		UpdateAllViews(NULL, UAV_DXFSHAPEUPDATE);
		// 更新ﾌﾗｸﾞ
		SetModifiedFlag();
	}
}

void CDXFDoc::OnEditAutoShape() 
{
	CDxfAutoWorkingDlg	dlg(&m_AutoWork);
	if ( dlg.DoModal() != IDOK )
		return;

	m_AutoWork.nSelect		= dlg.m_nSelect;
	m_AutoWork.dOffset		= dlg.m_dOffset;
	m_AutoWork.bAcuteRound	= dlg.m_bAcuteRound;
	m_AutoWork.nLoopCnt		= dlg.m_nLoopCnt;
	m_AutoWork.nScanLine	= dlg.m_nScan;
	m_AutoWork.bCircleScroll= dlg.m_bCircle;

	// ｵﾌｾｯﾄ初期値の更新
	int	i, j;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	for ( i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		if ( !pLayer->IsCutType() )
			continue;
		for ( j=0; j<pLayer->GetShapeSize(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			// 自動処理対象か否か(CDXFchain* だけを対象とする)
			if ( pShape->GetShapeType()!=DXFSHAPETYPE_CHAIN || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
				continue;
			// 各種設定の反映
			pShape->SetOffset(m_AutoWork.dOffset);
			pShape->SetAcuteRound(m_AutoWork.bAcuteRound);
		}
	}

	// 現在登録されている加工指示を削除
	UpdateAllViews(NULL, UAV_DXFAUTODELWORKING);

	// 自動加工指示
	CThreadDlg	dlgThread(ID_EDIT_SHAPE_AUTO, this,
					AUTOWORKING, (LPARAM)&m_AutoWork);
	if ( dlgThread.DoModal() == IDOK )
		UpdateAllViews(NULL, UAV_DXFAUTOWORKING);	// 自動加工指示登録(と再描画)
	else
		UpdateAllViews(NULL);						// 再描画のみ

	// 更新ﾌﾗｸﾞ
	SetModifiedFlag();
}

void CDXFDoc::OnEditStrictOffset()
{
	if ( AfxMessageBox(IDS_ANA_STRICTOFFSET, MB_YESNO|MB_ICONQUESTION) != IDYES )
		return;

	//	ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄ同士の交点を検索し、厳密なｵﾌｾｯﾄを計算する
	CThreadDlg	dlgThread(ID_EDIT_SHAPE_AUTO, this,
					AUTOSTRICTOFFSET);
	if ( dlgThread.DoModal() == IDOK ) {
		UpdateAllViews(NULL, UAV_DXFAUTOWORKING,
			reinterpret_cast<CObject *>(TRUE));	// 再描画のみ
		SetModifiedFlag();
	}
}

void CDXFDoc::OnUpdateEditShape(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bDocFlg[DXFDOC_LATHE] ? FALSE : !m_bDocFlg[DXFDOC_SHAPE]);
}

void CDXFDoc::OnUpdateEditShaping(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bDocFlg[DXFDOC_SHAPE]);
}

void CDXFDoc::OnUpdateShapePattern(CCmdUI* pCmdUI)
{
	if ( pCmdUI->m_nID == ID_EDIT_SHAPE_POC )	// ﾎﾟｹｯﾄ処理は未実装
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->SetCheck( pCmdUI->m_nID == m_nShapeProcessID );
}

void CDXFDoc::OnShapePattern(UINT nID)
{
	if ( m_nShapeProcessID != nID ) {
		// 各ﾋﾞｭｰへの通知を先にしないと、仮描画を消せない
		UpdateAllViews(NULL, UAV_DXFSHAPEID);	// to CDXFView::CancelForSelect()
		// IDの切り替え
		m_nShapeProcessID = nID;
	}
	else
		m_nShapeProcessID = 0;	// 加工指示無効化
}

void CDXFDoc::OnUpdateFileDXF2NCD(CCmdUI* pCmdUI) 
{
	BOOL	bEnable = TRUE;
	// ｴﾗｰﾃﾞｰﾀの場合はNC変換ﾒﾆｭｰを無効にする
	if ( !m_bDocFlg[DXFDOC_READY] || !m_pCircle )
		bEnable = FALSE;
	else if ( pCmdUI->m_nID == ID_FILE_DXF2NCD_SHAPE )
		bEnable = m_bDocFlg[DXFDOC_SHAPE];	// 形状処理が済んでいるか
	else if ( pCmdUI->m_nID == ID_FILE_DXF2NCD_LATHE )
		bEnable = m_bDocFlg[DXFDOC_LATHE];	// 旋盤用原点を読み込んだか
	else if ( pCmdUI->m_nID == ID_FILE_DXF2NCD_WIRE )
		bEnable = m_bDocFlg[DXFDOC_WIRE];	// ﾜｲﾔ加工機の条件を満たしているか
	else {
		// 単一ﾚｲﾔの場合は拡張生成をoffにする
		if ( pCmdUI->m_nID!=ID_FILE_DXF2NCD && GetCutLayerCnt()<=1 )
			bEnable = FALSE;
	}

	pCmdUI->Enable(bEnable);
}

void CDXFDoc::OnFileDXF2NCD(UINT nID) 
{
	int		i;
	enMAKETYPE	enType;
	BOOL	bNCView,
			bSingle = (nID==ID_FILE_DXF2NCD || nID==ID_FILE_DXF2NCD_SHAPE ||
						nID==ID_FILE_DXF2NCD_LATHE || nID==ID_FILE_DXF2NCD_WIRE);
	CLayerData* pLayer;
	CString	strInit, strLayerFile;
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();

	if ( nID == ID_FILE_DXF2NCD_SHAPE ) {
		// 全ﾚｲﾔを対象に
		for ( i=0; i<m_obLayer.GetSize(); i++ )
			m_obLayer[i]->SetCutTargetFlg(TRUE);
	}
	else {
		// 現在の表示状況で対象ﾌﾗｸﾞを置換
		for ( i=0; i<m_obLayer.GetSize(); i++ )
			m_obLayer[i]->SetCutTargetFlg_fromView();
	}

	switch ( nID ) {
	case ID_FILE_DXF2NCD_EX1:	// ﾚｲﾔごとの複数条件
	case ID_FILE_DXF2NCD_EX2:	// ﾚｲﾔごとのZ座標
		enType = NCMAKEMILL;
		{
			CMakeNCDlgEx	ps(nID, this);
			ps.SetWizardMode();
			if ( ps.DoModal() != ID_WIZFINISH )
				return;
			m_strNCFileName	= ps.m_strNCFileName;
			strInit = ps.m_strInitFileName;
			strLayerFile = ps.m_strLayerToInitFileName;
			bNCView = ps.m_dlg1.m_bNCView;
		}
		break;

	case ID_FILE_DXF2NCD:		// 標準生成
	case ID_FILE_DXF2NCD_SHAPE:	// 形状処理
	case ID_FILE_DXF2NCD_LATHE:	// 旋盤ﾃﾞｰﾀ生成
	case ID_FILE_DXF2NCD_WIRE:	// ﾜｲﾔ加工機ﾃﾞｰﾀ生成
		enType = ( nID <= ID_FILE_DXF2NCD_SHAPE ) ? NCMAKEMILL :
					(enMAKETYPE)(nID - ID_FILE_DXF2NCD_LATHE + 1);
		{
			CMakeNCDlg	dlg(nID-ID_FILE_DXF2NCD+IDS_MAKENCD_TITLE_BASIC, enType, this);
			if ( dlg.DoModal() != IDOK )
				return;
			m_strNCFileName	= dlg.m_strNCFileName;
			strInit = dlg.m_strInitFileName;
			bNCView = dlg.m_bNCView;
		}
		break;

	}

	// 設定の保存
	if ( !strInit.IsEmpty() ) {
		pOpt->AddInitHistory(enType, strInit);
	}
	if ( !strLayerFile.IsEmpty() ) {
		if ( SaveLayerMap(strLayerFile) )
			pOpt->AddInitHistory(NCMAKELAYER, strLayerFile);
	}
	pOpt->SetViewFlag(bNCView);

	BOOL		bAllOut;
	CDocument*	pDoc;
	// すでに開いているﾄﾞｷｭﾒﾝﾄなら閉じるｱﾅｳﾝｽ
	if ( bSingle ) {
		pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_NCD, m_strNCFileName);
		if ( pDoc )
			pDoc->OnCloseDocument();
		bAllOut = TRUE;
	}
	else {
		bAllOut = FALSE;
		for ( i=0; i<m_obLayer.GetSize(); i++ ) {
			pLayer = m_obLayer[i];
			if ( pLayer->IsMakeTarget() ) {
				if ( pLayer->IsLayerFlag(LAYER_PART_OUT) ) {
					pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_NCD, pLayer->GetNCFile());
					if ( pDoc )
						pDoc->OnCloseDocument();
				}
				else
					bAllOut = TRUE;
			}
		}
		if ( bAllOut ) {
			pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_NCD, m_strNCFileName);
			if ( pDoc )
				pDoc->OnCloseDocument();
		}
		// ﾚｲﾔ情報の並べ替え
		m_obLayer.Sort(LayerCompareFunc_CutNo);
	}

	// CDXFCircleの穴加工対象ﾃﾞｰﾀを元に戻すｽﾚｯﾄﾞの終了待ち
	m_csRestoreCircleType.Lock();
	m_csRestoreCircleType.Unlock();

	// 状況案内ﾀﾞｲｱﾛｸﾞ(変換ｽﾚｯﾄﾞ生成)
	CThreadDlg	dlgThread(ID_FILE_DXF2NCD, this, (WPARAM)nID);
	int nRet = dlgThread.DoModal();

	// CDXFCircleの穴加工対象ﾃﾞｰﾀを元に戻す
	AfxBeginThread(RestoreCircleTypeThread, this,
		/*THREAD_PRIORITY_LOWEST*/
		THREAD_PRIORITY_IDLE
		/*THREAD_PRIORITY_BELOW_NORMAL*/);

	// 「NC生成後に開く」が無効の場合だけ出力完了ﾒｯｾｰｼﾞ
	if ( nRet == IDOK ) {
		if ( !bNCView && bAllOut ) {
			CString	strMsg;
			strMsg.Format(IDS_ANA_FILEOUTPUT, m_strNCFileName);
			AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
			return;
		}
	}
	else
		return;

	// NC生成後のﾃﾞｰﾀを開く
	if ( bSingle ) {
		AfxGetNCVCApp()->OpenDocumentFile(m_strNCFileName);
	}
	else {
		bAllOut = FALSE;	// m_strNCFileName も開くかどうか
		for ( i=0; i<m_obLayer.GetSize(); i++ ) {
			pLayer = m_obLayer[i];
			if ( pLayer->IsMakeTarget() && !pLayer->IsLayerFlag(LAYER_PART_ERROR) ) {
				if ( pLayer->IsLayerFlag(LAYER_PART_OUT) )
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
		OnFileSaveAs();
}

void CDXFDoc::OnFileSaveAs() 
{
	CString	strExt, strPath, strNewName( "*" + AfxGetNCVCApp()->GetDocExtString(TYPE_DXF) );
	strExt.Format(IDS_CAM_FILTER, strNewName, strNewName);
	::Path_Name_From_FullPath(m_strPathName, strPath, strNewName, FALSE);
	if ( ::NCVC_FileDlgCommon(AFX_IDS_SAVEFILE, strExt, TRUE, strNewName, strPath,
				FALSE, OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT) == IDOK ) {
		OnSaveDocument(strNewName);
		SetPathName(strNewName);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc サブスレッド

UINT CDXFDoc::RestoreCircleTypeThread(LPVOID pParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("RestoreCircleFlagThread()\nStart", TRUE, DBG_RED);
#endif
	int			i, j;
	ENDXFTYPE	enType;
	CDXFDoc*	pDoc = reinterpret_cast<CDXFDoc*>(pParam);
	CDXFdata*	pData;
	CLayerData*	pLayer;

	pDoc->m_csRestoreCircleType.Lock();		// ｽﾚｯﾄﾞ終了までﾛｯｸ
	for ( i=0; i<pDoc->m_obLayer.GetSize(); i++ ) {
		pLayer = pDoc->m_obLayer[i];
		for ( j=0; j<pLayer->GetDxfSize() && pDoc->m_bDocFlg[DXFDOC_THREAD]; j++ ) {
			pData = pLayer->GetDxfData(j);
			enType = pData->GetType();
			if ( enType != pData->GetMakeType() )
				pData->ChangeMakeType(enType);
		}
	}
	pDoc->m_csRestoreCircleType.Unlock();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// 並べ替え補助関数

int LayerCompareFunc_CutNo(CLayerData* pFirst, CLayerData* pSecond)
{
	return pFirst->GetLayerListNo() - pSecond->GetLayerListNo();
}

int LayerCompareFunc_Name(CLayerData* pFirst, CLayerData* pSecond)
{
	return pFirst->GetStrLayer().Compare( pSecond->GetStrLayer() );
}

int BindAreaCompareFunc(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	CRect	rc1 = pFirst->pDoc->GetMaxRect(),
			rc2 = pSecond->pDoc->GetMaxRect();
	double	a1 = rc1.Width() * rc1.Height(),
			a2 = rc2.Width() * rc2.Height();
	if ( a1 > a2 )
		return 1;
	else if ( a1 < a2 )
		return -1;
	else
		return 0;
}
