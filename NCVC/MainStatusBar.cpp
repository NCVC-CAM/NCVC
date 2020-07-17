// MainStatusBar.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainStatusBar.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainStatusBar

BEGIN_MESSAGE_MAP(CMainStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CMainStatusBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USERPROGRESSPOS, OnUserProgressPos)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainStatusBar �N���X�̍\�z/����

CMainStatusBar::CMainStatusBar()
{
}

CMainStatusBar::~CMainStatusBar()
{
}

/////////////////////////////////////////////////////////////////////////////
// CMainStatusBar ���b�Z�[�W �n���h��

int CMainStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CStatusBar::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// �ð���ް����۸�ڽ���۰ق�z�u
	if ( !m_ctProgress.Create(WS_CHILD|WS_VISIBLE, CRect(0, 0, 0, 0),
			this, ID_MAIN_PROGRESS) ) {
		TRACE0("Failed to create progress control\n");
		return -1;      // �쐬�Ɏ��s
	}

	// XpStyle����
	::DisableXpStyle(m_ctProgress.m_hWnd);	// StdAfx.h

	return 0;
}

void CMainStatusBar::ChangeProgressSize(int nIndex, int nWidth) 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CMainStatusBar::ChangeProgressSize()");
	dbg.printf("nWidth=%d", nWidth);
#endif
	if ( nWidth <= 0 )
		return;
	SetPaneInfo(nIndex, ID_MAIN_PROGRESS, SBPS_NOBORDERS|SBPS_NORMAL, nWidth);
	CRect	rc;
	GetItemRect(nIndex, &rc);
	m_ctProgress.MoveWindow(&rc);
#ifdef _DEBUG
	dbg.printf("top=%d left=%d bottom=%d right=%d",
		rc.top, rc.left, rc.bottom, rc.right);
#endif
}

LRESULT CMainStatusBar::OnUserProgressPos(WPARAM wParam, LPARAM)
{
	m_ctProgress.SetPos((int)wParam);
	return 0;
}
