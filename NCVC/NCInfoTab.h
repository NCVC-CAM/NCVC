// NCInfoTab.h: CNCInfoTab �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "TabView.h"

class CNCInfoTab : public CTabViewBase  
{
protected:
	CNCInfoTab();		// ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCInfoTab)

// �A�g���r���[�g
public:
	CNCDoc*	GetDocument();

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
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnDestroy();
//	afx_msg void	OnSetFocus(CWnd* pOldWnd);
	// ��ވړ�
	afx_msg	void	OnMoveTab(UINT);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCInfoTab::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
