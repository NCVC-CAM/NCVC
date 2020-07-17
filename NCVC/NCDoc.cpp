// NCDoc.cpp : CNCDoc �N���X�̓���̒�`���s���܂��B
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "MCOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCListView.h"
#include "NCInfoView.h"
#include "NCWorkDlg.h"
#include "ThreadDlg.h"
#include "DXFMakeOption.h"
#include "DXFMakeClass.h"
#include "MakeDXFDlg.h"
#include "DXFkeyword.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif
//#define	_DEBUGOLD
#undef	_DEBUGOLD

// �����񌟍��p(TH_NCRead.cpp, NCMakeClass.cpp ������Q��)
extern	LPCTSTR	g_szGdelimiter = "GSMOF";
extern	LPCTSTR	g_szNdelimiter = "XYZRIJKPLDH";
extern	LPTSTR	g_pszDelimiter;		// g_szGdelimiter[] + g_szNdelimiter[] (NCVC.cpp�Ő���)

// �w�肳�ꂽ�l���׸�
extern	const	DWORD	g_dwSetValFlags[] = {
	NCD_X, NCD_Y, NCD_Z, 
	NCD_R, NCD_I, NCD_J, NCD_K,
	NCD_P, NCD_L,
	NCD_D, NCD_H
};

/////////////////////////////////////////////////////////////////////////////
// CNCDoc

IMPLEMENT_DYNCREATE(CNCDoc, CDocument)

