// CustomMenu.cpp: CCustomMenu クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "CustomMenu.h"
#include "CustomToolBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef	_DEBUGOLD
//#define	_DEBUGOLD
#endif

// ｱｲｺﾝｻｲｽﾞ
extern	const	int		gg_nIconX;
extern	const	int		gg_nIconY;

// ﾆｰﾓﾆｯｸ (ﾒﾆｭｰ文字列) 描画位置
static	const	int		g_nMenuBack = 1;
static	const	int		g_nMenuLeft = 5;
static	const	int		g_nMenuRight = gg_nIconX - 1;

//
HICON	CCustomMenu::m_hCheckIcon = NULL;
int		CCustomMenu::m_nIconFrameX = 0;
int		CCustomMenu::m_nIconFrameY = 0;

//////////////////////////////////////////////////////////////////////
// CCustomMenu クラスの構築/消滅

CCustomMenu::CCustomMenu()
{
#ifdef _DEBUGOLD
	printf("CCustomMenu::CCustomMenu() Start\n");
#endif

	// 非ｸﾗｲｱﾝﾄ領域に関するﾒﾄﾘｯｸ値 (ﾒﾆｭｰﾊﾞｰの高さ等) 取得
	NONCLIENTMETRICS	nclm;
	::ZeroMemory(&nclm, sizeof(NONCLIENTMETRICS));
	nclm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY( ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
							sizeof(NONCLIENTMETRICS), &nclm, 0) );

	// ﾌｫﾝﾄ作成
	m_fontMenu.CreateFontIndirect(&nclm.lfMenuFont);

	// 静的変数の初期化
	if ( !m_hCheckIcon ) {
		// ﾁｪｯｸﾒﾆｭｰのｱｲｺﾝﾊﾝﾄﾞﾙ取得
		m_hCheckIcon = (HICON)::LoadImage(AfxGetResourceHandle(),
				MAKEINTRESOURCE(IDI_CHECK), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
		ASSERT(m_hCheckIcon);
		// ｱｲｺﾝﾌﾚｰﾑｻｲｽﾞ (ｱｲｺﾝそのものではなく、まわりのﾌﾚｰﾑの大きさ)
		m_nIconFrameX = m_nIconFrameY = nclm.iMenuHeight + 2;
	}

	// ｲﾒｰｼﾞﾘｽﾄの初期化
	m_ilToolBar.Create(gg_nIconX, gg_nIconY, ILC_COLORDDB|ILC_MASK, 1, 1);
}

