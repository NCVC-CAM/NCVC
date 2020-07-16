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

/*	--- �p�~
// m_ptPaper[1] �ւ���̫�Ĉ���(A4����)
static	const	int		xExt = 420;
static	const	int		yExt = 297;
*/
// ���_��������̫�Ă̔��a
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
	// NC�����ƭ�
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_DXF2NCD, ID_FILE_DXF2NCD_EX2, OnUpdateFileDXF2NCD)
	ON_COMMAND_RANGE(ID_FILE_DXF2NCD, ID_FILE_DXF2NCD_EX2, OnFileDXF2NCD)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �N���X�̍\�z/����

CDXFDoc::CDXFDoc()
{
	int		i;
	// ������Ԃʹװ, �گ�ނ͌p��
	m_bShape = m_bReady = FALSE;
	m_bThread = m_bReload = TRUE;
	m_nShapePattern = 0;
	// �ް���������
	for ( i=0; i<SIZEOF(m_nDataCnt); i++ )
		m_nDataCnt[i] = 0;
	for ( i=0; i<SIZEOF(m_pCircle); i++ )
		m_pCircle[i] = NULL;
	// ��޼ު�ċ�`�̏�����
	m_rcMax.left = m_rcMax.top = DBL_MAX;
	m_rcMax.right = m_rcMax.bottom = -DBL_MAX;
	// �ؼ��ٌ��_�̏�����
	m_ptOrgOrig = HUGE_VAL;
	// �������蓖�ăT�C�Y
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
// հ�����ފ֐�

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
		RemoveSub(m_obDXFArray[nIndex]);	// �����ް��̍폜�Ɗ֘A�ް��̌��Z
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
	// ��O�۰�͏�ʂŷ���
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
		RemoveSub(m_obDXFArray[i]);	// �ް��̍폜�Ɗ֘A�ް��̌��Z
	m_obDXFArray.RemoveAt(nIndex, nCnt);
}

void CDXFDoc::RemoveAtText(int nIndex, int nCnt)
{
	int	nLoop = min(m_obDXFTextArray.GetSize(), nIndex+nCnt);
	for ( int i=nIndex; i<nLoop; i++ )
		RemoveSub(m_obDXFTextArray[i]);	// �ް��̍폜�Ɗ֘A�ް��̌��Z
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
	// ���H�J�n�ʒu�w��ڲԂ̊J�n�~�������ʂɂ�����ڲԏ��̓o�^���s��
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
	// �S�Ă̵�޼ު�ĂƎw����W�̋������擾
	for ( int i=0; i<m_obDXFArray.GetSize(); i++ ) {
		pData = m_obDXFArray[i];
		// ���݂̕\���ر��
		if ( rc.IntersectRect(rcView, pData->GetMaxRect()) ) {
			// �\��ڲԂ��ۂ�
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
// CDXFDoc �f�f

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
// CDXFDoc �N���X�̃I�[�o���C�h�֐�

BOOL CDXFDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	BOOL	bResult;
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
		bResult = CDocument::OnOpenDocument(lpszPathName);
	}

	if ( bResult ) {
#ifdef _DEBUG
		SerializeInfo();
#endif
		// �޷���ĕύX�ʒm�گ�ނ̐���
		POSITION	pos = GetFirstViewPosition();
		CFrameWnd*	pFrame = GetNextView(pos)->GetParentFrame();
		CDocBase::OnOpenDocument(lpszPathName, pFrame);
		// �؍��ް����Ȃ��Ƃ�
		if ( m_obDXFArray.GetSize() < 1 ) {
			AfxMessageBox(IDS_ERR_DXFDATACUT, MB_OK|MB_ICONEXCLAMATION);
		}
		// ���_�ް����Ȃ��Ƃ�(��`�̏㉺�ɒ���)
		else if ( !m_pCircle[DXFCIRCLEORG] ) {
			CPointD	pt(HUGE_VAL);
			switch ( AfxGetNCVCApp()->GetDXFOption()->GetDxfFlag(DXFOPT_ORGTYPE) ) {
			case 1:	// �E��
				pt = m_rcMax.BottomRight();
				break;
			case 2:	// �E��
				pt.x = m_rcMax.right;	pt.y = m_rcMax.top;
				break;
			case 3:	// ����
				pt.x = m_rcMax.left;	pt.y = m_rcMax.bottom;
				break;
			case 4:	// ����
				pt = m_rcMax.TopLeft();
				break;
			case 5:	// ����
				pt = m_rcMax.CenterPoint();
				break;
			default:
				AfxMessageBox(IDS_ERR_DXFDATAORG, MB_OK|MB_ICONEXCLAMATION);
				break;
			}
			if ( pt != HUGE_VAL ) {
				SetCutterOrigin(pt, dOrgR);
				m_bReady = TRUE;	// ����OK!
			}
		}
		else {
			m_ptOrgOrig = m_pCircle[DXFCIRCLEORG]->GetCenter();	// ̧�ق���̵ؼ��ٌ��_��ۑ�
			m_bReady = TRUE;	// ����OK!
		}
	}

	// Ҳ��ڰт���۸�ڽ�ް������
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);

	return bResult;
}