BEGIN_MESSAGE_MAP(CNCDoc, CDocument)
	//{{AFX_MSG_MAP(CNCDoc)
	ON_UPDATE_COMMAND_UI(ID_FILE_NCINSERT, OnUpdateFileInsert)
	ON_COMMAND(ID_FILE_NCINSERT, OnFileInsert)
	ON_COMMAND(ID_FILE_NCD2DXF, OnFileNCD2DXF)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_WORKRECT, OnUpdateWorkRect)
	ON_COMMAND(ID_NCVIEW_WORKRECT, OnWorkRect)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_MAXRECT, OnUpdateMaxRect)
	ON_COMMAND(ID_NCVIEW_MAXRECT, OnMaxRect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCDoc �N���X�̍\�z/����

CNCDoc::CNCDoc()
{
	m_fError = TRUE;	// ������Ԃʹװ
	m_dMove[0] = m_dMove[1] = 0.0;
	m_dCutTime = -1.0;
	m_nTraceStart = m_nTraceDraw = 0;
	m_bWorkRect = m_bMaxRect = FALSE;
	m_hCutcalc = NULL;
	m_bCutcalc = m_bCorrect = FALSE;
	// ܰ����W�n�擾
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();
	for ( int i=0; i<WORKOFFSET; i++ )
		m_ptNcWorkOrg[i] = pMCopt->GetWorkOffset(i);
	m_ptNcWorkOrg[i] = 0.0;		// G92�̏�����
	m_nWorkOrg = pMCopt->GetModalSetting(MODALGROUP2);		// G54�`G59
	if ( m_nWorkOrg<0 || SIZEOF(m_ptNcWorkOrg)<=m_nWorkOrg )
		m_nWorkOrg = 0;
	// ��޼ު�ċ�`�̏�����
	m_rcMax.SetRectMinimum();
	// �������蓖�ăT�C�Y
	m_obBlock.SetSize(0, 1024);
	m_obGdata.SetSize(0, 1024);
}

CNCDoc::~CNCDoc()
{
	int	i;
	for ( i=0; i<m_obGdata.GetSize(); i++ )
		delete	m_obGdata[i];
	for ( i=0; i<m_obBlock.GetSize(); i++ )
		delete	m_obBlock[i];
	// �ꎞ�W�J��ϸ�̧�ق�����
	for ( i=0; i<m_obMacroFile.GetSize(); i++ ) {
#ifdef _DEBUG
		BOOL bResult = ::DeleteFile(m_obMacroFile[i]);
		if ( !bResult ) {
			g_dbg.printf("Delete File=%s", m_obMacroFile[i]);
			::NC_FormatMessage();
		}
#else
		::DeleteFile(m_obMacroFile[i]);
#endif
	}
}

/////////////////////////////////////////////////////////////////////////////
// հ�����ފ֐�

BOOL CNCDoc::RouteCmdToAllViews
	(CView* pActiveView, UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	CView*	pView;

	for ( POSITION pos=GetFirstViewPosition(); pos; ) {
		pView = GetNextView(pos);
		// ��è���ޭ��� CNC[View|Info][*] �Ȃ�����ٰèݸނ��Ȃ�
		// CNC[View|Info][*] �� CNC[View|Info]Tab ���籸è���ޭ��ɑ΂��Ă��������ٰèݸ�
		if ( pView!=pActiveView &&
				( pView->IsKindOf(RUNTIME_CLASS(CNCListView)) ||
				  pView->IsKindOf(RUNTIME_CLASS(CNCInfoTab)) ||
				  pView->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) ) {
			if ( pView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
				return TRUE;
		}
	}
	return FALSE;
}

CNCdata* CNCDoc::DataOperation
	(const CNCdata* pDataSrc, LPNCARGV lpArgv, int nIndex/*=-1*/, ENNCOPERATION enOperation/*=NCADD*/)
{
	CMCOption*	pOpt = AfxGetNCVCApp()->GetMCOption();
	CNCdata*	pData = NULL;
	CNCblock*	pBlock;
	CPoint3D	pt( m_ptNcWorkOrg[m_nWorkOrg] + m_ptNcLocalOrg );
	int			i;

	// ��O�۰�͏�ʂŷ���
	if ( lpArgv->nc.nGtype == G_TYPE ) {
		switch ( lpArgv->nc.nGcode ) {
		case 0:		// ����
		case 1:
			// ��޼ު�Đ���
			pData = new CNCline(pDataSrc, lpArgv, pt);
			SetMaxRect(pData);		// �ŏ��E�ő�l�̍X�V
			if ( lpArgv->nc.dwValFlags & NCD_CORRECT )
				m_bCorrect = TRUE;
			break;
		case 2:		// �~��
		case 3:
			pData = new CNCcircle(pDataSrc, lpArgv, pt);
			SetMaxRect(pData);
			if ( lpArgv->nc.dwValFlags & NCD_CORRECT )
				m_bCorrect = TRUE;
			break;
		case 81:	// �Œ軲��
		case 82:
		case 83:
		case 84:
		case 85:
		case 86:
		case 87:
		case 88:
		case 89:
			pData = new CNCcycle(pDataSrc, lpArgv, pt, pOpt->GetFlag(MC_FLG_L0CYCLE));
			SetMaxRect(pData);
			break;
		case 10:	// �ް��ݒ�
			if ( lpArgv->nc.dwValFlags & (NCD_P|NCD_R) ) {	// G10P_R_
				// �H����̒ǉ�
				if ( pOpt->AddTool((int)lpArgv->nc.dValue[NCA_P], lpArgv->nc.dValue[NCA_R], lpArgv->bAbs) ) {
					pData = new CNCdata(pDataSrc, lpArgv, pt);
				}
				else {
					i = lpArgv->nc.nLine;
					if ( 0<=i && i<m_obBlock.GetSize() ) {	// �ی�
						pBlock = GetNCblock(i);
						pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_G10ADDTOOL);
					}
				}
				break;
			}
			else if ( lpArgv->nc.dwValFlags & NCD_P ) {
				// ܰ����W�n�̐ݒ�
				int nWork = (int)lpArgv->nc.dValue[NCA_P];
				if ( nWork>=0 && nWork<WORKOFFSET ) { 
					for ( i=0; i<NCXYZ; i++ ) {
						if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] )
							m_ptNcWorkOrg[nWork][i] += lpArgv->nc.dValue[i];
					}
					pData = new CNCdata(pDataSrc, lpArgv, pt);
					break;
				}
			}
			// P�l�F���s�\
			i = lpArgv->nc.nLine;
			if ( 0<=i && i<m_obBlock.GetSize() ) {
				pBlock = GetNCblock(i);
				pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
			}
			break;
		case 52:	// ۰�ٍ��W�ݒ�
			for ( i=0; i<NCXYZ; i++ ) {
				if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] )
					m_ptNcLocalOrg[i] = lpArgv->nc.dValue[i];
			}
			pData = new CNCdata(pDataSrc, lpArgv, pt);
			break;
		case 92:
			// ۰�ٍ��W�n�ر��G92�l�擾
			for ( i=0; i<NCXYZ; i++ ) {
				m_ptNcLocalOrg[i] = 0.0;
				pt[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
					lpArgv->nc.dValue[i] : pDataSrc->GetEndValue(i);
			}
			// ���݈ʒu - G92�l �ŁAG92���W�n���_���v�Z
			m_ptNcWorkOrg[WORKOFFSET] = pDataSrc->GetEndPoint() - pt;
			m_nWorkOrg = WORKOFFSET;	// G92���W�n�I��
			pt = m_ptNcWorkOrg[WORKOFFSET];
			// through
		default:	// G04 ...
			pData = new CNCdata(pDataSrc, lpArgv, pt);
		}
	}	// end of G_TYPE
	else {
		// M_TYPE, O_TYPE, etc.
		pData = new CNCdata(pDataSrc, lpArgv, pt);
	}

	// ��޼ު�ēo�^
	switch ( enOperation ) {
	case NCADD:
		m_obGdata.Add(pData);
		break;
	case NCINS:
		m_obGdata.InsertAt(nIndex, pData);
		break;
	case NCMOD:
		RemoveAt(nIndex, 1);
		m_obGdata.SetAt(nIndex, pData);
		break;
	}

	// �s�ԍ����ݸ�����װ�׸ނ̐ݒ�
	UINT	nError = pData->GetNCObjErrorCode();
	if ( nError > 0 ) {
		i = pData->GetBlockLineNo();
		if ( 0<=i && i<m_obBlock.GetSize() ) {	// �ی�
			pBlock = GetNCblock(i);
			pBlock->SetNCBlkErrorCode(nError);
		}
	}

	return pData;
}

