// TH_MakeNurbs.cpp
//		NURBS�ȖʗpNC����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"		// NCMakeBase.h�p
#include "3dModelDoc.h"
#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::string;
using namespace boost;

// ��۰��ٕϐ���`
static	CThreadDlg*		g_pParent;
static	C3dModelDoc*	g_pDoc;
static	CNCMakeMillOpt*	g_pMakeOpt;
static	int				g_nFase;	// ̪��އ�

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)

// NC�����ɕK�v���ް��Q
static	CTypedPtrArrayEx<CPtrArray, CNCMakeMill*>	g_obMakeData;	// ���H�ް�

// ��ފ֐�
static	void	InitialVariable(void);			// �ϐ�������
static	void	SetStaticOption(void);			// �ÓI�ϐ��̏�����
static	BOOL	MakeNurbs_MainFunc(void);		// NC������Ҳ�ٰ��
static	BOOL	OutputNurbsCode(void);			// NC���ނ̏o��

// ��޽گ�ފ֐�
static	CCriticalSection	g_csMakeAfter;	// MakeNurbs_AfterThread()�گ��ۯ���޼ު��
static	UINT	MakeNurbs_AfterThread(LPVOID);	// ��n���گ��

//////////////////////////////////////////////////////////////////////
// NURBS�ȖʗpNC�����گ��
//////////////////////////////////////////////////////////////////////

UINT MakeNurbs_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeNurbs_Thread() Start\n");
#endif
	int		nResult = IDCANCEL;

	// ��۰��ٕϐ�������
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc = static_cast<C3dModelDoc *>(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);

	// �������\��
	g_nFase = 0;
	SendFaseMessage(g_pParent, g_nFase, -1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		// NC������߼�ݵ�޼ު�Ă̐����Ƶ�߼�݂̓ǂݍ���
		g_pMakeOpt = new CNCMakeMillOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKENURBS)->GetHead());
		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		InitialVariable();
		// �������Ƃɕω��������Ұ���ݒ�
		SetStaticOption();
		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
		// �������蓖��
		g_obMakeData.SetSize(0, 2048);
		// �����J�n
		BOOL bResult = MakeNurbs_MainFunc();
		if ( bResult )
			bResult = OutputNurbsCode();

		// �߂�l���
		if ( bResult && IsThread() )
			nResult = IDOK;

#ifdef _DEBUG
		printf("MakeNurbs_Thread All Over!!!\n");
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	// �I������
	_dp.SetDecimal3();
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��

	// ��������NC���ނ̏����گ��(�D��x��������)
	AfxBeginThread(MakeNurbs_AfterThread, NULL,
		THREAD_PRIORITY_IDLE);

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	CNCMakeMill::InitialVariable();		// CNCMakeBase::InitialVariable()
}

void SetStaticOption(void)
{
	// ������߼�݂ɂ��ÓI�ϐ��̏�����
	CNCMakeMill::SetStaticOption(g_pMakeOpt);
}

BOOL OutputNurbsCode(void)
{
	CString	strPath, strFile,
			strNCFile(g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_pParent, g_nFase, g_obMakeData.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		UINT	nOpenFlg = CFile::modeCreate | CFile::modeWrite |
			CFile::shareExclusive | CFile::typeText | CFile::osSequentialScan;
		CStdioFile	fp(strNCFile, nOpenFlg);
		for ( INT_PTR i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
			g_obMakeData[i]->WriteGcode(fp);
			SetProgressPos(g_pParent, i+1);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	SetProgressPos(g_pParent, g_obMakeData.GetSize());
	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC����Ҳݽگ��
//////////////////////////////////////////////////////////////////////

BOOL MakeNurbs_MainFunc(void)
{
	int		mx, my, mz, i, j, k;
	CNCMakeMill*	pNCD;
	Coord***	pScanCoord = g_pDoc->GetScanPathCoord();

	tie(mx, my) = g_pDoc->GetScanNumXY();
	SendFaseMessage(g_pParent, g_nFase, mx*my);

	for ( i=0; i<mx; i++ ) {
		for ( j=0; j<my; j++ ) {
			mz = g_pDoc->GetScanNumZ(j);
			for ( k=0; k<mz; k++ ) {
				pNCD = new CNCMakeMill(pScanCoord[i][j][k]);
				ASSERT( pNCD );
				g_obMakeData.Add(pNCD);
			}
			SetProgressPos(g_pParent, i*my+j);
		}
	}

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

// NC�����̸�۰��ٕϐ�������(��n��)�گ��
UINT MakeNurbs_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

#ifdef _DEBUG
	printf("MakeNurbs_AfterThread() Start\n");
#endif
	for ( int i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	g_obMakeData.RemoveAll();

	if ( g_pMakeOpt )
		delete	g_pMakeOpt;

	g_csMakeAfter.Unlock();

	return 0;
}
