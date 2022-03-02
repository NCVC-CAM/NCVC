// TH_MakeNurbs.cpp
//		NURBS�ȖʗpNC����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "3dModelDoc.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ��۰��ٕϐ���`
static	CThreadDlg*			g_pParent;
static	C3dModelDoc*		g_pDoc;

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	IsThread()	g_pParent->IsThreadContinue()

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

	// �I������
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��

	return 0;
}