void CNCDoc::StrOperation(LPCSTR pszTmp, int nIndex/*=-1*/, ENNCOPERATION enOperation/*=NCADD*/)
{
	int	nBuf = (int)strcspn(pszTmp, g_pszDelimiter);
	CString		str(pszTmp);
	CNCblock*	pBlock = NULL;

	// ��O�۰�͏�ʂŷ���
	pBlock = new CNCblock(str.Left(nBuf).Trim(), str.Mid(nBuf).Trim());
	switch ( enOperation ) {
	case NCADD:
		m_obBlock.Add(pBlock);
		break;
	case NCINS:
		m_obBlock.InsertAt(nIndex, pBlock);
		break;
	case NCMOD:
		delete	m_obBlock[nIndex];
		m_obBlock.SetAt(nIndex, pBlock);
		break;
	}
}

void CNCDoc::RemoveAt(int nIndex, int nCnt)
{
	nCnt = min(nCnt, m_obGdata.GetSize() - nIndex);
	CNCdata*	pData;
	for ( int i=nIndex; i<nIndex+nCnt; i++ ) {
		pData = m_obGdata[i];
		if ( pData->GetGtype() == G_TYPE ) {
			switch ( pData->GetType() ) {
			case NCDLINEDATA:
				m_dMove[pData->GetGcode()] -= pData->GetCutLength();
				break;
			case NCDCYCLEDATA:
				m_dMove[0] -= static_cast<CNCcycle *>(pData)->GetCycleMove();
				m_dMove[1] -= pData->GetCutLength();
				break;
			case NCDARCDATA:
				m_dMove[1] -= pData->GetCutLength();
				break;
			}
		}
		delete pData;
	}
	m_obGdata.RemoveAt(nIndex, nCnt);
}

void CNCDoc::RemoveStr(int nIndex, int nCnt)
{
	nCnt = min(nCnt, m_obBlock.GetSize() - nIndex);
	for ( int i=nIndex; i<nIndex+nCnt; i++ )
		delete	m_obBlock[i];
	m_obBlock.RemoveAt(nIndex, nCnt);
}

void CNCDoc::AllChangeFactor(double f) const
{
	f *= LOMETRICFACTOR;
	for ( int i=0; i<GetNCsize(); i++ )
		GetNCdata(i)->DrawTuning(f);
}

void CNCDoc::AllChangeFactorXY(double f) const
{
	f *= LOMETRICFACTOR;
	for ( int i=0; i<GetNCsize(); i++ )
		GetNCdata(i)->DrawTuningXY(f);
}

void CNCDoc::AllChangeFactorXZ(double f) const
{
	f *= LOMETRICFACTOR;
	for ( int i=0; i<GetNCsize(); i++ )
		GetNCdata(i)->DrawTuningXZ(f);
}

void CNCDoc::AllChangeFactorYZ(double f) const
{
	f *= LOMETRICFACTOR;
	for ( int i=0; i<GetNCsize(); i++ )
		GetNCdata(i)->DrawTuningYZ(f);
}

