// 3dModelChild.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "3dModelChild.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// C3dModelChild

IMPLEMENT_DYNCREATE(C3dModelChild, CMDIChildWnd)

BEGIN_MESSAGE_MAP(C3dModelChild, CChildBase)
	ON_WM_MDIACTIVATE()
END_MESSAGE_MAP()

C3dModelChild::C3dModelChild()
{
}

C3dModelChild::~C3dModelChild()
{
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelChild ���b�Z�[�W �n���h���[

void C3dModelChild::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
	__super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
#ifdef _DEBUG
	printf("C3dModelChild::bActivate=%d\n", bActivate);
#endif
	if ( bActivate ) {
		// Ӱ��ڽ�޲�۸ނւ��޷���Đؑ֒ʒm
		AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
		// �A�N�Z�����[�^�e�[�u���̋����u��
		// DestroyAcceleratorTable() ���Ȃ��Ă������H
		m_hAccelTable = NULL;	// ���ꂪ�Ȃ���MFC����ASSERT
		LoadAccelTable( MAKEINTRESOURCE(IDR_3DMTYPE) );
	}
}
