// DXFDoc.cpp : �C���v�������e�[�V���� �t�@�C��
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
#include "DXFMakeOption.h"
#include "DXFMakeClass.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;

// ڲԏ��̕ۑ�
static	LPCTSTR	g_szLayerToInitComment[] = {
	"##\n",
	"## NCVC:ڲԖ��Ɛ؍����̧�ق̊֌W���̧��\n",
	"## ڲԖ��C�؍�Ώ��׸�(1:�Ώ� 0:���O)�C�؍����̧��,\n",
	"##\t�����Ő[Z, �����Ő[Z�������H�ɂ��K�p(1:���� 0:���Ȃ�),\n",
	"##\t�ʏo��(1:���� 0:���Ȃ�), �ʏo��̧�ٖ�,\n"
	"##\t�o�ͼ��ݽ, �o�ͺ���, �o�ͺ���\n"
};
// ڲԂ̐؍폇
static	int		LayerCompareFunc_CutNo(CLayerData*, CLayerData*);
// ڲԖ��̕��בւ�
static	int		LayerCompareFunc_Name(CLayerData*, CLayerData*);
// �������̕��ёւ�
static	int		BindHeightCompareFunc(LPCADBINDINFO, LPCADBINDINFO);
// �������̕��ёւ�
static	CPointF	g_ptCutterOrg;
static	int		BindMakeSeqFunc0(LPCADBINDINFO, LPCADBINDINFO);
static	int		BindMakeSeqFunc1(LPCADBINDINFO, LPCADBINDINFO);
static	int		BindMakeSeqFunc2(LPCADBINDINFO, LPCADBINDINFO);
static	int		BindMakeSeqFunc3(LPCADBINDINFO, LPCADBINDINFO);
static	int		BindMakeSeqFunc4(LPCADBINDINFO, LPCADBINDINFO);
static	int		BindMakeSeqFunc5(LPCADBINDINFO, LPCADBINDINFO);
static	int		BindMakeSeqFunc6(LPCADBINDINFO, LPCADBINDINFO);
static	int		BindMakeSeqFunc7(LPCADBINDINFO, LPCADBINDINFO);
static	int		BindMakeSeqFunc8(LPCADBINDINFO, LPCADBINDINFO);
//
#define	IsBindMode()	m_bDocFlg[DXFDOC_BIND]
#define	IsBindParent()	m_bDocFlg[DXFDOC_BINDPARENT]

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
	ON_COMMAND(ID_FILE_NCD2DXF, &CDXFDoc::OnFileDXF2DXF)
	// NC�����ƭ�
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_DXF2NCD, ID_FILE_DXF2NCD_WIRE, &CDXFDoc::OnUpdateFileDXF2NCD)
	ON_COMMAND_RANGE(ID_FILE_DXF2NCD, ID_FILE_DXF2NCD_WIRE, &CDXFDoc::OnFileDXF2NCD)
	// �`����H�w��
	ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_SHAPE_SEL, ID_EDIT_SHAPE_POC, &CDXFDoc::OnUpdateShapePattern)
	ON_COMMAND_RANGE(ID_EDIT_SHAPE_SEL, ID_EDIT_SHAPE_POC, &CDXFDoc::OnShapePattern)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �N���X�̍\�z/����

CDXFDoc::CDXFDoc()
{
	// �׸ޏ����ݒ�
	m_bDocFlg.set(DXFDOC_RELOAD);
	m_bDocFlg.set(DXFDOC_THREAD);
	m_nShapeProcessID = 0;
	// �ް�������
	ZEROCLR(m_nDataCnt);		// m_nDataCnt[i++]=0
	ZEROCLR(m_nLayerDataCnt);
	ZEROCLR(m_pLatheLine);
	m_pCircle = NULL;
	m_pParentDoc = NULL;
	m_pSerializeLayer = NULL;
	// ��޼ު�ċ�`�̏�����
	m_rcMax.SetRectMinimum();
	// �������蓖�ăT�C�Y
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
		// CreateObject()�Ő��������ݽ�ݽ��delete�s�v!!
		delete	m_bindInfo[i];
	}
}

/////////////////////////////////////////////////////////////////////////////
// հ�����ފ֐�

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

void CDXFDoc::RemoveData(CLayerData* pLayer, INT_PTR nIndex)
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
	if ( nLayerType==DXFCAMLAYER || (IsBindMode()&&nLayerType!=DXFORGLAYER) )
		SetMaxRect(pData);
}

void CDXFDoc::RemoveAt(LPCTSTR lpszLayer, INT_PTR nIndex, INT_PTR nCnt)
{
	CLayerData*	pLayer;
	if ( m_mpLayer.Lookup(lpszLayer, pLayer) ) {
		INT_PTR	n = pLayer->GetDxfSize() - nIndex;
		nCnt = min(nCnt, n);
		while ( nCnt-- )
			RemoveData( pLayer, nIndex );
	}
}