void CNCDoc::CreateCutcalcThread(void)
{
	WaitCalcThread();	// ���݌v�Z���Ȃ璆�f

	LPNCVCTHREADPARAM	pParam = new NCVCTHREADPARAM;
	pParam->pParent = NULL;
	pParam->pDoc = this;
	pParam->wParam = NULL;		// CNCInfoView1
	pParam->lParam = NULL;

	CView*	pView;
	for ( POSITION pos=GetFirstViewPosition(); pos; ) {
		pView = GetNextView(pos);
		if ( pView->IsKindOf(RUNTIME_CLASS(CNCInfoView1)) ) {
			pParam->wParam = (WPARAM)pView;
			break;
		}
	}

	CWinThread* pThread = AfxBeginThread(CuttimeCalc_Thread, pParam,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	m_hCutcalc = ::NCVC_DuplicateHandle(pThread->m_hThread);
	if ( !m_hCutcalc )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
	m_bCutcalc = TRUE;
	pThread->ResumeThread();
}

void CNCDoc::WaitCalcThread(void)
{
	m_bCutcalc = FALSE;
	if ( m_hCutcalc ) {
#ifdef _DEBUG
		CMagaDbg	dbg("CNCDoc::WaitCalcThread()", DBG_BLUE);
		if ( ::WaitForSingleObject(m_hCutcalc, INFINITE) == WAIT_FAILED ) {
			dbg.printf("WaitForSingleObject() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("WaitForSingleObject() OK");
		if ( !::CloseHandle(m_hCutcalc) ) {
			dbg.printf("CloseHandle() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("CloseHandle() OK");
#else
		::WaitForSingleObject(m_hCutcalc, INFINITE);
		::CloseHandle(m_hCutcalc);
#endif
		m_hCutcalc = NULL;
	}
}

int CNCDoc::SearchBlockRegex(boost::regex& r, int nStart/*=0*/, BOOL bReverse/*=FALSE*/)
{
	int		i;

	if ( bReverse ) {
		for (i=nStart; i>=0; i--) {
			if ( regex_search((LPCTSTR)(GetNCblock(i)->GetStrGcode()), r) )
				return i;
		}
	}
	else {
		for (i=nStart; i<GetNCBlockSize(); i++) {
			if ( regex_search((LPCTSTR)(GetNCblock(i)->GetStrGcode()), r) )
				return i;
		}
	}

	return -1;
}

void CNCDoc::ClearBreakPoint(void)
{
	CNCblock*	pBlock;
	for (int i=0; i<GetNCBlockSize(); i++) {
		pBlock = GetNCblock(i);
		pBlock->SetBlockFlag(pBlock->GetBlockFlag() & ~NCF_BREAK, FALSE);
	}
}

BOOL CNCDoc::IncrementTrace(int& nTraceDraw)
{
	int			nMax = GetNCsize(), nLine1, nLine2;
	BOOL		bResult = FALSE, bBreakChk;
	CNCdata*	pData;

	m_csTraceDraw.Lock();
	if ( m_nTraceDraw > 0 ) {
		pData = GetNCdata(m_nTraceDraw-1);
		nLine1 = pData->GetBlockLineNo();		// �ݸ���đO����ۯ��s
		bBreakChk = pData->GetGtype()==M_TYPE ? FALSE : TRUE;
	}
	else {
		nLine1 = 0;
		bBreakChk = FALSE;
	}
	// NC��޼ު�ĒP�ʂ̲ݸ����
	m_nTraceDraw++;
	if ( nMax < m_nTraceDraw ) {
		nTraceDraw = -1;
		m_nTraceDraw = nMax;
		m_csTraceDraw.Unlock();
		return FALSE;
	}
	pData = GetNCdata(m_nTraceDraw-1);
	// �ݸ���Č�̵�޼ު�Ă�M_TYPE�ŁA�ݸ���đO�Ɠ�����ۯ��Ȃ�
	if ( pData->GetGtype()==M_TYPE && nLine1==pData->GetBlockLineNo() ) {
		// ����ɲݸ����
		m_nTraceDraw++;
		pData = GetNCdata(m_nTraceDraw-1);
	}
	nLine2 = pData->GetBlockLineNo();			// �ݸ���Č����ۯ��s
	nTraceDraw = m_nTraceDraw;
	m_csTraceDraw.Unlock();

	// ��ڲ��߲�Ă�����
	if ( bBreakChk ) {
		while ( ++nLine1 <= nLine2 ) {
			if ( IsBreakPoint(nLine1) ) {
				bResult = TRUE;
				break;
			}
		}
	}
	else
		bResult = IsBreakPoint( GetNCdata(nTraceDraw-1)->GetBlockLineNo() );

	return bResult;
}

BOOL CNCDoc::SetLineToTrace(BOOL bStart, int nLine)
{
	for ( int i=nLine; i<GetNCBlockSize(); i++ ) {
		if ( GetNCblock(i)->GetBlockToNCdata() )
			break;
	}
	if ( i >= GetNCBlockSize() ) {
		m_csTraceDraw.Lock();
		m_nTraceDraw = GetNCsize();
		if ( bStart )
			m_nTraceStart = m_nTraceDraw;
		m_csTraceDraw.Unlock();
		return FALSE;
	}
	m_csTraceDraw.Lock();
	m_nTraceDraw = GetNCblock(i)->GetBlockToNCdataArrayNo();
	if ( bStart ) {
		int n =m_nTraceDraw - 1;
		m_nTraceStart = max(0, n);
	}
	m_csTraceDraw.Unlock();

	return TRUE;
}

void CNCDoc::GetWorkRectPP(int a, double dTmp[])
{
	ASSERT(a>=NCA_X && a<=NCA_Z);
	switch (a) {
	case NCA_X:
		dTmp[0] = m_rcMax.left;
		dTmp[1] = m_rcMax.right;
		break;
	case NCA_Y:
		dTmp[0] = m_rcMax.top;
		dTmp[1] = m_rcMax.bottom;
		break;
	case NCA_Z:
		dTmp[0] = m_rcMax.high;
		dTmp[1] = m_rcMax.low;
		break;
	}
}

void CNCDoc::MakeDXF(const CDXFMakeOption* pDXFMake)
{
	CWaitCursor		wait;
	CProgressCtrl*	pProgress;
	CTypedPtrArrayEx<CPtrArray, CDXFMake*>	obDXFdata;// DXF�o�ͲҰ��
	CDXFMake*	pMake;
	CNCdata*	pData;
	CNCdata*	pDataBase;
	int			i, j, nLoop = m_obGdata.GetSize(), nCorrect;
	BOOL		bOrigin = TRUE;
	DWORD		dwValFlag;
	CString		strMsg;

	obDXFdata.SetSize(0, nLoop);
	// �ÓI�ϐ�������
	CDXFMake::SetStaticOption(pDXFMake);
	// ���ʎw��ɂ�錟���׸ނ̐ݒ�
	switch ( pDXFMake->GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		dwValFlag = NCD_X | NCD_Z;
		break;
	case 2:		// YZ
		dwValFlag = NCD_Y | NCD_Z;
		break;
	default:	// XY
		dwValFlag = NCD_X | NCD_Y;
		break;
	}
	dwValFlag |= (NCD_I | NCD_J | NCD_K);	// �~�ʕ�ԗp(�ǂ̕��ʂ����������)

	// Ҳ��ڰт���۸�ڽ�ް����
	pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	pProgress->SetRange32(0, nLoop);
	pProgress->SetPos(0);

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		// �e����ݐ���(ENTITIES�̾���ݖ��܂�)
		for ( i=SEC_HEADER; i<=SEC_ENTITIES; i++ ) {
			pMake = new CDXFMake(i, this);
			obDXFdata.Add(pMake);
		}
		// NC��޼ު�Ă�DXF�o��
		for ( i=0; i<nLoop; i++ ) {
			pData = m_obGdata[i];
			if ( pData->GetGtype()==G_TYPE && pData->GetType()!=NCDBASEDATA &&
					pData->GetValFlags()&dwValFlag ) {
				if ( bOrigin ) {
					// �ŏ��̑Ώۺ��ނ̊J�n���W�����_�Ƃ���
					pMake = new CDXFMake(pData->GetStartPoint());
					obDXFdata.Add(pMake);
					bOrigin = FALSE;		// ���_�o�͍ς�
				}
				// ��޼ު�Đ���
				pMake = new CDXFMake(pData);
				obDXFdata.Add(pMake);
				// �␳��޼ު��
				if ( m_bCorrect ) {
					pDataBase = pData;
					nCorrect = pDataBase->GetCorrectArray()->GetSize();
					for ( j=0; j<nCorrect; j++ ) {
						pData = pDataBase->GetCorrectArray()->GetAt(j);
						pMake = new CDXFMake(pData, TRUE);
						obDXFdata.Add(pMake);
					}
				}
			}
			if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
				pProgress->SetPos(i);		// ��۸�ڽ�ް
		}
		// ENTITIES����ݏI����EOF�o��
		pMake = new CDXFMake(SEC_NOSECTION);
		obDXFdata.Add(pMake);
		// �o��
		CStdioFile	fp(m_strDXFFileName,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
		for ( i=0; i<obDXFdata.GetSize(); i++ )
			obDXFdata[i]->WriteDXF(fp);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
	catch (CFileException* e) {
		strMsg.Format(IDS_ERR_DATAWRITE, m_strDXFFileName);
		AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	// ��޼ު�ď���
	for ( i=0; i<obDXFdata.GetSize(); i++ )
		delete	obDXFdata[i];
	obDXFdata.RemoveAll();

	pProgress->SetPos(0);
}

void CNCDoc::AddMacroFile(const CString& strMacroFile)
{
	try {
		if ( !strMacroFile.IsEmpty() )
			m_obMacroFile.Add(strMacroFile);
	}
	catch (CMemoryException* e) {
		e->Delete();	// �װ����(Msg)�͓��ɕK�v�Ƃ��Ȃ�
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCDoc �N���X�̐f�f

#ifdef _DEBUG
void CNCDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNCDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCDoc �N���X�̃I�[�o���C�h�֐�

BOOL CNCDoc::OnOpenDocument(LPCTSTR lpszPathName) 
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
		if ( bResult ) {
			// CDocument::GetPathName() ��
			// OnOpenDocument() �I������ڰ�ܰ����ݒ肷��̂Ŏg���Ȃ�
			m_strCurrentFile = lpszPathName;
		}
	}

	if ( bResult ) {
		// ̧�ٓǂݍ��݌������
		bResult = SerializeAfterCheck();
		if ( bResult ) {
			// �޷���ĕύX�ʒm�گ�ނ̐���
			POSITION	pos = GetFirstViewPosition();
			CDocBase::OnOpenDocument(lpszPathName, GetNextView(pos)->GetParentFrame());
		}
	}

	// Ҳ��ڰт���۸�ڽ�ް������
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);

	return bResult;
}

BOOL CNCDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// �޷���ĕύX�ʒm�گ�ނ̏I��
	CDocBase::OnCloseDocument();

	if ( GetPathName().CompareNoCase(lpszPathName) != 0 ) {
		CNCDoc* pDoc = AfxGetNCVCApp()->GetAlreadyNCDocument(lpszPathName);
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

void CNCDoc::OnCloseDocument() 
{
/*
	�e�ޭ��� OnDestroy() ������ɌĂ΂��
*/
#ifdef _DEBUG
	CMagaDbg	dbg("CNCDoc::OnCloseDocument()\nStart", DBG_GREEN);
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
	WaitCalcThread();				// �؍펞�Ԍv�Z�گ��

	CDocument::OnCloseDocument();
}

void CNCDoc::ReportSaveLoadException
	(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault) 
{
	if ( e->IsKindOf(RUNTIME_CLASS(CUserException)) ) {
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		return;	// �W���װү���ނ��o���Ȃ�
	}
	CDocument::ReportSaveLoadException(lpszPathName, e, bSaving, nIDPDefault);
}

void CNCDoc::SetModifiedFlag(BOOL bModified)
{
	CString	strTitle( GetTitle() );
	if ( UpdateModifiedTitle(bModified, strTitle) )		// DocBase.cpp
		SetTitle(strTitle);
	CDocument::SetModifiedFlag(bModified);
}

/////////////////////////////////////////////////////////////////////////////
// CNCDoc �V���A���C�[�[�V����

void CNCDoc::Serialize(CArchive& ar)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCDoc::Serialize()\nStart");
#endif

	if ( ar.IsStoring() ) {
		// ̧�ٕۑ�
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetRange32(0, GetNCBlockSize());
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		for ( int i=0; i<GetNCBlockSize(); i++ ) {
			ar.WriteString(GetNCblock(i)->GetStrBlock()+"\r\n");
			if ( (i & 0x003f) == 0 )		// 64�񂨂�(����6�ޯ�Ͻ�)
				AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(i);
		}
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		return;
	}

	// ̧�ٓǂݍ���
	SerializeBlock(ar, m_obBlock);
}

void CNCDoc::SerializeBlock
	(CArchive& ar, CNCblockArray& obBlock, DWORD dwFlags/*=0*/, BOOL bProgress/*=TRUE*/)
{
	// �s�ԍ���G���ނ̕���
	static	LPCTSTR	ss_szLineDelimiter = "%N0123456789";

#ifdef _DEBUG
	CMagaDbg	dbg;
#endif
	CString		strBlock, strLine;
	CNCblock*	pBlock = NULL;
	int			n, nCnt = 0;

	ULONGLONG	dwSize = ar.GetFile()->GetLength();		// ̧�ٻ��ގ擾
	DWORD		dwPosition = 0;
	CProgressCtrl* pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	if ( bProgress ) {
		// Ҳ��ڰт���۸�ڽ�ް����
		pProgress->SetRange32(0, 100);
		pProgress->SetPos(0);
	}

	try {
		while ( ar.ReadString(strBlock) ) {
			// ��۸�ڽ�ް�̕\��
			dwPosition += strBlock.GetLength() + 2;	// ���s���ޕ�
			if ( bProgress && (++nCnt & 0x003f)==0 ) {	// 64�񂨂�(����6�ޯ�Ͻ�)
				n = (int)(dwPosition*100/dwSize);
				pProgress->SetPos(min(100, n));
			}
			// �s�ԍ���G���ނ̕���(XYZ...�܂�)
			strLine = strBlock.SpanIncluding(ss_szLineDelimiter);
			pBlock = new CNCblock(strLine.Trim(), strBlock.Mid(strLine.GetLength()).Trim(), dwFlags);
			obBlock.Add(pBlock);
			pBlock = NULL;
#ifdef _DEBUGOLD
			dbg.printf("LineCnt=%d Line=%s Gcode=%s", nCnt,
				strLine, strBlock.Mid(strLine.GetLength()) );
#endif
		}
		if ( bProgress )
			pProgress->SetPos(100);
	}
	catch ( CMemoryException* e ) {
		if ( pBlock )
			delete	pBlock;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		AfxThrowUserException();	// �Öق� ReportSaveLoadException() �Ăяo��
	}
}

BOOL CNCDoc::SerializeAfterCheck(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg;
#endif
	int			i;
#ifdef _DEBUGOLD
	for ( i=0; i<GetNCBlockSize(); i++ )
		dbg.printf("%4d:%s", i, GetNCblock(i)->GetStrBlock());
#endif
	// �ް�����
	for ( i=0; i<GetNCBlockSize(); i++ ) {
		if ( GetNCblock(i)->GetStrGcode().GetLength() > 0 )
			break;
	}
	if ( i >= GetNCBlockSize() ) {
		AfxMessageBox(IDS_ERR_NCDATA, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// �ϊ��󋵈ē��޲�۸�(�ϊ��گ�ސ���)
	CThreadDlg	dlg(IDS_READ_NCD, this);
	if ( dlg.DoModal() != IDOK )
		return FALSE;
	if ( m_obGdata.GetSize() < 1 ) {
		AfxMessageBox(IDS_ERR_NCDATA, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	dbg.printf("m_rcMax left =%f top   =%f", m_rcMax.left, m_rcMax.top);
	dbg.printf("m_rcMax right=%f bottom=%f", m_rcMax.right, m_rcMax.bottom);
	dbg.printf("m_rcMax.low  =%f high  =%f", m_rcMax.low, m_rcMax.high);
#endif
	// �␳���W�v�Z
	if ( m_bCorrect ) {
		CThreadDlg	dlg(IDS_CORRECT_NCD, this);
		if ( dlg.DoModal() != IDOK )
			return FALSE;
	}

	// �؍펞�Ԍv�Z�گ�ފJ�n
	CreateCutcalcThread();
	// NC�ް��̍ő�l�擾
	m_nTraceDraw = GetNCsize();
	// �װ�׸މ���
	m_fError = FALSE;
	// G10�ŉ��o�^���ꂽ�H������폜
	AfxGetNCVCApp()->GetMCOption()->ReductionTools(TRUE);

	return TRUE;
}

BOOL CNCDoc::SerializeInsertBlock
	(LPCTSTR lpszFileName, int nInsert, DWORD dwFlags/*=0*/, BOOL bProgress/*=TRUE*/)
{
/*
	MFC\SRC\DOCCORE.CPP �� CDocument::OnOpenDocument ���Q�l
*/
	CFileException	fe;
	CFile*	pFile = GetFile(lpszFileName, CFile::modeRead|CFile::shareDenyWrite, &fe);
	if ( !pFile ) {
		ReportSaveLoadException(lpszFileName, &fe, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
		return FALSE;
	}
	CArchive	ar(pFile, CArchive::load|CArchive::bNoFlushOnDelete);
	ar.m_pDocument = this;
	ar.m_bForceFlat = FALSE;

	BOOL	bResult = TRUE;
	if ( pFile->GetLength() > 0 ) {
		CNCblockArray	obBlock;	// �ꎞ�̈�
		obBlock.SetSize(0, 1024);
		// ̧�ٓǂݍ���
		SerializeBlock(ar, obBlock, dwFlags, bProgress);
		// ��ۯ��}��
		m_obBlock.InsertAt(nInsert, &obBlock);
	}
	else
		bResult = FALSE;

	ar.Close();
	ReleaseFile(pFile, FALSE);

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CNCDoc �R�}���h

void CNCDoc::OnUpdateFileInsert(CCmdUI* pCmdUI) 
{
	CNCChild*		pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	CNCListView*	pList = pFrame->GetListView();
	pCmdUI->Enable(pList->GetListCtrl().GetFirstSelectedItemPosition() ? TRUE : FALSE);
}

void CNCDoc::OnFileInsert()
{
	int		i;
	CNCChild*		pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	CNCListView*	pList = pFrame->GetListView();
	POSITION pos;
	if ( !(pos=pList->GetListCtrl().GetFirstSelectedItemPosition()) )
		return;
	int	nInsert = pList->GetListCtrl().GetNextSelectedItem(pos);

	CString	strFileName(AfxGetNCVCApp()->GetRecentFileName());
	if ( ::NCVC_FileDlgCommon(ID_FILE_NCINSERT,
				AfxGetNCVCApp()->GetFilterString(TYPE_NCD), strFileName) != IDOK )
		return;

#ifdef _DEBUG
	CMagaDbg	dbg("CNCDoc::OnFileInsert()\nStart");
#endif

	// �u���وʒu�ɓǂݍ��݁v�̏���
	WaitCalcThread();			// �؍펞�Ԍv�Z�̒��f

	// �ĕϊ����s������ m_obGdata ���폜
	for ( i=0; i<m_obGdata.GetSize(); i++ )
		delete	m_obGdata[i];
	m_obGdata.RemoveAll();
	m_bCorrect = FALSE; 
	// ��ۯ��ް����׸ނ�ر
	for ( i=0; i<GetNCBlockSize(); i++ )
		GetNCblock(i)->SetNCBlkErrorCode(0);
	// �ϐ�������
	m_fError = TRUE;
	m_nTraceDraw = 0;

	// ̧��(NC��ۯ��̑}��)
	if ( SerializeInsertBlock(strFileName, nInsert) ) {
		// ��������̧�ق�}��̧�ٖ��ɐݒ�
		m_strCurrentFile = strFileName;
		// ̧�ٓǂݍ��݌������
		SerializeAfterCheck();	// �߂�l�������Ă��ǂ��ɂ��Ȃ�Ȃ�
		// �X�V�׸�ON
		SetModifiedFlag();
		// �e�ޭ��̐ݒ� & �`��X�V
		UpdateAllViews(NULL, UAV_FILEINSERT);
	}

	// Ҳ��ڰт���۸�ڽ�ް������
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
}

void CNCDoc::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(IsModified());
}

void CNCDoc::OnFileNCD2DXF() 
{
	CMakeDXFDlg	ps(this);
	if ( ps.DoModal() != IDOK )
		return;
	if ( !ps.GetDXFMakeOption()->SaveDXFMakeOption() )
		AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);

	// ̧�ٖ��擾�Ə㏑���m�F
	m_strDXFFileName = ps.m_dlg1.GetDXFFileName();
	if ( ::IsFileExist(m_strDXFFileName, FALSE) )	// NCVC.cpp
		MakeDXF(ps.GetDXFMakeOption());	// DXF�o��
}

void CNCDoc::OnUpdateWorkRect(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCWORK) != NULL );
}

void CNCDoc::OnWorkRect() 
{
	if ( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCWORK) ) {
		// CNCWorkDlg::OnCancel() �̊ԐڌĂяo��
		AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCWORK)->PostMessage(WM_CLOSE);
		return;
	}
	try {
		CNCWorkDlg*	pDlg = new CNCWorkDlg;
		pDlg->Create(IDD_NCVIEW_WORK);
		AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCWORK, pDlg);
	}
	catch ( CMemoryException* e ) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
}

void CNCDoc::OnUpdateMaxRect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocError() ? FALSE : TRUE);
	pCmdUI->SetCheck(m_bMaxRect);
}

void CNCDoc::OnMaxRect() 
{
	m_bMaxRect = !m_bMaxRect;
	// �ޭ��̍X�V
	UpdateAllViews(NULL, UAV_DRAWMAXRECT);
}
