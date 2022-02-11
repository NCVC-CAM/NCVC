// Layer.cpp: CLayerData, CLayerMap クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCMakeMillOpt.h"
#include "DXFdata.h"
#include "Layer.h"
#include "DXFDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::string;
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
	m_dInitZCut	= m_dZCut = 0.0f;
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
	m_dInitZCut	= m_dZCut = 0.0f;
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

void CLayerData::AllChangeFactor(float f) const
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

void CLayerData::AllSetDxfFlg(DWORD dwFlags, BOOL bSet) const
{
	int		i;
	for ( i=0; i<m_obDXFArray.GetSize(); i++ )
		m_obDXFArray[i]->SetDxfFlg(dwFlags, bSet);
	for ( i=0; i<m_obDXFTextArray.GetSize(); i++ )
		m_obDXFTextArray[i]->SetDxfFlg(dwFlags, bSet);
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
		m_dInitZCut = 0.0f;
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
	extern	LPCTSTR		gg_szComma;		// ","

	int		i = 0;
	TCHAR	szFile[_MAX_PATH];
	string	str(strBuf), strTok;
	STDSEPA		sep(gg_szComma, "", keep_empty_tokens);
	STDTOKEN	tok(str, sep);

	// 命令解析ﾙｰﾌﾟ
	BOOST_FOREACH(strTok, tok) {
		boost::algorithm::trim(strTok);
		switch ( i++ ) {
		case 0:		// 切削対象ﾌﾗｸﾞ
			m_bLayerFlg.set(LAYER_CUT_TARGET, atoi(strTok.c_str()) ? 1 : 0);
			break;
		case 1:		// 切削条件ﾌｧｲﾙ
			// 相対ﾊﾟｽなら絶対ﾊﾟｽに
			if ( ::PathIsRelative(strTok.c_str()) &&
					::PathSearchAndQualify(strTok.c_str(), szFile, _MAX_PATH) )
				strTok = szFile;
			SetInitFile(strTok.c_str());
			break;
		case 2:		// 強制最深Z
			m_dZCut = (float)atof(strTok.c_str());
			break;
		case 3:		// 強制最深Zを穴加工にも適用
			m_bLayerFlg.set(LAYER_DRILL_Z, atoi(strTok.c_str()) ? 1 : 0);
			break;
		case 4:		// 個別出力
			m_bLayerFlg.set(LAYER_PART_OUT, atoi(strTok.c_str()) ? 1 : 0);
			break;
		case 5:		// 個別出力ﾌｧｲﾙ名
			if ( ::PathIsRelative(strTok.c_str()) &&
					::PathSearchAndQualify(strTok.c_str(), szFile, _MAX_PATH) )
				strTok = szFile;
			m_strNCFile = strTok.c_str();
			break;
		case 6:		// 出力ｼｰｹﾝｽ
			if ( !strTok.empty() )
				m_nListNo = atoi(strTok.c_str());
			break;
		case 7:		// 出力ｺﾒﾝﾄ
			m_strLayerComment = strTok.c_str();
			break;
		case 8:		// 出力ｺｰﾄﾞ
			m_strLayerCode = strTok.c_str();
			break;
		}
	}
#ifdef _DEBUG
	printf("Layer=%s\n", LPCTSTR(m_strLayer));
	printf("--- Check=%d InitFile=%s\n", m_bLayerFlg[LAYER_CUT_TARGET] ? 1 : 0, LPCTSTR(m_strInitFile));
	printf("--- Z=%f Drill=%d\n", m_dZCut, m_bLayerFlg[LAYER_DRILL_Z] ? 1 : 0);
	printf("--- PartOut=%d NCFile=%s\n", m_bLayerFlg[LAYER_PART_OUT] ? 1 : 0, LPCTSTR(m_strNCFile));
	printf("--- Seq=%d, Comment=%s Code=%s\n", m_nListNo, LPCTSTR(m_strLayerComment), LPCTSTR(m_strLayerCode));
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
	extern	DWORD	g_dwCamVer;		// NCVC.cpp
	BOOL	bView, bTarget;

	if ( ar.IsStoring() ) {
		bView = m_bLayerFlg[LAYER_VIEW];
		bTarget = m_bLayerFlg[LAYER_CUT_TARGET];
		ar << m_strLayer << m_nType << bView << bTarget;
		// CDXFmapｼﾘｱﾗｲｽﾞ情報用にCDXFdataのｼｰｹﾝｽ№初期化
		CDXFdata::ms_nSerialSeq = 0;
	}
	else {
		ar >> m_strLayer >> m_nType >> bView;
		m_bLayerFlg.set(LAYER_VIEW, bView);
		if ( g_dwCamVer >= NCVCSERIALVERSION_3600 ) {
			ar >> bTarget;
			m_bLayerFlg.set(LAYER_CUT_TARGET, bTarget);
		}
		// CDXFdataｼﾘｱﾗｲｽﾞ情報用にCLayerData*を格納
		static_cast<CDXFDoc *>(ar.m_pDocument)->SetSerializeLayer(this);
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
	CRectF	rc1(pFirst->GetMaxRect()), rc2(pSecond->GetMaxRect());
	float	dResult = rc1.Width() * rc1.Height() - rc2.Width() * rc2.Height();
	if ( dResult == 0.0f )
		nResult = 0;
	else if ( dResult > 0.0f )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int AreaCompareFunc2(CDXFshape* pFirst, CDXFshape* pSecond)
{
	int		nResult;
	CRectF	rc1(pFirst->GetMaxRect()), rc2(pSecond->GetMaxRect());
	float	dResult = rc2.Width() * rc2.Height() - rc1.Width() * rc1.Height();
	if ( dResult == 0.0f )
		nResult = 0;
	else if ( dResult > 0.0f )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int SequenceCompareFunc(CDXFshape* pFirst, CDXFshape* pSecond)
{
	return pFirst->GetSerializeSeq() - pSecond->GetSerializeSeq();
}