void CDXFDoc::RemoveAtText(LPCTSTR lpszLayer, INT_PTR nIndex, INT_PTR nCnt)
{
	CLayerData*	pLayer;
	if ( m_mpLayer.Lookup(lpszLayer, pLayer) ) {
		INT_PTR	n = pLayer->GetDxfTextSize() - nIndex;
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
		m_mpLayer.RemoveKey(pLayer->GetLayerName());
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
	// ڲԔz���񖳂� => MakeNCDlgEx[1|2]1����
	if ( !pArray )
		pArray = &m_obLayer;

	INT_PTR			i, j;
	const INT_PTR	nLoop = pArray->GetSize();
	CLayerData*	pLayer;
	CLayerData*	pLayerCmp;
	CString		strLayer, strFile, strCmp;

	// �ؼ���̧�قƂ̏d�������ƌʏo�͓��m�̏d������
	for ( i=0; i<nLoop; i++ ) {
		pLayer = pArray->GetAt(i);
		strFile = pLayer->GetNCFile();
		if ( !pLayer->IsLayerFlag(LAYER_CUT_TARGET) || !pLayer->IsLayerFlag(LAYER_PART_OUT) || strFile.IsEmpty() )
			continue;
		strLayer = pLayer->GetLayerName();
		// �ؼ���̧�قƂ̏d������(����޲�۸ނ̂�)
		if ( !strOrgFile.IsEmpty() ) {
			if ( strOrgFile.CompareNoCase(strFile) == 0 ) {
				AfxMessageBox(IDS_ERR_OVERLAPPINGFILE, MB_OK|MB_ICONEXCLAMATION);
				return strLayer;
			}
		}
		// �ʏo�͓��m�̏d������
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
	extern	LPCTSTR		gg_szComma;		// ","
	int		n;
	BOOL	bResult = TRUE;
	CString	strTmp, strBuf;
	TCHAR	szCurrent[_MAX_PATH];
	CLayerData*	pLayer;

	// �����ިڸ�؂� lpszFile �� -> PathSearchAndQualify()
	::GetCurrentDirectory(_MAX_PATH, szCurrent);
	::Path_Name_From_FullPath(lpszFile, strTmp/*strPath*/, strBuf/*dummy*/);
	::SetCurrentDirectory(strTmp);

	try {
		CStdioFile	fp(lpszFile,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		// �ǂݍ���ٰ��
		while ( fp.ReadString(strTmp) ) {
			strBuf = strTmp.Trim();
			if ( ::NC_IsNullLine(strBuf) )
				continue;
			// ڲԖ���������
			n = strBuf.Find(gg_szComma);
			if ( n > 0 ) {
				pLayer = GetLayerData(strBuf.Left(n).Trim());
				if ( pLayer ) {
					// ���Ƃ�CLayerData�̎d��
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

	// �����ިڸ�؂����ɖ߂�
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
		obLayer.Sort(LayerCompareFunc_Name);	// �ۑ��̂��߂����ɖ��O�����
		//
		CStdioFile	fp(lpszFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
		// ���ďo��
		BOOST_FOREACH(const auto& e, g_szLayerToInitComment) {
			fp.WriteString(e);
		}
		fp.WriteString(g_szLayerToInitComment[0]);
		// �ް��o��
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

void CDXFDoc::AllChangeFactor(float f)
{
	int		i;

	for ( i=0; i<m_obLayer.GetSize(); i++ )
		m_obLayer[i]->AllChangeFactor(f);
	if ( m_pCircle )
		m_pCircle->DrawTuning(f);
	for ( i=0; i<SIZEOF(m_pLatheLine); i++ ) {
		if ( m_pLatheLine[i] )
			m_pLatheLine[i]->DrawTuning(f);
	}
}

void CDXFDoc::AllSetDxfFlg(DWORD dwFlags, BOOL bSet)
{
	for ( int i=0; i<m_obLayer.GetSize(); i++ )
		m_obLayer[i]->AllSetDxfFlg(dwFlags, bSet);
}

void CDXFDoc::AllRoundObjPoint(float dRound)
{
	int		i, j;
	CLayerData*	pLayer;
	CDXFdata*	pData;

	CPointF	ptOrg(m_rcMax.CenterPoint());
	m_rcMax.SetRectMinimum();

	for ( i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		for ( j=0; j<pLayer->GetDxfSize(); j++ ) {
			pData = pLayer->GetDxfData(j);
			pData->RoundObjPoint(ptOrg, dRound);
			SetMaxRect(pData);
		}
		for ( j=0; j<pLayer->GetDxfTextSize(); j++ ) {
			pData = pLayer->GetDxfTextData(j);
			pData->RoundObjPoint(ptOrg, dRound);
			SetMaxRect(pData);
		}
	}
}

void CDXFDoc::CreateCutterOrigin(const CPointF& pt, float r/*=ORGRADIUS*/, BOOL bRedraw/*=FALSE*/)
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

void CDXFDoc::CreateLatheLine(const CPointF& pts, const CPointF& pte)
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

BOOL CDXFDoc::GetEditOrgPoint(LPCTSTR lpctStr, CPointF& pt)
{
	extern	LPCTSTR	gg_szCat;	// ", "
	LPTSTR	lpszNum = NULL, lpsztok, lpszcontext;
	BOOL	bResult = TRUE;
	
	// �uX,Y�v�`���̕�����𕪗�
	try {
		lpszNum = new TCHAR[lstrlen(lpctStr)+1];
		lpsztok = strtok_s(lstrcpy(lpszNum, lpctStr), gg_szCat, &lpszcontext);
		for ( int i=0; i<2 && lpsztok; i++ ) {
			pt[i] = (float)atof(boost::algorithm::trim_copy(std::string(lpsztok)).c_str());
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

tuple<CDXFshape*, CDXFdata*, float> CDXFDoc::GetSelectObject(const CPointF& pt, const CRectF& rcView)
{
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFshape*	pShapeResult = NULL;
	CDXFdata*	pData;
	CDXFdata*	pDataResult  = NULL;
	int			i, j;
	CRectF		rc;
	float		dGap, dGapMin = FLT_MAX;

	// �S�Ă̐؍��޼ު�Ă����ԋ߂��W���Ƌ������擾
	for ( i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		if ( !pLayer->IsCutType() || !pLayer->IsLayerFlag(LAYER_VIEW) )
			continue;
		for ( j=0; j<pLayer->GetShapeSize(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			// �\����`�ɏ����ł��������Ă����(�����܂�)
			if ( rc.CrossRect(rcView, pShape->GetMaxRect()) ) {
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

void CDXFDoc::AddBindInfo(LPCADBINDINFO pInfo)
{
	m_bindInfo.Add(pInfo);
	for ( int i=0; i<SIZEOF(m_nDataCnt); i++ )
		m_nDataCnt[i] += pInfo->pDoc->GetDxfDataCnt((ENDXFTYPE)i);
}

void CDXFDoc::SortBindInfo(void)
{
	m_bindInfo.Sort(BindHeightCompareFunc);
#ifdef _DEBUG
	for ( int i=0; i<m_bindInfo.GetCount(); i++ ) {
		CDXFDoc* pDoc = m_bindInfo[i]->pDoc;
		CRectF	rc( pDoc->GetMaxRect() );
		printf("No.%d=%s\n", i+1, LPCTSTR(pDoc->GetPathName()) );
		printf(" area=%f\n", rc.Width() * rc.Height() );
	}
#endif
}

void CDXFDoc::RemoveBindData(INT_PTR n)
{
	ASSERT(n>=0 && n<GetBindInfoCnt());
	m_bindInfo[n]->pView->SendMessage(WM_CLOSE);	// ���鏈�����Ȃ��Ƴ���޳���c��
	delete	m_bindInfo[n];
	m_bindInfo.RemoveAt(n);
}

void MakeDXF_Sub(const CDXFDoc* pDoc, const CPointF& pt, CDxfMakeArray& obDXFdata)
{
	INT_PTR		i, j;
	CLayerData*	pLayer;
	CDXFMake*	pMake;

	// �}�`���o��
	for ( i=0; i<pDoc->GetLayerCnt(); i++ ) {
		pLayer = pDoc->GetLayerData(i);
		for ( j=0; j<pLayer->GetDxfSize(); j++ ) {
			pMake = new CDXFMake(pLayer->GetDxfData(j), pt);
			obDXFdata.Add(pMake);
		}
		for ( j=0; j<pLayer->GetDxfTextSize(); j++ ) {
			pMake = new CDXFMake(pLayer->GetDxfTextData(j), pt);
			obDXFdata.Add(pMake);
		}
	}
}

void CDXFDoc::MakeDXF(const CString& strFile)
{
	CWaitCursor		wait;
	CDXFMakeOption	pMakeOpt(FALSE);	// ��̫�Ēl�ŏ�����
	CDxfMakeArray	obDXFdata;// DXF�o�ͲҰ��
	CDXFMake*		pMake;
	CPointF		pto;	// (0,0)
	BOOL		bResult = TRUE;
	int			i;
	CString		strMsg;

	// �ÓI�ϐ�������
	CDXFMake::SetStaticOption(&pMakeOpt);

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		// �e����ݐ���(ENTITIES�̾���ݖ��܂�)
		for ( i=SEC_HEADER; i<=SEC_ENTITIES; i++ ) {
			pMake = new CDXFMake((enSECNAME)i, this);
			obDXFdata.Add(pMake);
		}
		// ���_���o��
		if ( m_pCircle ) {
			CPoint3F	ptOrg = m_pCircle->GetCenter();
			pMake = new CDXFMake(ptOrg, m_pCircle->GetR());
			obDXFdata.Add(pMake);
		}
		// �}�`���o��
		if ( IsBindParent() ) {
			pMake = new CDXFMake(GetMaxRect());	// ܰ���`
			obDXFdata.Add(pMake);
			for ( i=0; i<m_bindInfo.GetSize(); i++ ) {
				if ( m_bindInfo[i]->bTarget ) {
					pto = m_bindInfo[i]->pt - m_bindInfo[i]->ptOffset;	// �z�u���W - �q����޳�`�挴�_
					MakeDXF_Sub(m_bindInfo[i]->pDoc, pto, obDXFdata);
				}
			}
		}
		else
			MakeDXF_Sub(this, pto, obDXFdata);
		// ENTITIES����ݏI����EOF�o��
		pMake = new CDXFMake(SEC_NOSECNAME);
		obDXFdata.Add(pMake);
		// �o��
		CStdioFile	fp(strFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
		for ( i=0; i<obDXFdata.GetSize(); i++ )
			obDXFdata[i]->WriteDXF(fp);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}
	catch (CFileException* e) {
		strMsg.Format(IDS_ERR_DATAWRITE, strFile);
		AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}

	// ��޼ު�ď���
	for ( i=0; i<obDXFdata.GetSize(); i++ )
		delete	obDXFdata[i];
	obDXFdata.RemoveAll();

	if ( bResult ) {
		strMsg.Format(IDS_ANA_FILEOUTPUT, strFile);
		AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �f�f

#ifdef _DEBUG
void CDXFDoc::DbgSerializeInfo(void)
{
	printf("CDXFDoc::DbgSerializeInfo()\n");
	printf("m_rcMax left =%f right =%f\n", m_rcMax.left, m_rcMax.right);
	printf("        top  =%f bottom=%f\n", m_rcMax.top, m_rcMax.bottom);
	printf("        Width=%f Height=%f\n", m_rcMax.Width(), m_rcMax.Height());
	printf("        CenterPoint() x=%f y=%f\n", m_rcMax.CenterPoint().x, m_rcMax.CenterPoint().y);
	if ( m_pCircle ) {
		printf("m_ptOrg x=%f y=%f R=%f\n",
			m_pCircle->GetCenter().x, m_pCircle->GetCenter().y,
			m_pCircle->GetR() );
	}
	else {
		printf("m_ptOrg ???\n");
	}
	CLayerData*	pLayer;
	for ( int i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		printf("LayerName=%s DataCnt=%d TextCnt=%d\n",
			LPCTSTR(pLayer->GetLayerName()), pLayer->GetDxfSize(), pLayer->GetDxfTextSize());
	}
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �N���X�̃I�[�o���C�h�֐�

BOOL CDXFDoc::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if ( IsBindMode() ) {
		CDXFDoc* pDoc = GetBindParentDoc();
		return pDoc ? pDoc->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) : FALSE;
	}

	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CDXFDoc::OnNewDocument()
{
	const CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	// CNCVCApp::OnFileCADbind() CAD�ް��̓�������̂݌Ă΂��
	// �ƭ�����ނ̐V�K�쐬�͖���
	m_rcMax.left = m_rcMax.top = 0;		// zero clear
	m_rcMax.right	= pOpt->GetBindSize(NCA_X);
	m_rcMax.bottom	= pOpt->GetBindSize(NCA_Y);
	// ���_����
	CPointF		pt;
	switch ( pOpt->GetDxfOptNum(DXFOPT_BINDORG) ) {
	case 0:		// �E��
		pt = m_rcMax.BottomRight();
		break;
	case 1:		// �E��
		pt.x = m_rcMax.right;
		pt.y = m_rcMax.top;
		break;
	case 2:		// ����
		pt.x = m_rcMax.left;
		pt.y = m_rcMax.bottom;
		break;
	case 3:		// ����
		pt = m_rcMax.TopLeft();
		break;
	default:	// ����
		pt = m_rcMax.CenterPoint();
		break;
	}
	CreateCutterOrigin(pt);
	SetDocFlag(DXFDOC_BINDPARENT);	// ����Ӱ��(�e)�ɐݒ�
	SetDocFlag(DXFDOC_RELOAD, FALSE);

	return __super::OnNewDocument();
}

BOOL CDXFDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	BOOL	bResult;
	INT_PTR	i, nCnt;
	PFNNCVCSERIALIZEFUNC	pSerialFunc = AfxGetNCVCApp()->GetSerializeFunc();

	if ( pSerialFunc ) {
		// ��޲ݼري֐���ۑ��Ḑ�ٕύX�ʒm�ȂǂɎg�p
		m_pfnSerialFunc = pSerialFunc;	// DocBase.h
		// ��޲݂̼ري֐����Ăяo��
		bResult = (*pSerialFunc)(this, lpszPathName);
		// �ري֐��̏�����
		AfxGetNCVCApp()->SetSerializeFunc((PFNNCVCSERIALIZEFUNC)NULL);
	}
	else {
		// �ʏ�̼ري֐��Ăяo��
		bResult = __super::OnOpenDocument(lpszPathName);
	}

	// NC�����\���ǂ���������
	if ( bResult && !IsBindParent() ) {
#ifdef _DEBUG
		DbgSerializeInfo();
#endif
		// �؍��ް����Ȃ��Ƃ�
		for ( i=nCnt=0; i<m_obLayer.GetSize(); i++ ) {
			CLayerData* pLayer = m_obLayer[i];
			nCnt += pLayer->GetDxfSize();	// CAM���C���̒P�ƃe�L�X�g�̈���
//			nCnt += pLayer->GetDxfSize() + pLayer->GetDxfTextSize();
		}
		if ( nCnt < 1 ) {
			if ( !IsBindMode() )
				AfxMessageBox(IDS_ERR_DXFDATACUT, MB_OK|MB_ICONEXCLAMATION);
			bResult = FALSE;
		}
		else if ( m_pCircle ) {
			if ( !m_ptOrgOrig )		// �رُ��Ɋ܂܂�Ă���(CAM̧��)�ꍇ������
				m_ptOrgOrig = m_pCircle->GetCenter();	// ̧�ق���̵ؼ��ٌ��_��ۑ�
			m_bDocFlg.set(DXFDOC_READY);	// ����OK!
		}
		else if ( !IsBindMode() ) {
			// ���_�ް����Ȃ��Ƃ�(��`�̏㉺�ɒ���)
			optional<CPointF>	pt;
			CPointF				ptRC;	// optional�^�ւ̑���p
			switch ( AfxGetNCVCApp()->GetDXFOption()->GetDxfOptNum(DXFOPT_ORGTYPE) ) {
			case 1:	// �E��
				pt = m_rcMax.BottomRight();
				break;
			case 2:	// �E��
				ptRC.x = m_rcMax.right;
				ptRC.y = m_rcMax.top;
				pt = ptRC;
				break;
			case 3:	// ����
				ptRC.x = m_rcMax.left;
				ptRC.y = m_rcMax.bottom;
				pt = ptRC;
				break;
			case 4:	// ����
				pt = m_rcMax.TopLeft();
				break;
			case 5:	// ����
				pt = m_rcMax.CenterPoint();
				break;
			default:
				AfxMessageBox(IDS_ERR_DXFDATAORG, MB_OK|MB_ICONEXCLAMATION);
				// OnOpenDocument() �߂�l bResult �͂��̂܂܂ŗǂ�
				break;
			}
			if ( pt ) {
				CreateCutterOrigin(*pt);
				m_bDocFlg.set(DXFDOC_READY);	// ����OK!
			}
		}
	}

	if ( bResult && !IsBindMode() ) {
		if ( m_bDocFlg[DXFDOC_READY] ) {
			// �����ް������\���ǂ���������
			if ( m_pLatheLine[0] && m_pLatheLine[1] ) {
				m_bDocFlg.set(DXFDOC_LATHE);
				// �O�a��\������[0]�ɁA�i�ŏ�X���W�����µ�޼ު�āj
				// �[�ʂ�\������[1]��
				CPointF	pt1, pt2;
				float	minX = FLT_MAX;
				INT_PTR	j, n = 0;
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
				// �O�a��޼ު�Ē���(X�l�̑傫�������n�_��)
				pt1 = m_pLatheLine[0]->GetNativePoint(0);
				pt2 = m_pLatheLine[0]->GetNativePoint(1);
				if ( pt1.x < pt2.x )
					m_pLatheLine[0]->SwapNativePt();
				// �[�ʵ�޼ު�Ē���(Y�l�̑傫�������n�_��)
				pt1 = m_pLatheLine[1]->GetNativePoint(0);
				pt2 = m_pLatheLine[1]->GetNativePoint(1);
				if ( pt1.y < pt2.y )
					m_pLatheLine[1]->SwapNativePt();
			}
			// ܲԉ��H�@�ް������\���ǂ���������
			if ( GetCutLayerCnt() <= 2 )
				m_bDocFlg.set(DXFDOC_WIRE);
		}
		// �޷���ĕύX�ʒm�گ�ނ̐���
		POSITION	pos = GetFirstViewPosition();
		ASSERT( pos );
		OnOpenDocumentSP(lpszPathName, GetNextView(pos)->GetParentFrame());	// CDocBase
	}

	// Ҳ��ڰт���۸�ڽ�ް������
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);

	if ( IsBindParent() ) {
		// �ð���ް�̕\���X�V
		POSITION	pos = GetFirstViewPosition();
		ASSERT( pos );
		CView*		pViewParent = GetNextView(pos);
		pViewParent->PostMessage(WM_USERBINDINIT, 1);
	}

	return bResult;
}

BOOL CDXFDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// �e�ޭ��ɕۑ��ʒm
	UpdateAllViews(NULL, UAV_FILESAVE);	// CDXFShapeView�����X�V

	// �޷���ĕύX�ʒm�گ�ނ̏I��
	OnCloseDocumentSP();	// CDocBase

	if ( GetPathName().CompareNoCase(lpszPathName) != 0 ) {
		CDocument* pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_DXF, lpszPathName);
		if ( pDoc )
			pDoc->OnCloseDocument();	// ���ɊJ���Ă����޷���Ă����
	}

	// �ۑ�����
	BOOL bResult = __super::OnSaveDocument(lpszPathName);

	// �޷���ĕύX�ʒm�گ�ނ̐���
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
	printf("CDXFDoc::OnCloseDocument() Start\n");
#endif
	// ۯ���޲݂�����
	if ( !IsLockThread() ) {	// CDocBase
#ifdef _DEBUG
		printf("AddinLock FALSE\n");
#endif
		return;
	}
	// �������̽گ�ނ𒆒f������
	OnCloseDocumentSP();	// ̧�ٕύX�ʒm�گ��
	m_bDocFlg.reset(DXFDOC_THREAD);
	m_csRestoreCircleType.Lock();
	m_csRestoreCircleType.Unlock();
#ifdef _DEBUG
	printf("m_csRestoreCircleType Unlock OK\n");
#endif

	__super::OnCloseDocument();
}

BOOL CDXFDoc::SaveModified()
{
	BOOL	bResult;
	CString	strExt( AfxGetNCVCApp()->GetDocExtString(TYPE_DXF) ),
			strOrigPath( GetPathName() );

	// CAD�ް��ōX�V���������ꍇ
	if ( IsModified() && strExt.CompareNoCase(strOrigPath.Right(4)) != 0 ) {
		CString	strPath, strName, strMsg;
		::Path_Name_From_FullPath(strOrigPath, strPath, strName);
		strMsg.Format(IDS_ANA_VALUECHANGE, strName);
		switch ( AfxMessageBox(strMsg, MB_YESNOCANCEL|MB_ICONQUESTION) ) {
		case IDCANCEL:
			bResult = FALSE;
			break;
		case IDYES:
			OnFileSaveAs();		// cam�ŕۑ��₢���킹
			// through
		default:	// case IDNO:
			bResult = TRUE;
			break;
		}
	}
	else {
		// ��̫�Ă̕ۑ��₢���킹����
		bResult = __super::SaveModified();
	}

	return bResult;
}

void CDXFDoc::UpdateFrameCounts()
{
	if ( !IsBindMode() ) {
/*
		// �ڰт̐���}�����邱�Ƃų���޳���ق́u:1�v��t�^���Ȃ��悤�ɂ���
		CFrameWnd*	pFrame;
		for ( POSITION pos=GetFirstViewPosition(); pos; ) {
			pFrame = GetNextView(pos)->GetParentFrame();
			if ( pFrame ) {
				pFrame->m_nWindow = 0;
				pFrame->OnUpdateFrameTitle(TRUE);
				break;
			}
		}
*/
		// �q���޷���Ă���è�ނɂȂ���
		// �ڰт�̧�ٖ��������錻�ۂւ̑Ή�
		POSITION pos = GetFirstViewPosition();
		if ( pos ) {
			CView*	pView = GetNextView(pos);
			if ( pView ) {
				CFrameWnd* pFrame = pView->GetParentFrame();
				if ( pFrame ) {
					pFrame->SetActiveView(pView, FALSE);
					__super::UpdateFrameCounts();
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �V���A���C�Y

void CDXFDoc::Serialize(CArchive& ar)
{
	extern	DWORD	g_dwCamVer;		// NCVC.cpp

	// DxfSetupReload������OFF
	m_bDocFlg.reset(DXFDOC_RELOAD);

	int			i, j, nLoopCnt;
	BYTE		nByte = 0;
	BOOL		bReady, bShape;
	CPointF		pt;
	CDXFDoc*	pDoc = static_cast<CDXFDoc*>(ar.m_pDocument);
	CLayerData*	pLayer;

	// �������g���e�A�܂��́A�Ăяo�������e�łȂ�
	if ( IsBindParent() || !pDoc->IsDocFlag(DXFDOC_BINDPARENT) ) {
		// ͯ�ް���
		CCAMHead	cam;
		cam.Serialize(ar);
	}

	if ( ar.IsStoring() ) {
		// �e����
		bReady = m_bDocFlg[DXFDOC_READY];
		bShape = m_bDocFlg[DXFDOC_SHAPE];
		ar << bReady << bShape;
		ar << m_AutoWork.nSelect << m_AutoWork.dOffset << m_AutoWork.bAcuteRound <<
			m_AutoWork.nLoopCnt << m_AutoWork.nScanLine << m_AutoWork.bCircleScroll;
		// NC����̧�ٖ�
		ar << m_strNCFileName;
		// ���_��޼ު��
		if ( m_pCircle ) {
			ar << (BYTE)1;
			ar.WriteObject(m_pCircle);
		}
		else
			ar << (BYTE)0;
		// CAD�ؼ��ٌ��_
		if ( m_ptOrgOrig ) {
			ar << (BYTE)1;
			pt = *m_ptOrgOrig;
			ar << pt.x << pt.y;
		}
		else
			ar << (BYTE)0;
		// ���Ռ��_
		if ( m_pLatheLine[0] )
			nByte++;
		if ( m_pLatheLine[1] )
			nByte++;
		ar << nByte;
		for ( i=0; i<SIZEOF(m_pLatheLine); i++ ) {
			if ( m_pLatheLine[i] )
				ar.WriteObject(m_pLatheLine[i]);
		}
		// �������
		ar << m_rcMax.left << m_rcMax.top << m_rcMax.right << m_rcMax.bottom;
		nLoopCnt = (int)(m_bindInfo.GetSize());
		ar << nLoopCnt;
		if ( nLoopCnt > 0 ) {
			LPCADBINDINFO	pInfo;
			for ( i=0; i<nLoopCnt; i++ ) {
				ar.m_pDocument = this;	// �e�q�̋�ʂ��ω����Ȃ��悤��
				pInfo = m_bindInfo[i];
				ar << (BYTE)(pInfo->bTarget ? 1 : 0);
				ar << pInfo->pDoc->GetPathName();
				ar << pInfo->pt.x << pInfo->pt.y << pInfo->ptOffset.x << pInfo->ptOffset.y;
				pInfo->pDoc->Serialize(ar);
			}
		}
		else {
			// ڲԏ��(��DXF�ް�)�̕ۑ�
			m_obLayer.Serialize(ar);
		}
		return;		//�ۑ��͂����܂�
	}

	// �e����
	ar >> bReady >> bShape;
	m_bDocFlg.set(DXFDOC_READY, bReady);
	m_bDocFlg.set(DXFDOC_SHAPE, bShape);
	if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
		double	dOffset;
		if ( g_dwCamVer >= NCVCSERIALVERSION_1700 ) {	// Ver1.70�`
			ar >> m_AutoWork.nSelect >> dOffset >> m_AutoWork.bAcuteRound >>
				m_AutoWork.nLoopCnt >> m_AutoWork.nScanLine >> m_AutoWork.bCircleScroll;
		}
		else {
			ar >> dOffset;
			if ( g_dwCamVer >= NCVCSERIALVERSION_1505 )	// Ver1.10�`
				ar >> m_AutoWork.bAcuteRound;
		}
		m_AutoWork.dOffset = (float)dOffset;
	}
	else {
		ar >> m_AutoWork.nSelect >> m_AutoWork.dOffset >> m_AutoWork.bAcuteRound >>
			m_AutoWork.nLoopCnt >> m_AutoWork.nScanLine >> m_AutoWork.bCircleScroll;
	}
	// NC����̧�ٖ�
	ar >> m_strNCFileName;
	// ���_��޼ު��
	ar >> nByte;
	if ( nByte > 0 )
		m_pCircle = static_cast<CDXFcircleEx*>(ar.ReadObject(RUNTIME_CLASS(CDXFcircleEx)));
	// CAD�ؼ��ٌ��_(Ver1.10a�`)
	if ( g_dwCamVer >= NCVCSERIALVERSION_1507 )
		ar >> nByte;
	else
		nByte = 1;		// �����ǂݍ���
	if ( nByte > 0 ) {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CPointD	ptD;
			ar >> ptD.x >> ptD.y;
			pt = ptD;
		}
		else {
			ar >> pt.x >> pt.y;
		}
		m_ptOrgOrig = pt;
	}
	// ���Ռ��_(Ver2.30�`)
	if ( g_dwCamVer >= NCVCSERIALVERSION_2300 ) {
		ar >> nByte;
		for ( i=0; i<nByte && i<SIZEOF(m_pLatheLine); i++ )
			m_pLatheLine[i] = static_cast<CDXFline*>(ar.ReadObject(RUNTIME_CLASS(CDXFline)));
		for ( ; i<SIZEOF(m_pLatheLine); i++ )
			m_pLatheLine[i] = NULL;
	}
	// �������(Ver3.60�`)
	if ( g_dwCamVer >= NCVCSERIALVERSION_3600 ) {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CRect3D	rc;
			ar >> rc.left >> rc.top >> rc.right >> rc.bottom;
			m_rcMax = rc;
		}
		else {
			ar >> m_rcMax.left >> m_rcMax.top >> m_rcMax.right >> m_rcMax.bottom;
		}
		ar >> nLoopCnt;
	}
	else
		nLoopCnt = 0;
	if ( nLoopCnt > 0 ) {
		SetDocFlag(DXFDOC_BINDPARENT);
		CString			strPath;
		LPCADBINDINFO	pInfo;
		CDXFView*		pView;
		CRect			rc;
		CPointF			ptOffset;
		BOOL			bResult;
		POSITION	pos = GetFirstViewPosition();
		ASSERT( pos );
		CView*		pViewParent = GetNextView(pos);
		ASSERT( pViewParent );
		for ( i=0; i<nLoopCnt; i++ ) {
			// �Ώ��׸�
			ar >> nByte;
			// ̧�ٖ�
			ar >> strPath;
			// �z�u���W
			if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
				CPointD	pt1, pt2;
				ar >> pt1.x >> pt1.y >> pt2.x >> pt2.y;
				pt = pt1;
				ptOffset = pt2;
			}
			else {
				ar >> pt.x >> pt.y >> ptOffset.x >> ptOffset.y;
			}
			// �q�޷���Ă̐���
			pDoc  = static_cast<CDXFDoc*>(RUNTIME_CLASS(CDXFDoc)->CreateObject());
			if ( pDoc ) {
				pDoc->SetPathName(strPath, FALSE);
				ar.m_pDocument = this;
				pDoc->SetDocFlag(DXFDOC_BIND);
				pDoc->SetBindParentDoc(this);
				pDoc->Serialize(ar);
				// �q�ޭ��̐���
				pView = static_cast<CDXFView*>(RUNTIME_CLASS(CDXFView)->CreateObject());
				if ( pView ) {
					if ( bResult=pView->Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, rc, pViewParent, AFX_IDW_PANE_FIRST+i+1) ) {
						pDoc->AddView(pView);
						pView->OnInitialUpdate();
					}
					else {
						pView->DestroyWindow();
						pView = NULL;
					}
				}
				else
					bResult = FALSE;
				if ( bResult ) {
					pInfo = new CADBINDINFO(nByte ? TRUE : FALSE, pDoc, pView, pt, ptOffset);
					AddBindInfo(pInfo);
				}
				else
					delete	pDoc;
			}
		}
	}
	else {
		// ڲԏ��(��DXF�ް�)�̓ǂݍ���
		m_obLayer.Serialize(ar);
	}

	// --- �ް��ǂݍ��݌�̏���

	// NC����̧�ق��߽����
	if ( !m_strNCFileName.IsEmpty() ) {
		CString	strPath, strFile;
		::Path_Name_From_FullPath(m_strNCFileName, strPath, strFile);
		if ( !::PathFileExists(strPath) )
			m_strNCFileName.Empty();
	}

	// ڲԏ�񏈗�
	CDXFdata*	pData;
	ENDXFTYPE	enType;
	for ( i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		// �ʏ�z�񂩂�ڲԖ���ϯ��ݸ�
		m_mpLayer.SetAt(pLayer->GetLayerName(), pLayer);
		// �ް��̶��ĂƐ�L�̈�̐ݒ�
		nLoopCnt = (int)(pLayer->GetDxfSize());
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
// CDXFDoc �R�}���h

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

	// ���_�ړ�����
	optional<CPointF>	pt;
	CPointF				ptOffset;
	switch ( dlg.m_nSelect ) {
	case 0:		// ���l�ړ�
		if ( GetEditOrgPoint(dlg.m_strNumeric, ptOffset) )
			pt = m_pCircle->GetCenter() + ptOffset;
		break;
	case 1:		// ��`�ړ�
		switch ( dlg.m_nRectType ) {
		case 0:	// �E��
			pt = m_rcMax.BottomRight();
			break;
		case 1:	// �E��
			ptOffset.x = m_rcMax.right;
			ptOffset.y = m_rcMax.top;
			pt = ptOffset;
			break;
		case 2:	// ����
			ptOffset.x = m_rcMax.left;
			ptOffset.y = m_rcMax.bottom;
			pt = ptOffset;
			break;
		case 3:	// ����
			pt = m_rcMax.TopLeft();
			break;
		case 4:	// ����
			pt = m_rcMax.CenterPoint();
			break;
		}
		break;
	case 2:		// �ؼ��ٌ��_�ɖ߂�
		pt = m_ptOrgOrig;
		break;
	}

	if ( pt ) {
		CreateCutterOrigin(*pt, ORGRADIUS, TRUE);
		m_bDocFlg.set(DXFDOC_READY);	// ����OK!
	}
}

void CDXFDoc::OnUpdateEditShape(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bDocFlg[DXFDOC_LATHE]||IsBindParent() ?
		FALSE : !m_bDocFlg[DXFDOC_SHAPE]);
}

void CDXFDoc::OnEditShape() 
{
	// �󋵈ē��޲�۸�(�����گ�ސ���)
	CThreadDlg	dlgThread(ID_EDIT_DXFSHAPE, this);
	if ( dlgThread.DoModal() == IDOK ) {
		m_bDocFlg.set(DXFDOC_SHAPE);
		// ���د�����޳���L���� + DXFView ��̨��ү���ޑ��M
		static_cast<CDXFChild *>(AfxGetNCVCMainWnd()->MDIGetActive())->ShowShapeView();
		// DXFShapeView�X�V
		UpdateAllViews(NULL, UAV_DXFSHAPEUPDATE);
		// �X�V�׸�
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

	// �̾�ď����l�̍X�V
	int	i, j;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	for ( i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		if ( !pLayer->IsCutType() )
			continue;
		for ( j=0; j<pLayer->GetShapeSize(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			// ���������Ώۂ��ۂ�(CDXFchain* ������ΏۂƂ���)
			if ( pShape->GetShapeType()!=DXFSHAPETYPE_CHAIN || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
				continue;
			// �e��ݒ�̔��f
			pShape->SetOffset(m_AutoWork.dOffset);
			pShape->SetAcuteRound(m_AutoWork.bAcuteRound);
		}
	}

	// ���ݓo�^����Ă�����H�w�����폜
	UpdateAllViews(NULL, UAV_DXFAUTODELWORKING);

	// �������H�w��
	CThreadDlg	dlgThread(ID_EDIT_SHAPE_AUTO, this,
					AUTOWORKING, (LPARAM)&m_AutoWork);
	if ( dlgThread.DoModal() == IDOK )
		UpdateAllViews(NULL, UAV_DXFAUTOWORKING);	// �������H�w���o�^(�ƍĕ`��)
	else
		UpdateAllViews(NULL);						// �ĕ`��̂�

	// �X�V�׸�
	SetModifiedFlag();
}

void CDXFDoc::OnEditStrictOffset()
{
	if ( AfxMessageBox(IDS_ANA_STRICTOFFSET, MB_YESNO|MB_ICONQUESTION) != IDYES )
		return;

	//	�̾�ĵ�޼ު�ē��m�̌�_���������A�����ȵ̾�Ă��v�Z����
	CThreadDlg	dlgThread(ID_EDIT_SHAPE_AUTO, this,
					AUTOSTRICTOFFSET);
	if ( dlgThread.DoModal() == IDOK ) {
		UpdateAllViews(NULL, UAV_DXFAUTOWORKING,
			reinterpret_cast<CObject *>(TRUE));	// �ĕ`��̂�
		SetModifiedFlag();
	}
}

void CDXFDoc::OnUpdateEditShaping(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bDocFlg[DXFDOC_SHAPE]);
}

void CDXFDoc::OnUpdateShapePattern(CCmdUI* pCmdUI)
{
	if ( pCmdUI->m_nID == ID_EDIT_SHAPE_POC )	// �߹�ď����͖�����
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(m_bDocFlg[DXFDOC_SHAPE]);

	if ( m_bDocFlg[DXFDOC_SHAPE] )
		pCmdUI->SetCheck( pCmdUI->m_nID == m_nShapeProcessID );
}

void CDXFDoc::OnShapePattern(UINT nID)
{
	if ( m_nShapeProcessID != nID ) {
		// �e�ޭ��ւ̒ʒm���ɂ��Ȃ��ƁA���`��������Ȃ�
		UpdateAllViews(NULL, UAV_DXFSHAPEID);	// to CDXFView::CancelForSelect()
		// ID�̐؂�ւ�
		m_nShapeProcessID = nID;
	}
	else
		m_nShapeProcessID = 0;	// ���H�w��������
}

void CDXFDoc::OnFileDXF2DXF() 
{
	CString	strPath, strFile, strExt, strNewFile;
	if ( IsBindParent() && GetPathName().IsEmpty() ) {
		// ��������
		ASSERT( GetBindInfoCnt() >= 0 );
		::Path_Name_From_FullPath(GetBindInfoData(0)->pDoc->GetPathName(), strNewFile, strFile);
	}
	else {
		::Path_Name_From_FullPath(GetPathName(), strPath, strFile, FALSE);
		VERIFY(strExt.LoadString(IDS_DXF_FILTER));
		strNewFile = strPath + strFile + '.' + strExt.Left(3);	// .dxf
	}
	if ( ::NCVC_FileDlgCommon(AFX_IDS_SAVEFILE, IDS_DXF_FILTER, TRUE, strNewFile, NULL,
				FALSE, OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT) == IDOK )
		MakeDXF(strNewFile);
}

void CDXFDoc::OnUpdateFileDXF2NCD(CCmdUI* pCmdUI) 
{
	BOOL	bEnable = TRUE;
	// �װ�ް��̏ꍇ��NC�ϊ��ƭ��𖳌��ɂ���
	if ( !m_bDocFlg[DXFDOC_READY] || !m_pCircle )
		bEnable = FALSE;
	else {
		switch ( pCmdUI->m_nID ){
		case ID_FILE_DXF2NCD:	// �Ȃ���default�Ɉ���������...
			break;		// OK
		case ID_FILE_DXF2NCD_SHAPE:
			bEnable = m_bDocFlg[DXFDOC_SHAPE];	// �`�󏈗����ς�ł��邩
			break;
		case ID_FILE_DXF2NCD_LATHE:
			bEnable = m_bDocFlg[DXFDOC_LATHE];	// ���՗p���_��ǂݍ��񂾂�
			break;
		case ID_FILE_DXF2NCD_WIRE:
			bEnable = m_bDocFlg[DXFDOC_WIRE];	// ܲԉ��H�@�̏����𖞂����Ă��邩
			break;
		default:	// �g������
			if ( GetCutLayerCnt() <= 1 )		// �P��ڲԂ̏ꍇ��off
				bEnable = FALSE;
			break;
		}
	}

	pCmdUI->Enable(bEnable);
}

void CDXFDoc::OnFileDXF2NCD(UINT nID) 
{
	int			i;
	enMAKETYPE	enType;
	BOOL	bNCView, bAllOut,
			bSingle = (nID==ID_FILE_DXF2NCD || nID==ID_FILE_DXF2NCD_SHAPE ||
						nID==ID_FILE_DXF2NCD_LATHE || nID==ID_FILE_DXF2NCD_WIRE);
	CDocument*	pDoc;
	CLayerData* pLayer;
	CString		strInit, strLayerFile;
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();

	if ( nID == ID_FILE_DXF2NCD_SHAPE ) {
		// �SڲԂ�Ώۂ�
		for ( i=0; i<m_obLayer.GetSize(); i++ )
			m_obLayer[i]->SetCutTargetFlg(TRUE);
	}
	else {
		// ���݂̕\���󋵂őΏ��׸ނ�u��
		for ( i=0; i<m_obLayer.GetSize(); i++ )
			m_obLayer[i]->SetCutTargetFlg_fromView();
	}

	switch ( nID ) {
	case ID_FILE_DXF2NCD_EX1:	// ڲԂ��Ƃ̕�������
	case ID_FILE_DXF2NCD_EX2:	// ڲԂ��Ƃ�Z���W
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

	case ID_FILE_DXF2NCD:		// �W������
	case ID_FILE_DXF2NCD_SHAPE:	// �`�󏈗�
	case ID_FILE_DXF2NCD_LATHE:	// �����ް�����
	case ID_FILE_DXF2NCD_WIRE:	// ܲԉ��H�@�ް�����
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

	// �ݒ�̕ۑ�
	if ( !strInit.IsEmpty() ) {
		pOpt->AddInitHistory(enType, strInit);
	}
	if ( !strLayerFile.IsEmpty() ) {
		if ( SaveLayerMap(strLayerFile) )
			pOpt->AddInitHistory(NCMAKELAYER, strLayerFile);
	}
	pOpt->SetViewFlag(bNCView);

	// ���łɊJ���Ă����޷���ĂȂ����ųݽ
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
		// ڲԏ��̕��בւ�
		m_obLayer.Sort(LayerCompareFunc_CutNo);
	}

	// CDXFCircle�̌����H�Ώ��ް������ɖ߂��گ�ނ̏I���҂�
	m_csRestoreCircleType.Lock();
	m_csRestoreCircleType.Unlock();

	// �󋵈ē��޲�۸�(�ϊ��گ�ސ���)
	CThreadDlg*	pDlg;
	INT_PTR		nResult = IDOK;
	if ( IsBindParent() ) {
		// ID_FILE_DXF2NCD -> ID_FILE_DXF2NCD_SHAPE
		// ID_FILE_DXF2NCD_WIRE -> ���̂܂�
		// ����2����݂̂�
		if ( nID == ID_FILE_DXF2NCD )
			nID = ID_FILE_DXF2NCD_SHAPE;
		int	nLoop = (int)(m_bindInfo.GetSize());
		CDXFDoc* pChildDoc;
		if ( nLoop == 1 ) {
			if ( m_bindInfo[0]->bTarget ) {
				pChildDoc = m_bindInfo[0]->pDoc;
				pChildDoc->m_strNCFileName = m_strNCFileName;
				pDlg = new CThreadDlg(ID_FILE_DXF2NCD, pChildDoc,
								(WPARAM)nID, (LPARAM)-2);
				nResult = pDlg->DoModal();
				// CDXFCircle�̌����H�Ώ��ް������ɖ߂�
				AfxBeginThread(RestoreCircleTypeThread, pChildDoc,
					THREAD_PRIORITY_LOWEST);
				delete	pDlg;
			}
			else
				nResult = IDCANCEL;
		}
		else {
			// ���_�ɋ߂����ŕ��בւ�
			switch ( pOpt->GetDxfOptNum(DXFOPT_BINDSORT) ) {
			case 1:		// ���ォ��E��
				m_bindInfo.Sort(BindMakeSeqFunc1);
				break;
			case 2:		// ���ォ�牺��
				m_bindInfo.Sort(BindMakeSeqFunc2);
				break;
			case 3:		// �E�ォ�獶��
				m_bindInfo.Sort(BindMakeSeqFunc3);
				break;
			case 4:		// �E�ォ�牺��
				m_bindInfo.Sort(BindMakeSeqFunc4);
				break;
			case 5:		// ��������E��
				m_bindInfo.Sort(BindMakeSeqFunc5);
				break;
			case 6:		// ����������
				m_bindInfo.Sort(BindMakeSeqFunc6);
				break;
			case 7:		// �E�����獶��
				m_bindInfo.Sort(BindMakeSeqFunc7);
				break;
			case 8:		// �E��������
				m_bindInfo.Sort(BindMakeSeqFunc8);
				break;
			default:	// ���_�ɋ߂���
				g_ptCutterOrg = m_pCircle->GetCenter();
				m_bindInfo.Sort(BindMakeSeqFunc0);
			}
			for ( i=0; i<nLoop && nResult==IDOK; i++ ) {
				if ( m_bindInfo[i]->bTarget ) {
					pChildDoc = m_bindInfo[i]->pDoc;
					pChildDoc->m_strNCFileName = m_strNCFileName;
					pDlg = new CThreadDlg(ID_FILE_DXF2NCD, pChildDoc,
								(WPARAM)nID,
								(LPARAM)(i<nLoop-1 ? i+1 : -1));
					nResult = pDlg->DoModal();
					AfxBeginThread(RestoreCircleTypeThread, pChildDoc,
						THREAD_PRIORITY_LOWEST);
					delete	pDlg;
				}
			}
		}
	}
	else {
		pDlg = new CThreadDlg(ID_FILE_DXF2NCD, this, (WPARAM)nID);
		nResult = pDlg->DoModal();
		AfxBeginThread(RestoreCircleTypeThread, this,
			THREAD_PRIORITY_IDLE);
		delete	pDlg;
	}

	if ( nResult != IDOK )
		return;
	// �uNC������ɊJ���v�������̏ꍇ�����o�͊���ү����
	if ( !bNCView && bAllOut ) {
		CString	strMsg;
		strMsg.Format(IDS_ANA_FILEOUTPUT, m_strNCFileName);
		AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
		return;
	}

	// NC��������ް����J��
	if ( bSingle ) {
		AfxGetNCVCApp()->OpenDocumentFile(m_strNCFileName);
	}
	else {
		bAllOut = FALSE;	// m_strNCFileName ���J�����ǂ���
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
	// �g���q�� .cam ���ۂ�
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
// CDXFDoc �T�u�X���b�h

UINT CDXFDoc::RestoreCircleTypeThread(LPVOID pParam)
{
#ifdef _DEBUG
	printf("RestoreCircleFlagThread() Start\n");
#endif
	int			i, j;
	ENDXFTYPE	enType;
	CDXFDoc*	pDoc = reinterpret_cast<CDXFDoc*>(pParam);
	CDXFdata*	pData;
	CLayerData*	pLayer;

	pDoc->m_csRestoreCircleType.Lock();		// �گ�ޏI���܂�ۯ�
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
// ���בւ��⏕�֐�

int LayerCompareFunc_CutNo(CLayerData* pFirst, CLayerData* pSecond)
{
	return pFirst->GetLayerListNo() - pSecond->GetLayerListNo();
}

int LayerCompareFunc_Name(CLayerData* pFirst, CLayerData* pSecond)
{
	return pFirst->GetLayerName().Compare( pSecond->GetLayerName() );
}

int BindHeightCompareFunc(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	CRectF	rc1( pFirst->pDoc->GetMaxRect() ),
			rc2( pSecond->pDoc->GetMaxRect() );
	float	h1 = rc1.Height(), h2 = rc2.Height(),
			w1 = rc1.Width(),  w2 = rc2.Width();
	int		nResult;
	if ( h1 < h2 )
		nResult = 1;
	else if ( h1 > h2 )
		nResult = -1;
	else {
		if ( w1 < w2 )
			nResult = 1;
		else if ( w1 > w2 )
			nResult = -1;
		else
			nResult = 0;
	}
	return nResult;
}

int BindMakeSeqFunc0(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	// ���H���_�ɋ߂���
	CPointF	ptDiff1(pFirst->pt  - g_ptCutterOrg),
			ptDiff2(pSecond->pt - g_ptCutterOrg);
	float	diff1 = ptDiff1.hypot(),
			diff2 = ptDiff2.hypot();
	if ( diff1 < diff2 )
		nResult = -1;
	else if ( diff1 > diff2 )
		nResult = 1;
	else
		nResult = 0;

	return nResult;
}

int BindMakeSeqFunc1(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	// ���ォ��E��
	if ( pFirst->pt.y < pSecond->pt.y )
		nResult = 1;
	else if ( pFirst->pt.y > pSecond->pt.y )
		nResult = -1;
	else {
		if ( pFirst->pt.x < pSecond->pt.x )
			nResult = -1;
		else if ( pFirst->pt.x > pSecond->pt.x )
			nResult = 1;
		else
			nResult = 0;
	}

	return nResult;
}

int BindMakeSeqFunc2(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	// ���ォ�牺��
	if ( pFirst->pt.x < pSecond->pt.x )
		nResult = -1;
	else if ( pFirst->pt.x > pSecond->pt.x )
		nResult = 1;
	else {
		if ( pFirst->pt.y < pSecond->pt.y )
			nResult = 1;
		else if ( pFirst->pt.y > pSecond->pt.y )
			nResult = -1;
		else
			nResult = 0;
	}

	return nResult;
}

int BindMakeSeqFunc3(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	CRectF	rc1(pFirst->pDoc->GetMaxRect()),
			rc2(pSecond->pDoc->GetMaxRect());
	CPointF	pt1(pFirst->pt.x +rc1.Width(), pFirst->pt.y),
			pt2(pSecond->pt.x+rc2.Width(), pSecond->pt.y);
	// �E�ォ�獶��
	if ( pt1.y < pt2.y )
		nResult = 1;
	else if ( pt1.y > pt2.y )
		nResult = -1;
	else {
		if ( pt1.x < pt2.x )
			nResult = 1;
		else if ( pt1.x > pt2.x )
			nResult = -1;
		else
			nResult = 0;
	}

	return nResult;
}

int BindMakeSeqFunc4(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	CRectF	rc1(pFirst->pDoc->GetMaxRect()),
			rc2(pSecond->pDoc->GetMaxRect());
	CPointF	pt1(pFirst->pt.x +rc1.Width(), pFirst->pt.y),
			pt2(pSecond->pt.x+rc2.Width(), pSecond->pt.y);
	// �E�ォ�牺��
	if ( pt1.x < pt2.x )
		nResult = 1;
	else if ( pt1.x > pt2.x )
		nResult = -1;
	else {
		if ( pt1.y < pt2.y )
			nResult = 1;
		else if ( pt1.y > pt2.y )
			nResult = -1;
		else
			nResult = 0;
	}

	return nResult;
}

int BindMakeSeqFunc5(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	CRectF	rc1(pFirst->pDoc->GetMaxRect()),
			rc2(pSecond->pDoc->GetMaxRect());
	CPointF	pt1(pFirst->pt.x,  pFirst->pt.y -rc1.Height()),
			pt2(pSecond->pt.x, pSecond->pt.y-rc2.Height());
	// ��������E��
	if ( pt1.y < pt2.y )
		nResult = -1;
	else if ( pt1.y > pt2.y )
		nResult = 1;
	else {
		if ( pt1.x < pt2.x )
			nResult = -1;
		else if ( pt1.x > pt2.x )
			nResult = 1;
		else
			nResult = 0;
	}

	return nResult;
}

int BindMakeSeqFunc6(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	CRectF	rc1(pFirst->pDoc->GetMaxRect()),
			rc2(pSecond->pDoc->GetMaxRect());
	CPointF	pt1(pFirst->pt.x,  pFirst->pt.y -rc1.Height()),
			pt2(pSecond->pt.x, pSecond->pt.y-rc2.Height());
	// ����������
	if ( pt1.x < pt2.x )
		nResult = -1;
	else if ( pt1.x > pt2.x )
		nResult = 1;
	else {
		if ( pt1.y < pt2.y )
			nResult = -1;
		else if ( pt1.y > pt2.y )
			nResult = 1;
		else
			nResult = 0;
	}

	return nResult;
}

int BindMakeSeqFunc7(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	CRectF	rc1(pFirst->pDoc->GetMaxRect()),
			rc2(pSecond->pDoc->GetMaxRect());
	CPointF	pt1(pFirst->pt.x +rc1.Width(), pFirst->pt.y -rc1.Height()),
			pt2(pSecond->pt.x+rc2.Width(), pSecond->pt.y-rc2.Height());
	// �E�����獶��
	if ( pt1.y < pt2.y )
		nResult = -1;
	else if ( pt1.y > pt2.y )
		nResult = 1;
	else {
		if ( pt1.x < pt2.x )
			nResult = 1;
		else if ( pt1.x > pt2.x )
			nResult = -1;
		else
			nResult = 0;
	}

	return nResult;
}

int BindMakeSeqFunc8(LPCADBINDINFO pFirst, LPCADBINDINFO pSecond)
{
	int		nResult;
	CRectF	rc1(pFirst->pDoc->GetMaxRect()),
			rc2(pSecond->pDoc->GetMaxRect());
	CPointF	pt1(pFirst->pt.x +rc1.Width(), pFirst->pt.y -rc1.Height()),
			pt2(pSecond->pt.x+rc2.Width(), pSecond->pt.y-rc2.Height());
	// �E��������
	if ( pt1.x < pt2.x )
		nResult = 1;
	else if ( pt1.x > pt2.x )
		nResult = -1;
	else {
		if ( pt1.y < pt2.y )
			nResult = -1;
		else if ( pt1.y > pt2.y )
			nResult = 1;
		else
			nResult = 0;
	}

	return nResult;
}
