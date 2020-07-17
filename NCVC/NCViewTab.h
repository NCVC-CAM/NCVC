// NCViewTab.h: CNCViewTab �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "TabView.h"
#include "ViewBase.h"
#include "NCViewSplit.h"
#include "NCdata.h"		// NCDRAWVIEW_NUM

class CNCViewTab;
class CNCListView;

/////////////////////////////////////////////////////////////////////////////
// CTraceThread �X���b�h

// �گ�ނւ̈���
typedef struct tagTRACETHREADPARAM {
	CMainFrame*		pMainFrame;
	CNCViewTab*		pParent;
	CNCListView*	pListView;
} TRACETHREADPARAM, *LPTRACETHREADPARAM;

class CTraceThread : public CWinThread
{
	CNCViewTab*		m_pParent;
	CNCListView*	m_pListView;

public:
/*
	CWinThread ����̔h���׽�𐶐����� m_pMainWnd ��Ă��Ȃ���
	AfxGetNCVCMainWnd() ���߲�����Q�Ƃł��Ȃ�
*/
	CTraceThread(LPTRACETHREADPARAM pParam) {
		m_bAutoDelete	= FALSE;				// CWinThread
		m_pMainWnd		= pParam->pMainFrame;	// �@�V
		m_pParent		= pParam->pParent;
		m_pListView		= pParam->pListView;
		delete	pParam;
	}

public:
	virtual BOOL	InitInstance();
	virtual int		ExitInstance();
};

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab

class CNCViewTab : public CTabViewBase
{
friend	class	CTraceThread;

	CNCViewSplit	m_wndSplitter1,		// �S��-1
					m_wndSplitter2,		// �S��-2
					m_wndSplitter22;

	HDC			m_hDC[NCDRAWVIEW_NUM];	// XYZ, XY, XZ, YZ �e�߰�ނ����޲���÷�������
	UINT		m_nTraceSpeed;		// �ڰ����s�̑��x
	CTraceThread*	m_pTraceThread;	// �ڰ����s�گ�������
	BOOL		m_bTraceContinue,	// �ڰ����s�p���׸�
				m_bTracePause;		// �ڰ��ꎞ��~
	CNCdata*	m_pDataTraceSel;	// �ڰ����s���̑I���ް�
	CEvent		m_evTrace;			// �ڰ��J�n�����(�ݽ�׸��ɂĎ蓮����Đݒ�)

	BOOL		m_bSplit[NCDRAWVIEW_NUM];	// ���د��\�����ꂽ���ǂ���

protected:
	CNCViewTab();		// ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCViewTab)

// �A�g���r���[�g
public:
	CNCDoc*	GetDocument();

// �I�y���[�V����
public:
	void	OnUserTraceStop(void) {		// from NCChild.cpp
		OnTraceStop();
	}
	void	DblClkChange(int nIndex) {	// from NCView*.cpp
		ActivatePage(nIndex);
		GetParentFrame()->SetActiveView(static_cast<CView *>(GetPage(nIndex)));
	}

// �I�[�o�[���C�h
public:
	virtual void OnInitialUpdate();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual	void OnActivatePage(int nIndex);

// �C���v�������e�[�V����
protected:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
//	afx_msg void OnSetFocus(CWnd* pOldWnd);
	// ��ވړ�
	afx_msg	void OnMoveTab(UINT);
	// �ڰ�
	afx_msg void OnUpdateTraceSpeed(CCmdUI* pCmdUI);
	afx_msg	void OnTraceSpeed(UINT);
	afx_msg void OnUpdateTraceRun(CCmdUI* pCmdUI);
	afx_msg void OnTraceRun();
	afx_msg void OnUpdateTracePause(CCmdUI* pCmdUI);
	afx_msg void OnTracePause();
	afx_msg void OnTraceStop();
	afx_msg void OnUpdateTraceCursor(CCmdUI* pCmdUI);
	afx_msg void OnTraceCursor(UINT);
	// �u�S�Ă��߲݂̐}�`̨�āv�ƭ�����ނ̎g�p����
	afx_msg	void OnUpdateAllFitCmd(CCmdUI* pCmdUI);
	// ��
	afx_msg void OnUpdateDefViewInfo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveKey (CCmdUI* pCmdUI);
	afx_msg void OnUpdateRoundKey(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline CNCDoc* CNCViewTab::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
