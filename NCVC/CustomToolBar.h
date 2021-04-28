// CustomToolBar.h: CCustomToolBar クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

// ﾂｰﾙﾊﾞｰﾘｿｰｽの構造
struct CToolBarItem
{
	WORD wVersion;		// ﾊﾞｰｼﾞｮﾝ (=1)
	WORD wWidth;		// 幅
	WORD wHeight;		// 高さ
	WORD wItemCount;	// ﾎﾞﾀﾝ数
	//WORD aItems[wItemCount]	// ﾒﾆｭｰ項目 (ｺﾏﾝﾄﾞ) ID のならび
	WORD* items()
		{ return (WORD*)(this+1); }
};

//	ﾃﾞﾌｫﾙﾄﾂｰﾙﾊﾞｰ定義情報
struct CUSTTBBUTTON
{
    int		idCommand;	// ﾎﾞﾀﾝが押されたときに送られるｺﾏﾝﾄﾞID(UINT?)
    BYTE	fsState;	// ﾎﾞﾀﾝの状態
    BYTE	fsStyle;	// ﾎﾞﾀﾝｽﾀｲﾙ
	BOOL	bDisplay;	// 表示ﾌﾗｸﾞ
};
typedef	CUSTTBBUTTON*	LPCUSTTBBUTTON;

//	ｶｽﾀﾑﾎﾞﾀﾝ情報
struct	CUSTTBINFO
{
	TBBUTTON	tb;
	CString		strInfo;
};
typedef	CUSTTBINFO*		LPCUSTTBINFO;

//////////////////////////////////////////////////////////////////////
// CCustomToolBar クラスのインターフェイス

class CCustomToolBar : public CToolBar  
{
protected:
	LPCTSTR			m_lpszResourceName;	// ﾂｰﾙﾊﾞｰのﾘｿｰｽ名
	LPCUSTTBBUTTON	m_lpCustTbButtons;	// 初期表示用ﾂｰﾙﾊﾞｰﾎﾞﾀﾝ定義
	// ﾎﾞﾀﾝ情報(TBBUTTON構造体格納)
	CTypedPtrArray<CPtrArray, LPCUSTTBINFO>	m_arButton;

public:
	CCustomToolBar();

// アトリビュート
public:

// オペレーション
protected:
	void	RestoreState(void);
	void	SaveState(void);
	CString	GetSubKey(void);
	CString	GetValueName(void);

	BOOL	LoadToolBarItem(LPCTSTR lpszResourceName,
				CMap<WORD, WORD&, int, int&>& mapImage);
	void	RemoveAllButtons(void);

// オーバーライド

// インプリメンテーション
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

	// ﾒｯｾｰｼﾞﾏｯﾌﾟ関数
protected:
	afx_msg void OnDestroy();
	afx_msg void OnNotifyQueryInsertOrDelete(NMHDR* pTbNotify, LRESULT* pResult);
	afx_msg void OnNotifyGetButtonInfo(NMHDR* pTbNotify, LRESULT* pResult);
	afx_msg void OnNotifyReset(NMHDR* pNmhdr, LRESULT* pResult);
	afx_msg void OnNotifyToolBarChange(NMHDR* pNmhdr, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};
