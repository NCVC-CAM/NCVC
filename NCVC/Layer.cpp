// Layer.cpp: CLayerData, CLayerMap クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCMakeMillOpt.h"
#include "DXFdata.h"
#include "Layer.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

// ﾚｲﾔ情報の保存
#define	LAYERTOINITORDER	10

IMPLEMENT_SERIAL(CLayerData, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

static	int		AreaCompareFunc1(CDXFshape*, CDXFshape*);	// 面積並べ替え(昇順)
static	int		AreaCompareFunc2(CDXFshape*, CDXFshape*);	// 面積並べ替え(降順)
static	int		SequenceCompareFunc(CDXFshape*, CDXFshape*);	// ｼﾘｱﾙﾅﾝﾊﾞｰで並べ替え

//////////////////////////////////////////////////////////////////////
// CLayerData クラス
//////////////////////////////////////////////////////////////////////

CLayerData::CLayerData()
{
	m_nType		= -1;
	m_bLayerFlg.reset();
	m_nListNo	= -1;
	m_dInitZCut	= m_dZCut = 0.0;
	m_obDXFArray.SetSize(0, 1024);
	m_obDXFTextArray.SetSize(0, 64);
	m_obShapeArray.SetSize(0, 64);
}

CLayerData::CLayerData(const CString& strLayer, int nType)
{
	m_strLayer	= strLayer;
	m_nType		= nType;
	m_bLayerFlg.reset();
	m_bLayerFlg.set(LAYER_VIEW);
	m_bLayerFlg.set(LAYER_CUT_TARGET);
	m_bLayerFlg.set(LAYER_DRILL_Z);
	m_nListNo	= -1;
	m_dInitZCut	= m_dZCut = 0.0;
	m_obDXFArray.SetSize(0, 1024);
	m_obDXFTextArray.SetSize(0, 64);
	m_obShapeArray.SetSize(0, 64);
}

CLayerData::CLayerData(const CLayerData* pLayer, BOOL bCut)
{	// CMakeNCDlgEx[1|2][1]用ｺﾋﾟｰｺﾝｽﾄﾗｸﾀ
	// 必要なﾃﾞｰﾀだけｺﾋﾟｰ
	m_strLayer		= pLayer->m_strLayer;
	m_nType			= pLayer->m_nType;
	m_nListNo		= pLayer->m_nListNo;
	m_bLayerFlg.reset();
	m_bLayerFlg.set(LAYER_CUT_TARGET, bCut);
	m_bLayerFlg.set(LAYER_DRILL_Z,  pLayer->IsLayerFlag(LAYER_DRILL_Z));
	m_bLayerFlg.set(LAYER_PART_OUT, pLayer->IsLayerFlag(LAYER_PART_OUT));
	m_strInitFile	= pLayer->m_strInitFile;
	m_strNCFile		= pLayer->m_strNCFile;
	m_strLayerComment = pLayer->m_strLayerComment;
	m_strLayerCode	= pLayer->m_strLayerCode;
	m_dInitZCut		= pLayer->m_dInitZCut;
	m_dZCut			= pLayer->m_dZCut;
}

CLayerData::~CLayerData()
{
	int		i;
	for ( i=0; i<m_obShapeArray.GetSize(); i++ )
		delete	m_obShapeArray[i];
	for ( i=0; i<m_obDXFArray.GetSize(); i++ )
		delete	m_obDXFArray[i];
	for ( i=0; i<m_obDXFTextArray.GetSize(); i++ )
		delete	m_obDXFTextArray[i];
}

CDXFdata* CLayerData::DataOperation(CDXFdata* pData, ENDXFOPERATION enOperation, int nIndex)
{
	ASSERT( pData );
	ENDXFTYPE	enType = pData->GetType();
	CDXFdata*	pDataResult = NULL;
	CDXFarray*	pArray = enType == DXFTEXTDATA ? (CDXFarray *)&m_obDXFTextArray : &m_obDXFArray;

	switch ( enOperation ) {
	case DXFADD:
		pDataResult = pData;
		pArray->Add(pData);
		break;
	case DXFINS:
		pDataResult = pData;
		pArray->InsertAt(nIndex, pData);
		break;
	case DXFMOD:
		pDataResult = pArray->GetAt(nIndex);
		pArray->SetAt(nIndex, pData);
		break;
	}

	return pData;
}

void CLayerData::AscendingShapeSort(void)
{
	m_obShapeArray.Sort(AreaCompareFunc1);
}

void CLayerData::DescendingShapeSort(void)
{
	m_obShapeArray.Sort(AreaCompareFunc2);
}

void CLayerData::SerializeShapeSort(void)
{
	m_obShapeArray.Sort(SequenceCompareFunc);
}

void CLayerData::AllChangeFactor(double f) const
{
	int		i;
	for ( i=0; i<m_obDXFArray.GetSize(); i++ )
		m_obDXFArray[i]->DrawTuning(f);
	for ( i=0; i<m_obDXFTextArray.GetSize(); i++ )
		m_obDXFTextArray[i]->DrawTuning(f);
	// 形状ｵﾌﾞｼﾞｪｸﾄ分
	for ( i=0; i<m_obShapeArray.GetSize(); i++ )
		m_obShapeArray[i]->AllChangeFactor(f);
}

void CLayerData::AllShapeClearSideFlg(void) const
{
	for ( int i=0; i<m_obShapeArray.GetSize(); i++ )
		m_obShapeArray[i]->ClearSideFlg();
}

void CLayerData::DrawWorking(CDC* pDC)
{
	for ( int i=0; i<m_obShapeArray.GetSize(); i++ )
		m_obShapeArray[i]->DrawWorking(pDC);
}

void CLayerData::SetInitFile(LPCTSTR lpszInitFile)
{
	if ( !lpszInitFile || lstrlen(lpszInitFile)<=0 ) {
		m_strInitFile.Empty();
		m_dInitZCut = 0.0;
	}
	else {
		m_strInitFile = lpszInitFile;
		CNCMakeMillOpt	ncOpt(m_strInitFile);
		m_dInitZCut = ncOpt.GetFlag(MKNC_FLG_DEEP) ? 
			ncOpt.GetDbl(MKNC_DBL_DEEP) : ncOpt.GetDbl(MKNC_DBL_ZCUT);
	}
}

void CLayerData::SetLayerInfo(const CString& strBuf)
{
	extern	LPCTSTR		gg_szComma;		// StdAfx.cpp
	// 命令を分割
	typedef tokenizer< char_separator<TCHAR> > tokenizer;
	static	char_separator<TCHAR> sep(gg_szComma, "", keep_empty_tokens);

	int		i;
	TCHAR	szFile[_MAX_PATH];
	std::string	str( strBuf ), strTemp;
	tokenizer	tok( str, sep );
	tokenizer::iterator it;

	// 命令解析ﾙｰﾌﾟ
	for ( i=0, it=tok.begin(); i<LAYERTOINITORDER-1 && it!=tok.end(); i++, ++it ) {
		switch ( i ) {
		case 0:		// 切削対象ﾌﾗｸﾞ
			m_bLayerFlg.set(LAYER_CUT_TARGET, atoi(it->c_str()) ? 1 : 0);
			break;
		case 1:		// 切削条件ﾌｧｲﾙ
			strTemp = ::Trim(*it);	// stdafx.h
			// 相対ﾊﾟｽなら絶対ﾊﾟｽに
			if ( ::PathIsRelative(strTemp.c_str()) &&
					::PathSearchAndQualify(strTemp.c_str(), szFile, _MAX_PATH) )
				strTemp = szFile;
			SetInitFile(strTemp.c_str());
			break;
		case 2:		// 強制最深Z
			m_dZCut = atof(it->c_str());
			break;
		case 3:		// 強制最深Zを穴加工にも適用
			m_bLayerFlg.set(LAYER_DRILL_Z, atoi(it->c_str()) ? 1 : 0);
			break;
		case 4:		// 個別出力
			m_bLayerFlg.set(LAYER_PART_OUT, atoi(it->c_str()) ? 1 : 0);
			break;
		case 5:		// 個別出力ﾌｧｲﾙ名
			strTemp = ::Trim(*it);
			if ( ::PathIsRelative(strTemp.c_str()) &&
					::PathSearchAndQualify(strTemp.c_str(), szFile, _MAX_PATH) )
				strTemp = szFile;
			m_strNCFile = strTemp.c_str();
			break;
		case 6:		// 出力ｼｰｹﾝｽ
			strTemp = ::Trim(*it);
			if ( !strTemp.empty() )
				m_nListNo = atoi(strTemp.c_str());
			break;
		case 7:		// 出力ｺﾒﾝﾄ
			m_strLayerComment = ::Trim(*it).c_str();
			break;
		case 8:		// 出力ｺｰﾄﾞ
			m_strLayerCode = ::Trim(*it).c_str();
			break;
		}
	}
#ifdef _DEBUG
	g_dbg.printf("Layer=%s", m_strLayer);
	g_dbg.printf("--- Check=%d InitFile=%s", m_bLayerFlg[LAYER_CUT_TARGET] ? 1 : 0, m_strInitFile);
	g_dbg.printf("--- Z=%f Drill=%d", m_dZCut, m_bLayerFlg[LAYER_DRILL_Z] ? 1 : 0);
	g_dbg.printf("--- PartOut=%d NCFile=%s", m_bLayerFlg[LAYER_PART_OUT] ? 1 : 0, m_strNCFile);
	g_dbg.printf("--- Seq=%d, Comment=%s Code=%s", m_nListNo, m_strLayerComment, m_strLayerCode);
#endif
}

CString CLayerData::FormatLayerInfo(LPCTSTR lpszBase)
{
	CString	strResult, strInitFile, strNCFile;

	// 同じﾙｰﾄﾊﾟｽなら相対ﾊﾟｽに変換
	strInitFile = ::PathIsSameRoot(lpszBase, m_strInitFile) ?			// Shlwapi.h
			::RelativePath(lpszBase, m_strInitFile) : m_strInitFile;	// stdafx.cpp

	strNCFile = ::PathIsSameRoot(lpszBase, m_strNCFile) ?
			::RelativePath(lpszBase, m_strNCFile) : m_strNCFile;

	strResult.Format("%s, %d, %s, %.3f, %d, %d, %s, %d, %s, %s\n",
		m_strLayer,
		m_bLayerFlg[LAYER_CUT_TARGET] ? 1 : 0,
		strInitFile,
		m_dZCut,
		m_bLayerFlg[LAYER_DRILL_Z]  ? 1 : 0, 
		m_bLayerFlg[LAYER_PART_OUT] ? 1 : 0,
		strNCFile,
		m_nListNo,
		m_strLayerComment,
		m_strLayerCode);

	return strResult;
}

void CLayerData::Serialize(CArchive& ar)
{
	BOOL	bView;

	if ( ar.IsStoring() ) {
		bView = m_bLayerFlg[LAYER_VIEW];
		ar << m_strLayer << m_nType << bView;
		// CDXFmapｼﾘｱﾗｲｽﾞ情報用にCDXFdataのｼｰｹﾝｽ№初期化
		CDXFdata::ms_nSerialSeq = 0;
	}
	else {
		ar >> m_strLayer >> m_nType >> bView;
		m_bLayerFlg.set(LAYER_VIEW, bView);
		// CDXFdataｼﾘｱﾗｲｽﾞ情報用にCLayerData*をCArchive::m_pDocumentに格納
		ar.m_pDocument = reinterpret_cast<CDocument *>(this);
	}
	// DXFｵﾌﾞｼﾞｪｸﾄのｼﾘｱﾗｲｽﾞ
	m_obDXFArray.Serialize(ar);
	m_obDXFTextArray.Serialize(ar);
	// 形状情報と加工指示のｼﾘｱﾗｲｽﾞ
	if ( IsCutType() )
		m_obShapeArray.Serialize(ar);
}

//////////////////////////////////////////////////////////////////////

int AreaCompareFunc1(CDXFshape* pFirst, CDXFshape* pSecond)
{
	int		nResult;
	CRectD	rc1(pFirst->GetMaxRect()), rc2(pSecond->GetMaxRect());
	double	dResult = rc1.Width() * rc1.Height() - rc2.Width() * rc2.Height();
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0.0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int AreaCompareFunc2(CDXFshape* pFirst, CDXFshape* pSecond)
{
	int		nResult;
	CRectD	rc1(pFirst->GetMaxRect()), rc2(pSecond->GetMaxRect());
	double	dResult = rc2.Width() * rc2.Height() - rc1.Width() * rc1.Height();
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0.0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int SequenceCompareFunc(CDXFshape* pFirst, CDXFshape* pSecond)
{
	return pFirst->GetSerializeSeq() - pSecond->GetSerializeSeq();
}