BOOL CDXFDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// �޷���ĕύX�ʒm�گ�ނ̏I��
	CDocBase::OnCloseDocument();

	if ( GetPathName().CompareNoCase(lpszPathName) != 0 ) {
		CDXFDoc* pDoc = AfxGetNCVCApp()->GetAlreadyDXFDocument(lpszPathName);
		ASSERT( pDoc != this );
		if ( pDoc )
			pDoc->OnCloseDocument();	// ���ɊJ���Ă����޷���Ă����
	}

	// �ۑ�����
	BOOL bResult = CDocument::OnSaveDocument(lpszPathName);

	// �޷���ĕύX�ʒm�گ�ނ̐���
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
	// ۯ���޲݂�����
	if ( !IsLockThread() ) {	// CDocBase
#ifdef _DEBUG
		dbg.printf("AddinLock FALSE");
#endif
		return;
	}
	// �������̽گ�ނ𒆒f������
	CDocBase::OnCloseDocument();	// ̧�ٕύX�ʒm�گ��
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
		return;	// �W���װү���ނ��o���Ȃ�
	}
	CDocument::ReportSaveLoadException(lpszPathName, e, bSaving, nIDPDefault);
}

void CDXFDoc::UpdateFrameCounts()
{
	// �ڰт̐���}�����邱�Ƃų���޳���ق́u:1�v��t�^���Ȃ��悤�ɂ���
	CFrameWnd*	pFrame;
	for ( POSITION pos = GetFirstViewPosition(); pos; ) {
		pFrame = GetNextView(pos)->GetParentFrame();
		if ( pFrame )
			pFrame->m_nWindow = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �V���A���C�Y

void CDXFDoc::Serialize(CArchive& ar)
{
	// DxfSetupReload������OFF
	m_bReload = FALSE;

	int			i;
	POSITION	pos;
	CString		strLayer;
	CLayerData*	pLayer;

	// ͯ�ް���
	CCAMHead	cam;
	cam.Serialize(ar);
	// ڲԏ��
	m_obLayer.RemoveAll();

	if ( ar.IsStoring() ) {
		// ڲԏ�� -> �رٔԍ���t�^���Ēʏ�z��ŕۑ�
		for ( i=0, pos=m_mpLayer.GetStartPosition(); pos; i++ ) {
			m_mpLayer.GetNextAssoc(pos, strLayer, pLayer);
			pLayer->SetSerial(i);
			m_obLayer.Add(pLayer);
		}
		m_obLayer.Serialize(ar);
		// ���_
		for ( i=0; i<SIZEOF(m_pCircle); i++ ) {
			if ( m_pCircle[i] ) {
				ar << (BYTE)1;
				ar.WriteObject(m_pCircle[i]);
			}
			else
				ar << (BYTE)0;
		}
		ar << m_ptOrgOrig.x << m_ptOrgOrig.y;
		// �\����(����҂�댯)
		UpdateAllViews(NULL, UAV_DXFGETVIEWINFO, (CObject *)&m_dxfViewInfo);
		ar << m_dxfViewInfo.ptOrg.x << m_dxfViewInfo.ptOrg.y << m_dxfViewInfo.dFactor;
	}
	else {
		// ڲԏ�� -> �ʏ�z��œǂݍ����ϯ��ݸ�
		// �e��޼ު�Ă� m_obLayer �̈ʒu���� CLayerData* �𓾂�
		m_obLayer.Serialize(ar);
		for ( i=0; i<m_obLayer.GetSize(); i++ ) {
			pLayer = m_obLayer[i];
			m_mpLayer.SetAt(pLayer->GetStrLayer(), pLayer);
		}
		// ���_
		BYTE	bExist;
		for ( i=0; i<SIZEOF(m_pCircle); i++ ) {
			ar >> bExist;
			if ( bExist ) {
				m_pCircle[i] = (CDXFcircleEx *)ar.ReadObject(RUNTIME_CLASS(CDXFcircleEx));
				SetMaxRect(m_pCircle[i]);
			}
		}
		ar >> m_ptOrgOrig.x >> m_ptOrgOrig.y;
		// �\����
		ar >> m_dxfViewInfo.ptOrg.x >> m_dxfViewInfo.ptOrg.y >> m_dxfViewInfo.dFactor;
	}
	// �e��CAD�ް�
	m_obDXFArray.Serialize(ar);			// DXF�؍��ް�
	m_obDXFTextArray.Serialize(ar);		// �@�V�@(CDXFtext only)
	m_obStartArray.Serialize(ar);		// DXF���H�J�n�ʒu�w���ް�
	m_obStartTextArray.Serialize(ar);	// �@�V�@(CDXFtext only)
	m_obMoveArray.Serialize(ar);		// DXF�ړ��w���ް�
	m_obMoveTextArray.Serialize(ar);	// �@�V�@(CDXFtext only)
	m_obCommentArray.Serialize(ar);		// ���ĕ����ް�(CDXFtext only)
	m_obLayer.RemoveAll();

	// NC����̧�ٖ�
	if ( ar.IsStoring() ) {
		ar << m_strNCFileName;
		return;		//�ۑ��͂����܂�
	}

	ar >> m_strNCFileName;
	// ̧���߽������
	CString	strPath, strFile;
	if ( !m_strNCFileName.IsEmpty() ) {
		::Path_Name_From_FullPath(m_strNCFileName, strPath, strFile);
		if ( !::PathFileExists(strPath) )
			m_strNCFileName.Empty();
	}

	// �ް��ǂݍ��݌�̏���
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
// CDXFDoc �R�}���h

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

	// ���_�ړ�����
	CPointD	pt(HUGE_VAL), ptOffset;
	switch ( dlg.m_nSelect ) {
	case 0:		// ���l�ړ�
		if ( GetEditOrgPoint(dlg.m_strNumeric, ptOffset) )
			pt = m_pCircle[DXFCIRCLEORG]->GetCenter() + ptOffset;
		break;
	case 1:		// ��`�ړ�
		switch ( dlg.m_nRectType ) {
		case 0:	// �E��
			pt = m_rcMax.BottomRight();
			break;
		case 1:	// �E��
			pt.x = m_rcMax.right;	pt.y = m_rcMax.top;
			break;
		case 2:	// ����
			pt.x = m_rcMax.left;	pt.y = m_rcMax.bottom;
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

	if ( pt != HUGE_VAL ) {
		SetCutterOrigin(pt, dOrgR, TRUE);
		m_bReady = TRUE;	// ����OK!
	}
}

void CDXFDoc::OnEditShape() 
{
	// �󋵈ē��޲�۸�(�����گ�ސ���)
	CThreadDlg	dlgThread(ID_EDIT_DXFSHAPE, this);
	if ( dlgThread.DoModal() == IDOK ) {
		m_bShape = TRUE;
		// ���د�����޳���L���� + DXFView ��̨��ү���ޑ��M
		((CDXFChild *)(AfxGetNCVCMainWnd()->MDIGetActive()))->ShowShapeView();
		// DXFShapeView�X�V
		UpdateAllViews(NULL, UAV_DXFSHAPEUPDATE);
		// ���H�w������̫��
		m_nShapePattern = ID_EDIT_SHAPE_VEC;
	}
}

void CDXFDoc::OnEditAutoShape() 
{
	CDxfAutoWorkingDlg	dlg;
	if ( dlg.DoModal() == IDOK ) {
		// ���ݓo�^����Ă�����H�w�����폜
		UpdateAllViews(NULL, UAV_DXFDELWORKING);
		// �������H�w��
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
	// �װ�ް��̏ꍇ��NC�ϊ��ƭ��𖳌��ɂ���
	if ( !m_bReady || !m_pCircle[DXFCIRCLEORG] )
		bEnable = FALSE;
	else {
		// �P��ڲԂ̏ꍇ�͊g��������off�ɂ���
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
	case ID_FILE_DXF2NCD:		// �P�����
	{
		CMakeNCDlg		dlg(this);
		if ( dlg.DoModal() != IDOK )
			return;
		m_strNCFileName	= dlg.m_strNCFileName;
		bNCView = dlg.m_bNCView;
	}
		break;

	case ID_FILE_DXF2NCD_EX1:	// ڲԂ��Ƃ̕�������
	{
		CMakeNCDlgEx1	dlg(this);
		if ( dlg.DoModal() != IDOK )
			return;
		m_strNCFileName	= dlg.m_strNCFileName;
		bNCView = dlg.m_bNCView;
	}
		break;

	case ID_FILE_DXF2NCD_EX2:	// ڲԂ��Ƃ�Z���W
	{
		CMakeNCDlgEx2	dlg(this);
		if ( dlg.DoModal() != IDOK )
			return;
		m_strNCFileName	= dlg.m_strNCFileName;
		bNCView = dlg.m_bNCView;
	}
		break;

	default:	// �ی�
		return;
	}

	CNCDoc*		pDoc;
	CLayerData*	pLayer;
	POSITION	pos;
	CString		strLayer;
	BOOL		bAllOut;
	// ���łɊJ���Ă����޷���ĂȂ����ųݽ
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

	// CDXFCircle�̌����H�Ώ��ް������ɖ߂��گ�ނ̏I���҂�
	m_csRestoreCircleType.Lock();
	m_csRestoreCircleType.Unlock();

	// �󋵈ē��޲�۸�(�ϊ��گ�ސ���)
	CThreadDlg	dlgThread(ID_FILE_DXF2NCD, this, (LPVOID)nID);
	int nRet = dlgThread.DoModal();

	// CDXFCircle�̌����H�Ώ��ް������ɖ߂�
	AfxBeginThread(RestoreCircleTypeThread, this,
		/*THREAD_PRIORITY_LOWEST*/
		THREAD_PRIORITY_IDLE
		/*THREAD_PRIORITY_BELOW_NORMAL*/);

	if ( nRet!=IDOK || !bNCView )
		return;

	// NC��������ް����J��
	if ( nID == ID_FILE_DXF2NCD ) {
		AfxGetNCVCApp()->OpenDocumentFile(m_strNCFileName);
	}
	else {
		bAllOut = FALSE;	// m_strNCFileName ���J�����ǂ���
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
	// �g���q�� .cam ���ۂ�
	if ( strExt.CompareNoCase(GetPathName().Right(4)) == 0 )
		OnSaveDocument(GetPathName());
	else
		OnFileSaveAs();		// ���O��t���ĕۑ�
}

void CDXFDoc::OnFileSaveAs() 
{
 	extern	LPCTSTR	gg_szWild;		// "*.";
	// ��ڰ��ް�t��̧�ٕۑ��޲�۸ނ��o�����߂ɵ��ްײ��
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
// CDXFDoc �T�u�X���b�h

UINT CDXFDoc::RestoreCircleTypeThread(LPVOID pParam)
{
	CDXFDoc* pDoc = (CDXFDoc *)pParam;
	pDoc->m_csRestoreCircleType.Lock();		// �گ�ޏI���܂�ۯ�
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
