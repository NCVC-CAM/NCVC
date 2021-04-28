// CustomToolBar.h: CCustomToolBar �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

// °��ްؿ���̍\��
struct CToolBarItem
{
	WORD wVersion;		// �ް�ޮ� (=1)
	WORD wWidth;		// ��
	WORD wHeight;		// ����
	WORD wItemCount;	// ���ݐ�
	//WORD aItems[wItemCount]	// �ƭ����� (�����) ID �̂Ȃ��
	WORD* items()
		{ return (WORD*)(this+1); }
};

//	��̫��°��ް��`���
struct CUSTTBBUTTON
{
    int		idCommand;	// ���݂������ꂽ�Ƃ��ɑ���������ID(UINT?)
    BYTE	fsState;	// ���݂̏��
    BYTE	fsStyle;	// ���ݽ���
	BOOL	bDisplay;	// �\���׸�
};
typedef	CUSTTBBUTTON*	LPCUSTTBBUTTON;

//	�������ݏ��
struct	CUSTTBINFO
{
	TBBUTTON	tb;
	CString		strInfo;
};
typedef	CUSTTBINFO*		LPCUSTTBINFO;

//////////////////////////////////////////////////////////////////////
// CCustomToolBar �N���X�̃C���^�[�t�F�C�X

class CCustomToolBar : public CToolBar  
{
protected:
	LPCTSTR			m_lpszResourceName;	// °��ް��ؿ����
	LPCUSTTBBUTTON	m_lpCustTbButtons;	// �����\���p°��ް���ݒ�`
	// ���ݏ��(TBBUTTON�\���̊i�[)
	CTypedPtrArray<CPtrArray, LPCUSTTBINFO>	m_arButton;

public:
	CCustomToolBar();

// �A�g���r���[�g
public:

// �I�y���[�V����
protected:
	void	RestoreState(void);
	void	SaveState(void);
	CString	GetSubKey(void);
	CString	GetValueName(void);

	BOOL	LoadToolBarItem(LPCTSTR lpszResourceName,
				CMap<WORD, WORD&, int, int&>& mapImage);
	void	RemoveAllButtons(void);

// �I�[�o�[���C�h

// �C���v�������e�[�V����
public:
	virtual ~CCustomToolBar();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	BOOL	CreateExEx(CWnd* pParentWnd, LPCTSTR lpszTitle, UINT nID = AFX_IDW_TOOLBAR,
				DWORD dwCtrlStyle = TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_WRAPABLE|
						CCS_ADJUSTABLE,
				DWORD dwStyle = WS_CHILD|WS_VISIBLE|
						CBRS_TOP|CBRS_GRIPPER|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC,
				CRect rcBorders = CRect(0, 0, 0, 0));
	BOOL	LoadToolBarEx(LPCTSTR lpszResourceName, LPCUSTTBBUTTON lpCustTbButtons, BOOL bRestore = TRUE);
	BOOL	LoadToolBarEx(UINT nIDResource, LPCUSTTBBUTTON lpCustTbButtons, BOOL bRestore = TRUE) {
		return LoadToolBarEx(MAKEINTRESOURCE(nIDResource), lpCustTbButtons, bRestore);
	}
	BOOL	SetCustomButtons(LPCTSTR lpszResourceName,
				CImageList* pilEnable, CImageList* pilDisable, CUSTTBINFO tbInfo[],
				BOOL bRestore = TRUE);

	// ү����ϯ�ߊ֐�
protected:
	afx_msg void OnDestroy();
	afx_msg void OnNotifyQueryInsertOrDelete(NMHDR* pTbNotify, LRESULT* pResult);
	afx_msg void OnNotifyGetButtonInfo(NMHDR* pTbNotify, LRESULT* pResult);
	afx_msg void OnNotifyReset(NMHDR* pNmhdr, LRESULT* pResult);
	afx_msg void OnNotifyToolBarChange(NMHDR* pNmhdr, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};
