// TabView.h: CTabViewBase �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Tab���۰ق��ޭ��׽���ڂ����{�׽

class CTabViewBase : public CCtrlView  
{
	CTypedPtrArray<CObArray, CWnd*> m_pPages;

public:
	CTabViewBase() : CCtrlView(WC_TABCONTROL,
		AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|TCS_FOCUSNEVER) {}
	DECLARE_DYNCREATE(CTabViewBase)

// �A�g���r���[�g
public:
	CTabCtrl& GetTabCtrl() const {
		return *(CTabCtrl*)this;
	}

// �I�y���[�V����
public:
	int		AddPage(LPCTSTR, CRuntimeClass*, CDocument*, CFrameWnd*);
	int		AddPage(LPCTSTR, CWnd*);
	void	RemovePage(int);
	int		GetPageCount(void) {
		return GetTabCtrl().GetItemCount();
	}
	BOOL	GetPageTitle(int, CString&);
	BOOL	SetPageTitle(int, LPCTSTR);

	void	ActivatePage(int);
	int		NextActivatePage(void);
	int		PrevActivatePage(void);
	int		GetActivePage(void) {
		return GetTabCtrl().GetCurSel();
	}
	CWnd*	GetPage(int nIndex) {
		return ( nIndex>=0 && nIndex<GetPageCount() ) ? m_pPages[nIndex] : NULL;
	}
	CWnd*	GetActivePageWnd(void) {
		return GetPage(GetActivePage());
	}

protected:
	void	ResizePage(CWnd*);

// �I�[�o�[���C�h
protected:
	virtual	BOOL	OnInitPage(int nIndex) {
		return TRUE;
	}
	virtual	void	OnActivatePage(int nIndex) {}
	virtual	void	OnDeactivatePage(int nIndex) {}
	virtual	void	OnDestroyPage(int nIndex) {}

// �������ꂽ���b�Z�[�W �}�b�v�֐�
	afx_msg	void	OnSize(UINT nType, int cx, int cy);
	afx_msg	void	OnDestroy();
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg	void	OnSelChanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg	void	OnSelChange(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};