CCustomMenu::~CCustomMenu()
{
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenu クラスの診断

#ifdef _DEBUG
void CCustomMenu::AssertValid() const
{
	CMenu::AssertValid();
}

void CCustomMenu::Dump(CDumpContext& dc) const
{
	CMenu::Dump(dc);
}
#endif
#ifdef _DEBUGOLD
void CCustomMenu::MAP_IMAGE_PRINT() const
{
	printf("<MAP_IMAGE_PRINT> ------------------- start ------------------- size=%d\n", m_arrayImage.GetSize());
	for ( int i=0; i<m_arrayImage.GetSize(); i++ ) {
		printf("<MAP_IMAGE_PRINT> i=%d itemID=%d\n", i, m_arrayImage[i]);
	}
	printf("<MAP_IMAGE_PRINT> -------------------  end  -------------------\n");
}

void CCustomMenu::VEC_MNEMONIC_PRINT() const
{
	printf("<VEC_MNEMONIC_PRINT> ------------------- start ------------------- size=%d\n", m_arrayString.GetSize());
	for ( int i=0; i<m_arrayString.GetSize(); i++ ) {
		printf("<VEC_MNEMONIC_PRINT> i=%d, arrayString=%s\n", i, LPCTSTR(m_arrayString[i]));
	}
	printf("<VEC_MNEMONIC_PRINT> -------------------  end  -------------------\n");
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CCustomMenu ﾒﾝﾊﾞ関数

BOOL CCustomMenu::LoadToolBar(LPCTSTR lpszResourceName)
{
	BOOL	bResult = LoadToolBarItem(lpszResourceName);
	if ( bResult )
		bResult = LoadToolBarImage(lpszResourceName);

	return bResult;
}

BOOL CCustomMenu::LoadToolBarItem(LPCTSTR lpszResourceName)
{
	HINSTANCE	hInst = AfxFindResourceHandle(lpszResourceName, RT_TOOLBAR);
	HRSRC		hRsrc = ::FindResource(hInst, lpszResourceName, RT_TOOLBAR);
	if ( !hRsrc )
		return FALSE;

	HGLOBAL hGlobal = ::LoadResource(hInst, hRsrc);
	if ( !hGlobal )
		return FALSE;
	CToolBarItem* pBarItem = (CToolBarItem *)::LockResource(hGlobal);
	if ( !pBarItem ) {
		::FreeResource(hGlobal);
		return FALSE;
	}
	ASSERT(pBarItem->wVersion==1);

	for( int i=0, iImage=0; i<pBarItem->wItemCount; i++ ) {
		UINT itemID = pBarItem->items()[i];
		// ｾﾊﾟﾚｰﾀ以外
		if ( itemID ) {
			// ﾒﾆｭｰ項目 ID - ｲﾒｰｼﾞ順対応ﾏｯﾌﾟ
			m_arrayImage.Add(itemID);
		}
	}

	::UnlockResource(hGlobal);
	::FreeResource(hGlobal);

#ifdef _DEBUGOLD
	MAP_IMAGE_PRINT();
#endif

	return TRUE;
}

BOOL CCustomMenu::LoadToolBarImage(LPCTSTR lpszResourceName)
{
	HINSTANCE	hInst = AfxFindResourceHandle(lpszResourceName, RT_BITMAP);
	HRSRC		hRsrc = ::FindResource(hInst, lpszResourceName, RT_BITMAP);
	if ( !hRsrc )
		return FALSE;
	// ｼｽﾃﾑｶﾗｰで調整したｲﾒｰｼﾞ取得
	HBITMAP hBitmap = AfxLoadSysColorBitmap(hInst, hRsrc);	// ← MFC\SRC\BARTOOL.CPP(59)
	ASSERT(hBitmap);

	// ｲﾒｰｼﾞﾘｽﾄへ登録
	CBitmap* pBitmap = CBitmap::FromHandle(hBitmap);
	m_ilToolBar.Add(pBitmap, ::GetSysColor(COLOR_BTNFACE));	// ← 厳密には色が違う時もある

	return TRUE;
}

int CCustomMenu::FindString(const CString& strMenu)
{
	for ( int i=0; i<m_arrayString.GetSize(); i++ )
		if ( m_arrayString[i] == strMenu )
			return i;
	return -1;
}

int CCustomMenu::FindItemID(DWORD nID)
{
	for ( int i=0; i<m_arrayImage.GetSize(); i++ )
		if ( m_arrayImage[i] == nID )
			return i;
	return -1;
}

CPoint CCustomMenu::GetIconPoint(const CRect& rc)
{
	CPoint	ptIcon(rc.left + ((m_nIconFrameX - gg_nIconX) >> 1),
				rc.top + ((m_nIconFrameY - gg_nIconY + 1) >> 1) );
	return ptIcon;
}

INT_PTR CCustomMenu::SetMenuString(CMenu* pMenu, UINT nItem)
{
	INT_PTR	nRet;

	// ﾒﾆｭｰ文字列取得
	CString strMenu;
	pMenu->GetMenuString(nItem, strMenu, MF_BYPOSITION);
	// ﾒﾆｭｰ文字列に登録
	if ( strMenu.GetLength() > 0 ) {
		nRet = FindString(strMenu);
		if ( nRet < 0 )
			nRet = m_arrayString.Add(strMenu);
	}
	else
		nRet = -1;

	return nRet;
}

void CCustomMenu::RemoveMenuString(const CStringArray& strArray)
{
	int		nIndex;

	for ( int i=0; i<strArray.GetSize(); i++ ) {
		nIndex = FindString(strArray[i]);
		if ( nIndex >= 0 )
			m_arrayString.RemoveAt(nIndex);
	}
}

BOOL CCustomMenu::DrawItemIcon(CDC* pDC, LPDRAWITEMSTRUCT lpDIS)
{
#ifdef _DEBUGOLD
	printf("CCustomMenu::DrawItemIcon() Start\n");
#endif
	HICON hIcon = NULL;					// ｱｲｺﾝﾊﾝﾄﾞﾙ
	BOOL bRet = FALSE;					// ｱｲｺﾝ描画ﾌﾗｸﾞ
	BOOL bCheck = FALSE;				// ﾁｪｯｸﾒﾆｭｰの背景ｲﾒｰｼﾞ描画ﾌﾗｸﾞ
	UINT itemID = lpDIS->itemID;		// ﾒﾆｭｰ項目 ID
	UINT itemState = lpDIS->itemState;	// 描画動作
	CRect rcItem(lpDIS->rcItem);		// 描画矩形
	// ｱｲｺﾝ描画位置
	CPoint ptIcon(GetIconPoint(rcItem));
	// ﾌﾚｰﾑ描画領域
	CRect rcFrame(rcItem.left, rcItem.top,
			rcItem.left + m_nIconFrameX, rcItem.top + m_nIconFrameX);

	// ｱｲｺﾝ取得
	int nRet = FindItemID(itemID);
	if ( nRet >= 0 ) {
		// ﾒﾆｭｰ項目 ID - ｲﾒｰｼﾞ順対応ﾏｯﾌﾟにあり
		hIcon = m_ilToolBar.ExtractIcon(nRet);
	}

	// ｱｲｺﾝ描画状態ｾｯﾄ
	UINT nFlags = DST_ICON | (itemState & ODS_GRAYED ? DSS_DISABLED : DSS_NORMAL);

	// ｱｲｺﾝ描画ﾌﾗｸﾞｾｯﾄ
	if ( itemState & ODS_CHECKED ) {
		// ﾁｪｯｸ状態
		if ( !hIcon )
			hIcon = m_hCheckIcon;	// ｱｲｺﾝがなければﾁｪｯｸﾒﾆｭｰｱｲｺﾝを使用
		bRet = TRUE;
		bCheck = TRUE;
	}
	else {
		// ｱｲｺﾝがあれば描画
		if ( hIcon )
			bRet = TRUE;
	}

	// ﾁｪｯｸﾒﾆｭｰの背景を描画
	if ( bCheck ) {
		if ( itemState & ODS_SELECTED )
			pDC->FillSolidRect(&rcFrame, ::GetSysColor(COLOR_MENU));
		else {
			COLORREF colSysColor = (COLORREF)::GetSysColor(COLOR_3DHILIGHT);
			// ﾃﾞｨｻﾞﾘﾝｸﾞ対応
			for ( int y=rcFrame.top; y<rcFrame.bottom; y++ )
				for ( int x=rcFrame.left; x<rcFrame.right; x++ )
					if ( (x + y) % 2 )
						pDC->SetPixelV(x, y, colSysColor);
		}
	}

	// ｱｲｺﾝ描画
	if ( bRet )
		pDC->DrawState(ptIcon, 0, hIcon, nFlags, (CBrush *)NULL);

	return bRet;
}

void CCustomMenu::DrawItemIconFrame(CDC* pDC, LPDRAWITEMSTRUCT lpDIS)
{
	UINT itemID = lpDIS->itemID;		// ﾒﾆｭｰ項目 ID
	UINT itemState = lpDIS->itemState;	// 描画動作
	CRect rcItem(lpDIS->rcItem);		// 描画矩形
	COLORREF clrTopLeft, clrBottomRight;// ﾌﾚｰﾑ左上描画色、右下部描画色
	// ﾌﾚｰﾑ描画領域
	CRect rcFrame(rcItem.left, rcItem.top, rcItem.left + m_nIconFrameX, rcItem.top + m_nIconFrameX);

	// ﾌﾚｰﾑ描画色ｾｯﾄ
	if ( itemState & ODS_CHECKED ) {
		// ﾁｪｯｸ状態
		clrTopLeft = ::GetSysColor(COLOR_3DSHADOW);
		clrBottomRight = ::GetSysColor(COLOR_3DHILIGHT);
	}
	else if ( itemState & ODS_GRAYED ) {
		// 淡色状態
		clrTopLeft = clrBottomRight = ::GetSysColor(COLOR_MENU);
	}
	else if ( itemState & ODS_SELECTED ) {
		// 選択状態
		clrTopLeft = ::GetSysColor(COLOR_3DHILIGHT);
		clrBottomRight = ::GetSysColor(COLOR_3DSHADOW);
	}
	else {
		// 通常状態
		clrTopLeft = clrBottomRight = ::GetSysColor(COLOR_MENU);
	}

	// ﾌﾚｰﾑ描画
	pDC->Draw3dRect(&rcFrame, clrTopLeft, clrBottomRight);
}

void CCustomMenu::DrawItemString(CDC* pDC, LPDRAWITEMSTRUCT lpDIS, BOOL bIcon)
{
	UINT itemID = lpDIS->itemID;		// ﾒﾆｭｰ項目 ID
	UINT itemState = lpDIS->itemState;	// 描画動作
	CRect rcItem(lpDIS->rcItem);		// 描画矩形
	int nIndex = (int)lpDIS->itemData;	// 文字列取得用
	COLORREF clrText, clrBack;			// 文字列前景色、背景色
#ifdef _DEBUGOLD
	printf("DrawItemString() itemID=%d nIndex=%d", itemID, nIndex);
#endif

	// 描画状態ｾｯﾄ
	if ( itemState & ODS_GRAYED ) {
		// 淡色状態
		if( itemState & ODS_SELECTED ) {
			// 選択状態
			clrText = ::GetSysColor(COLOR_GRAYTEXT);
			clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
			if ( clrText == clrBack )
				clrText = ::GetSysColor(COLOR_MENU);
		}
		else {
			// 通常状態
			clrText = ::GetSysColor(COLOR_GRAYTEXT);
			clrBack = ::GetSysColor(COLOR_MENU);
		}
	}
	else {
		// 淡色状態でない
		if ( itemState & ODS_SELECTED ) {
			// 選択状態
			clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
		}
		else {
			// 通常状態
			clrText = ::GetSysColor(COLOR_MENUTEXT);
			clrBack = ::GetSysColor(COLOR_MENU);
		}
	}

	// 背景描画
	CRect rcBack(rcItem);
	if ( bIcon )
		rcBack.left += m_nIconFrameX + g_nMenuBack;
	pDC->FillSolidRect(&rcBack, clrBack);

	// ﾒﾆｭｰ文字列取得
	CString strMenu;
	if( 0 <= nIndex && nIndex < m_arrayString.GetSize() )
		strMenu = m_arrayString[nIndex];

	// ﾒﾆｭｰ文字列描画
	CRect rcMnemonic(rcItem);
	rcMnemonic.left += m_nIconFrameX + g_nMenuLeft;
	rcMnemonic.right -= g_nMenuRight;
	pDC->SetBkMode(TRANSPARENT);
	if ( (itemState & ODS_GRAYED) && !(itemState & ODS_SELECTED) ) {
		// 淡色描画 (ﾃﾞｨｻﾞﾘﾝｸﾞ対応)
		CRect rcHilight(rcMnemonic + CPoint(1, 1));
		pDC->SetTextColor(::GetSysColor(COLOR_3DHILIGHT));
		DrawItemStringText(pDC, strMenu, rcHilight);
	}
	pDC->SetTextColor(clrText);
	DrawItemStringText(pDC, strMenu, rcMnemonic);
}

void CCustomMenu::DrawItemStringText(CDC* pDC, CString strText, CRect rcText)
{
	int nPos = strText.Find(_T('\t'));
	if ( nPos > 0 ) {
		// ｼｮｰﾄｶｯﾄｷｰあり ('\t' が存在すればそれ以降をｼｮｰﾄｶｯﾄｷｰとする)
		pDC->DrawText(strText.Left(nPos), rcText,
			DT_LEFT|DT_SINGLELINE|DT_VCENTER);
		pDC->DrawText(strText.Right(strText.GetLength() - nPos - 1), rcText,
			DT_RIGHT|DT_SINGLELINE|DT_VCENTER);
	}
	else {
		// ｼｮｰﾄｶｯﾄｷｰなし
		pDC->DrawText(strText, rcText, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
	}
}

void CCustomMenu::DrawItemSeparator(CDC* pDC, LPDRAWITEMSTRUCT lpDIS)
{
	CRect rcItem(lpDIS->rcItem);		// 描画矩形

	rcItem.top = rcItem.bottom = (rcItem.top + rcItem.bottom) >> 1;
	rcItem.left += gg_nIconX >> 1;		// ｾﾊﾟﾚｰﾀの右/左の隙間はｱｲｺﾝの 1/2 ぐらいが適当か?
	rcItem.right -= gg_nIconX >> 1;
	pDC->DrawEdge(&rcItem, EDGE_ETCHED, BF_TOP);
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenu メッセージ ハンドラ

void CCustomMenu::OnInitMenuPopup(CMenu* pMenu)
{
	ASSERT(pMenu);
	int		i, nIndex;

	HMENU hMenu = pMenu->GetSafeHmenu();
	ASSERT(hMenu);

	MENUITEMINFO mii;
	// 該当ﾒﾆｭｰをﾙｰﾌﾟ
	for ( i=0; i<pMenu->GetMenuItemCount(); i++ ) {
		// ﾒﾆｭｰ文字列ﾍﾞｸﾀに登録
		nIndex = (int)SetMenuString(pMenu, i);
		if ( nIndex >= 0 ) {
			// ｵｰﾅｰ描画設定
			::ZeroMemory(&mii, sizeof(MENUITEMINFO));
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_TYPE|MIIM_DATA;
			if ( ::GetMenuItemInfo(hMenu, i, TRUE, &mii) ) {
				mii.fType |= MFT_OWNERDRAW;
				mii.dwItemData = (DWORD)nIndex;
				::SetMenuItemInfo(hMenu, i, TRUE, &mii);
			}
		}
	}

#ifdef _DEBUGOLD
	VEC_MNEMONIC_PRINT();
#endif
}

void CCustomMenu::OnMeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	ASSERT(lpMIS);
	UINT itemID = lpMIS->itemID;		// ﾒﾆｭｰ項目 ID
	int nIndex = (int)lpMIS->itemData;	// 文字列取得用

	// ｾﾊﾟﾚｰﾀの判定
	if ( itemID ) {
		// 通常ﾒﾆｭｰ
		// ﾒﾆｭｰ文字列
		CString strMenu;
		if ( 0 <= nIndex && nIndex < m_arrayString.GetSize() )
			strMenu = m_arrayString[nIndex];

		// 大きさ計算
		CClientDC dc(AfxGetMainWnd());
		CFont* pOldFont = dc.SelectObject(&m_fontMenu);
		CSize size = dc.GetTextExtent(strMenu);
		dc.SelectObject(pOldFont);
		lpMIS->itemWidth = m_nIconFrameX + size.cx + g_nMenuLeft + g_nMenuRight;
		lpMIS->itemHeight = m_nIconFrameY;		// ｱｲｺﾝﾌﾚｰﾑと同じ高さ
	}
	else {
		// ｾﾊﾟﾚｰﾀ
		lpMIS->itemWidth = m_nIconFrameX;		// とくに意味はない
		lpMIS->itemHeight = m_nIconFrameY >> 1;	// ｾﾊﾟﾚｰﾀの高さは通常ﾒﾆｭｰの 1/2 ぐらいが適当か?
	}
}

void CCustomMenu::OnDrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(lpDIS);

	// 初期処理
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	// ｾﾊﾟﾚｰﾀの判定
	if ( lpDIS->itemID ) {
		// 通常ﾒﾆｭｰ
		// ｱｲｺﾝ描画
		BOOL bIcon;								// ｱｲｺﾝ描画ﾌﾗｸﾞ
		if ( bIcon = DrawItemIcon(pDC, lpDIS) )
			DrawItemIconFrame(pDC, lpDIS);		// ｱｲｺﾝを描画したらｱｲｺﾝﾌﾚｰﾑも描画

		// ﾒﾆｭｰ文字列描画
		DrawItemString(pDC, lpDIS, bIcon);
	}
	else {
		// ｾﾊﾟﾚｰﾀ描画
		DrawItemSeparator(pDC, lpDIS);			
	}
}

void CCustomMenu::OnSysColorChange(int nSize, UINT nIDResource[])
{
	int		i;
	// 一度ｲﾒｰｼﾞｱｲﾃﾑを削除して再登録
	int nCnt = m_ilToolBar.GetImageCount();
	for ( i=0; i<nCnt; i++ )
		m_ilToolBar.Remove(0);
	for ( i=0; i<nSize; i++ )
		LoadToolBarImage(MAKEINTRESOURCE(nIDResource[i]));
}

//////////////////////////////////////////////////////////////////////
// CCustomMenuEx クラスの構築/消滅

CCustomMenuEx::~CCustomMenuEx()
{
	WORD		wKey;
	LPCUSTMENUINFO	pInfo;
	PMAP_FOREACH(wKey, pInfo, &m_mpImage)
		delete	pInfo;
	END_FOREACH
	m_mpImage.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenuEx ﾒﾝﾊﾞ関数

void CCustomMenuEx::SetMapImageID(WORD wKey, WORD nImage, WORD nIndex)
{
	LPCUSTMENUINFO pLookup;
	LPCUSTMENUINFO pInfo = new CUSTMENUINFO;
	pInfo->nImage = nImage;
	pInfo->nIndex = nIndex;
	if ( m_mpImage.Lookup(wKey, pLookup) )	// 保険
		delete	pLookup;
	m_mpImage.SetAt(wKey, pInfo);
}

void CCustomMenuEx::RemoveCustomImageMap(size_t nCnt, LPWORD wKey)
{
	LPCUSTMENUINFO	pInfo;
	for ( size_t i=0; i<nCnt; i++ ) {
		if ( m_mpImage.Lookup(wKey[i], pInfo) ) {
			delete	pInfo;
			m_mpImage.RemoveKey(wKey[i]);
		}
	}
}

BOOL CCustomMenuEx::DrawItemIcon(CDC* pDC, LPDRAWITEMSTRUCT lpDIS)
{
	LPCUSTMENUINFO	pInfo;
	if ( !m_mpImage.Lookup((WORD)lpDIS->itemID, pInfo) )
		return CCustomMenu::DrawItemIcon(pDC, lpDIS);

#ifdef _DEBUGOLD
	printf("CCustomMenuEx::DrawItemIcon() Start\n");
#endif
	ASSERT( pInfo );

	if ( pInfo->nImage < 0 ||
			pInfo->nImage >= m_pilEnable.GetSize() ||
			pInfo->nImage >= m_pilDisable.GetSize() ||
		pInfo->nIndex < 0 ||
			pInfo->nIndex >= m_pilEnable[pInfo->nImage]->GetImageCount() ||
			pInfo->nIndex >= m_pilDisable[pInfo->nImage]->GetImageCount() )
		return TRUE;

	// ｶｽﾀﾑｲﾒｰｼﾞの描画
	CRect	rc(lpDIS->rcItem);
	CPoint	pt(GetIconPoint(rc));
	if ( lpDIS->itemState & ODS_GRAYED )
		m_pilDisable[pInfo->nImage]->Draw(pDC, (int)pInfo->nIndex, pt, ILD_NORMAL);	// 淡色状態
	else
		m_pilEnable[pInfo->nImage]->Draw(pDC, (int)pInfo->nIndex, pt, ILD_NORMAL);	// 通常状態

	return TRUE;
}
