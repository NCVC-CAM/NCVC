// CustomMenu.h: CCustomMenu クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////
// CCustomMenu クラスのインターフェイス

class CCustomMenu : public CMenu  
{
#ifdef _DEBUG
	void	MAP_IMAGE_PRINT() const;
	void	VEC_MNEMONIC_PRINT() const;
#endif

protected:
	CFont			m_fontMenu;		// 文字の大きさ計算用
	CImageList		m_ilToolBar;	// ﾂｰﾙﾊﾞｰのﾎﾞﾀﾝｲﾒｰｼﾞ
	CDWordArray		m_arrayImage;	// ﾒﾆｭｰ項目 ID - ｲﾒｰｼﾞ順対応ﾏｯﾌﾟ
	CStringArray	m_arrayString;	// ﾒﾆｭｰ文字列

	int		FindString(const CString&);
	int		FindItemID(DWORD);
	CPoint	GetIconPoint(const CRect&);

	static	HICON	m_hCheckIcon;			// ﾁｪｯｸﾒﾆｭｰのｱｲｺﾝﾊﾝﾄﾞﾙ
	static	int		m_nIconFrameX;			// ｱｲｺﾝﾌﾚｰﾑｻｲｽﾞ
	static	int		m_nIconFrameY;

public:
	CCustomMenu();

// アトリビュート
public:

// オペレーション
public:
	BOOL	LoadToolBar(LPCTSTR lpszResourceName);
	BOOL	LoadToolBar(UINT nIDResource) {
		return LoadToolBar(MAKEINTRESOURCE(nIDResource));
	}
	void	RemoveMenuString(const CStringArray&);

protected:
	BOOL	LoadToolBarItem(LPCTSTR lpszResourceName);
	BOOL	LoadToolBarImage(LPCTSTR lpszResourceName);
	INT_PTR	SetMenuString(CMenu* pMenu, UINT nItem);
	void	DrawItemIconFrame(CDC* pDC, LPDRAWITEMSTRUCT lpDIS);
	void	DrawItemString(CDC* pDC, LPDRAWITEMSTRUCT lpDIS, BOOL bIcon);
	void	DrawItemStringText(CDC* pDC, CString strText, CRect rcText);
	void	DrawItemSeparator(CDC* pDC, LPDRAWITEMSTRUCT lpDIS);
	// 内部呼び出しもCCustomMenuEx::DrawItemIcon()を呼び出させるため virtual指定
	virtual	BOOL	DrawItemIcon(CDC* pDC, LPDRAWITEMSTRUCT lpDIS);

// オーバーライド

// インプリメンテーション
public:
	virtual ~CCustomMenu();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// ﾒｯｾｰｼﾞﾏｯﾌﾟ(の代わり)
	void	OnSysColorChange(int nSize, UINT nIDResource[]);
	void	OnInitMenuPopup(CMenu* pMenu);
	void	OnDrawItem(LPDRAWITEMSTRUCT lpDIS);
	void	OnMeasureItem(LPMEASUREITEMSTRUCT lpMIS);
};

//////////////////////////////////////////////////////////////////////
// CCustomMenuEx クラスのインターフェイス

//	ｶｽﾀﾑﾒﾆｭｰのｲﾒｰｼﾞ情報
typedef	struct	tagCUSTMENUINFO {
	WORD	nImage;		// ｲﾒｰｼﾞﾘｽﾄ配列
	WORD	nIndex;		// ｲﾒｰｼﾞﾘｽﾄ内のｲﾒｰｼﾞ№
} CUSTMENUINFO, *LPCUSTMENUINFO;

class CCustomMenuEx : public CCustomMenu
{
	// ｲﾒｰｼﾞﾘｽﾄのﾎﾟｲﾝﾀ配列
	CTypedPtrArrayEx<CObArray, CImageList*>	m_pilEnable, m_pilDisable;
	// ﾒﾆｭｰｺﾏﾝﾄﾞIDをｷｰにしたｲﾒｰｼﾞ情報
	CTypedPtrMap<CMapWordToPtr, WORD, LPCUSTMENUINFO>	m_mpImage;

protected:
	// ｶｽﾀﾑｲﾒｰｼﾞﾎﾞﾀﾝを描画するためのｵｰﾊﾞｰﾗｲﾄﾞ
	BOOL	DrawItemIcon(CDC* pDC, LPDRAWITEMSTRUCT lpDIS);

public:
	virtual ~CCustomMenuEx();

	void	SetMapImageID(WORD, WORD, WORD);
	void	RemoveCustomImageMap(size_t, LPWORD);

	void	AddCustomEnableImageList(CImageList* pilEnable) {
		m_pilEnable.Add( pilEnable );
	}
	void	AddCustomDisableImageList(CImageList* pilDisable) {
		m_pilDisable.Add( pilDisable );
	}
};
