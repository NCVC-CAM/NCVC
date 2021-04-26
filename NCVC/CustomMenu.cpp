// CustomMenu.cpp: CCustomMenu ƒNƒ‰ƒX‚ÌƒCƒ“ƒvƒŠƒƒ“ƒe[ƒVƒ‡ƒ“
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

// ±²ºİ»²½Ş
extern	const	int		gg_nIconX;
extern	const	int		gg_nIconY;

// Æ°ÓÆ¯¸ (ÒÆ­°•¶š—ñ) •`‰æˆÊ’u
static	const	int		g_nMenuBack = 1;
static	const	int		g_nMenuLeft = 5;
static	const	int		g_nMenuRight = gg_nIconX - 1;

//
HICON	CCustomMenu::m_hCheckIcon = NULL;
int		CCustomMenu::m_nIconFrameX = 0;
int		CCustomMenu::m_nIconFrameY = 0;

//////////////////////////////////////////////////////////////////////
// CCustomMenu ƒNƒ‰ƒX‚Ì\’z/Á–Å

CCustomMenu::CCustomMenu()
{
#ifdef _DEBUGOLD
	printf("CCustomMenu::CCustomMenu() Start\n");
#endif

	// ”ñ¸×²±İÄ—Ìˆæ‚ÉŠÖ‚·‚éÒÄØ¯¸’l (ÒÆ­°ÊŞ°‚Ì‚‚³“™) æ“¾
	NONCLIENTMETRICS	nclm;
	::ZeroMemory(&nclm, sizeof(NONCLIENTMETRICS));
	nclm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY( ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
							sizeof(NONCLIENTMETRICS), &nclm, 0) );

	// Ì«İÄì¬
	m_fontMenu.CreateFontIndirect(&nclm.lfMenuFont);

	// Ã“I•Ï”‚Ì‰Šú‰»
	if ( !m_hCheckIcon ) {
		// Áª¯¸ÒÆ­°‚Ì±²ºİÊİÄŞÙæ“¾
		m_hCheckIcon = (HICON)::LoadImage(AfxGetResourceHandle(),
				MAKEINTRESOURCE(IDI_CHECK), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
		ASSERT(m_hCheckIcon);
		// ±²ºİÌÚ°Ñ»²½Ş (±²ºİ‚»‚Ì‚à‚Ì‚Å‚Í‚È‚­A‚Ü‚í‚è‚ÌÌÚ°Ñ‚Ì‘å‚«‚³)
		m_nIconFrameX = m_nIconFrameY = nclm.iMenuHeight + 2;
	}

	// ²Ò°¼ŞØ½Ä‚Ì‰Šú‰»
	m_ilToolBar.Create(gg_nIconX, gg_nIconY, ILC_COLORDDB|ILC_MASK, 1, 1);
}

CCustomMenu::~CCustomMenu()
{
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenu ƒNƒ‰ƒX‚Ìf’f

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
// CCustomMenu ÒİÊŞŠÖ”

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
		// ¾ÊßÚ°ÀˆÈŠO
		if ( itemID ) {
			// ÒÆ­°€–Ú ID - ²Ò°¼Ş‡‘Î‰Ï¯Ìß
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
	// ¼½ÃÑ¶×°‚Å’²®‚µ‚½²Ò°¼Şæ“¾
	HBITMAP hBitmap = AfxLoadSysColorBitmap(hInst, hRsrc);	// © MFC\SRC\BARTOOL.CPP(59)
	ASSERT(hBitmap);

	// ²Ò°¼ŞØ½Ä‚Ö“o˜^
	CBitmap* pBitmap = CBitmap::FromHandle(hBitmap);
	m_ilToolBar.Add(pBitmap, ::GetSysColor(COLOR_BTNFACE));	// © Œµ–§‚É‚ÍF‚ªˆá‚¤‚à‚ ‚é

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

	// ÒÆ­°•¶š—ñæ“¾
	CString strMenu;
	pMenu->GetMenuString(nItem, strMenu, MF_BYPOSITION);
	// ÒÆ­°•¶š—ñ‚É“o˜^
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
	HICON hIcon = NULL;					// ±²ºİÊİÄŞÙ
	BOOL bRet = FALSE;					// ±²ºİ•`‰æÌ×¸Ş
	BOOL bCheck = FALSE;				// Áª¯¸ÒÆ­°‚Ì”wŒi²Ò°¼Ş•`‰æÌ×¸Ş
	UINT itemID = lpDIS->itemID;		// ÒÆ­°€–Ú ID
	UINT itemState = lpDIS->itemState;	// •`‰æ“®ì
	CRect rcItem(lpDIS->rcItem);		// •`‰æ‹éŒ`
	// ±²ºİ•`‰æˆÊ’u
	CPoint ptIcon(GetIconPoint(rcItem));
	// ÌÚ°Ñ•`‰æ—Ìˆæ
	CRect rcFrame(rcItem.left, rcItem.top,
			rcItem.left + m_nIconFrameX, rcItem.top + m_nIconFrameX);

	// ±²ºİæ“¾
	int nRet = FindItemID(itemID);
	if ( nRet >= 0 ) {
		// ÒÆ­°€–Ú ID - ²Ò°¼Ş‡‘Î‰Ï¯Ìß‚É‚ ‚è
		hIcon = m_ilToolBar.ExtractIcon(nRet);
	}

	// ±²ºİ•`‰æó‘Ô¾¯Ä
	UINT nFlags = DST_ICON | (itemState & ODS_GRAYED ? DSS_DISABLED : DSS_NORMAL);

	// ±²ºİ•`‰æÌ×¸Ş¾¯Ä
	if ( itemState & ODS_CHECKED ) {
		// Áª¯¸ó‘Ô
		if ( !hIcon )
			hIcon = m_hCheckIcon;	// ±²ºİ‚ª‚È‚¯‚ê‚ÎÁª¯¸ÒÆ­°±²ºİ‚ğg—p
		bRet = TRUE;
		bCheck = TRUE;
	}
	else {
		// ±²ºİ‚ª‚ ‚ê‚Î•`‰æ
		if ( hIcon )
			bRet = TRUE;
	}

	// Áª¯¸ÒÆ­°‚Ì”wŒi‚ğ•`‰æ
	if ( bCheck ) {
		if ( itemState & ODS_SELECTED )
			pDC->FillSolidRect(&rcFrame, ::GetSysColor(COLOR_MENU));
		else {
			COLORREF colSysColor = (COLORREF)::GetSysColor(COLOR_3DHILIGHT);
			// ÃŞ¨»ŞØİ¸Ş‘Î‰
			for ( int y=rcFrame.top; y<rcFrame.bottom; y++ )
				for ( int x=rcFrame.left; x<rcFrame.right; x++ )
					if ( (x + y) % 2 )
						pDC->SetPixelV(x, y, colSysColor);
		}
	}

	// ±²ºİ•`‰æ
	if ( bRet )
		pDC->DrawState(ptIcon, 0, hIcon, nFlags, (CBrush *)NULL);

	return bRet;
}

void CCustomMenu::DrawItemIconFrame(CDC* pDC, LPDRAWITEMSTRUCT lpDIS)
{
	UINT itemID = lpDIS->itemID;		// ÒÆ­°€–Ú ID
	UINT itemState = lpDIS->itemState;	// •`‰æ“®ì
	CRect rcItem(lpDIS->rcItem);		// •`‰æ‹éŒ`
	COLORREF clrTopLeft, clrBottomRight;// ÌÚ°Ñ¶ã•`‰æFA‰E‰º•”•`‰æF
	// ÌÚ°Ñ•`‰æ—Ìˆæ
	CRect rcFrame(rcItem.left, rcItem.top, rcItem.left + m_nIconFrameX, rcItem.top + m_nIconFrameX);

	// ÌÚ°Ñ•`‰æF¾¯Ä
	if ( itemState & ODS_CHECKED ) {
		// Áª¯¸ó‘Ô
		clrTopLeft = ::GetSysColor(COLOR_3DSHADOW);
		clrBottomRight = ::GetSysColor(COLOR_3DHILIGHT);
	}
	else if ( itemState & ODS_GRAYED ) {
		// ’WFó‘Ô
		clrTopLeft = clrBottomRight = ::GetSysColor(COLOR_MENU);
	}
	else if ( itemState & ODS_SELECTED ) {
		// ‘I‘ğó‘Ô
		clrTopLeft = ::GetSysColor(COLOR_3DHILIGHT);
		clrBottomRight = ::GetSysColor(COLOR_3DSHADOW);
	}
	else {
		// ’Êíó‘Ô
		clrTopLeft = clrBottomRight = ::GetSysColor(COLOR_MENU);
	}

	// ÌÚ°Ñ•`‰æ
	pDC->Draw3dRect(&rcFrame, clrTopLeft, clrBottomRight);
}

void CCustomMenu::DrawItemString(CDC* pDC, LPDRAWITEMSTRUCT lpDIS, BOOL bIcon)
{
	UINT itemID = lpDIS->itemID;		// ÒÆ­°€–Ú ID
	UINT itemState = lpDIS->itemState;	// •`‰æ“®ì
	CRect rcItem(lpDIS->rcItem);		// •`‰æ‹éŒ`
	int nIndex = (int)lpDIS->itemData;	// •¶š—ñæ“¾—p
	COLORREF clrText, clrBack;			// •¶š—ñ‘OŒiFA”wŒiF
#ifdef _DEBUGOLD
	printf("DrawItemString() itemID=%d nIndex=%d", itemID, nIndex);
#endif

	// •`‰æó‘Ô¾¯Ä
	if ( itemState & ODS_GRAYED ) {
		// ’WFó‘Ô
		if( itemState & ODS_SELECTED ) {
			// ‘I‘ğó‘Ô
			clrText = ::GetSysColor(COLOR_GRAYTEXT);
			clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
			if ( clrText == clrBack )
				clrText = ::GetSysColor(COLOR_MENU);
		}
		else {
			// ’Êíó‘Ô
			clrText = ::GetSysColor(COLOR_GRAYTEXT);
			clrBack = ::GetSysColor(COLOR_MENU);
		}
	}
	else {
		// ’WFó‘Ô‚Å‚È‚¢
		if ( itemState & ODS_SELECTED ) {
			// ‘I‘ğó‘Ô
			clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
		}
		else {
			// ’Êíó‘Ô
			clrText = ::GetSysColor(COLOR_MENUTEXT);
			clrBack = ::GetSysColor(COLOR_MENU);
		}
	}

	// ”wŒi•`‰æ
	CRect rcBack(rcItem);
	if ( bIcon )
		rcBack.left += m_nIconFrameX + g_nMenuBack;
	pDC->FillSolidRect(&rcBack, clrBack);

	// ÒÆ­°•¶š—ñæ“¾
	CString strMenu;
	if( 0 <= nIndex && nIndex < m_arrayString.GetSize() )
		strMenu = m_arrayString[nIndex];

	// ÒÆ­°•¶š—ñ•`‰æ
	CRect rcMnemonic(rcItem);
	rcMnemonic.left += m_nIconFrameX + g_nMenuLeft;
	rcMnemonic.right -= g_nMenuRight;
	pDC->SetBkMode(TRANSPARENT);
	if ( (itemState & ODS_GRAYED) && !(itemState & ODS_SELECTED) ) {
		// ’WF•`‰æ (ÃŞ¨»ŞØİ¸Ş‘Î‰)
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
		// ¼®°Ä¶¯Ä·°‚ ‚è ('\t' ‚ª‘¶İ‚·‚ê‚Î‚»‚êˆÈ~‚ğ¼®°Ä¶¯Ä·°‚Æ‚·‚é)
		pDC->DrawText(strText.Left(nPos), rcText,
			DT_LEFT|DT_SINGLELINE|DT_VCENTER);
		pDC->DrawText(strText.Right(strText.GetLength() - nPos - 1), rcText,
			DT_RIGHT|DT_SINGLELINE|DT_VCENTER);
	}
	else {
		// ¼®°Ä¶¯Ä·°‚È‚µ
		pDC->DrawText(strText, rcText, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
	}
}

void CCustomMenu::DrawItemSeparator(CDC* pDC, LPDRAWITEMSTRUCT lpDIS)
{
	CRect rcItem(lpDIS->rcItem);		// •`‰æ‹éŒ`

	rcItem.top = rcItem.bottom = (rcItem.top + rcItem.bottom) >> 1;
	rcItem.left += gg_nIconX >> 1;		// ¾ÊßÚ°À‚Ì‰E/¶‚ÌŒ„ŠÔ‚Í±²ºİ‚Ì 1/2 ‚®‚ç‚¢‚ª“K“–‚©?
	rcItem.right -= gg_nIconX >> 1;
	pDC->DrawEdge(&rcItem, EDGE_ETCHED, BF_TOP);
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenu ƒƒbƒZ[ƒW ƒnƒ“ƒhƒ‰

void CCustomMenu::OnInitMenuPopup(CMenu* pMenu)
{
	ASSERT(pMenu);
	int		i, nIndex;

	HMENU hMenu = pMenu->GetSafeHmenu();
	ASSERT(hMenu);

	MENUITEMINFO mii;
	// ŠY“–ÒÆ­°‚ğÙ°Ìß
	for ( i=0; i<pMenu->GetMenuItemCount(); i++ ) {
		// ÒÆ­°•¶š—ñÍŞ¸À‚É“o˜^
		nIndex = (int)SetMenuString(pMenu, i);
		if ( nIndex >= 0 ) {
			// µ°Å°•`‰æİ’è
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
	UINT itemID = lpMIS->itemID;		// ÒÆ­°€–Ú ID
	int nIndex = (int)lpMIS->itemData;	// •¶š—ñæ“¾—p

	// ¾ÊßÚ°À‚Ì”»’è
	if ( itemID ) {
		// ’ÊíÒÆ­°
		// ÒÆ­°•¶š—ñ
		CString strMenu;
		if ( 0 <= nIndex && nIndex < m_arrayString.GetSize() )
			strMenu = m_arrayString[nIndex];

		// ‘å‚«‚³ŒvZ
		CClientDC dc(AfxGetMainWnd());
		CFont* pOldFont = dc.SelectObject(&m_fontMenu);
		CSize size = dc.GetTextExtent(strMenu);
		dc.SelectObject(pOldFont);
		lpMIS->itemWidth = m_nIconFrameX + size.cx + g_nMenuLeft + g_nMenuRight;
		lpMIS->itemHeight = m_nIconFrameY;		// ±²ºİÌÚ°Ñ‚Æ“¯‚¶‚‚³
	}
	else {
		// ¾ÊßÚ°À
		lpMIS->itemWidth = m_nIconFrameX;		// ‚Æ‚­‚ÉˆÓ–¡‚Í‚È‚¢
		lpMIS->itemHeight = m_nIconFrameY >> 1;	// ¾ÊßÚ°À‚Ì‚‚³‚Í’ÊíÒÆ­°‚Ì 1/2 ‚®‚ç‚¢‚ª“K“–‚©?
	}
}

void CCustomMenu::OnDrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(lpDIS);

	// ‰Šúˆ—
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	// ¾ÊßÚ°À‚Ì”»’è
	if ( lpDIS->itemID ) {
		// ’ÊíÒÆ­°
		// ±²ºİ•`‰æ
		BOOL bIcon;								// ±²ºİ•`‰æÌ×¸Ş
		if ( bIcon = DrawItemIcon(pDC, lpDIS) )
			DrawItemIconFrame(pDC, lpDIS);		// ±²ºİ‚ğ•`‰æ‚µ‚½‚ç±²ºİÌÚ°Ñ‚à•`‰æ

		// ÒÆ­°•¶š—ñ•`‰æ
		DrawItemString(pDC, lpDIS, bIcon);
	}
	else {
		// ¾ÊßÚ°À•`‰æ
		DrawItemSeparator(pDC, lpDIS);			
	}
}

void CCustomMenu::OnSysColorChange(int nSize, UINT nIDResource[])
{
	int		i;
	// ˆê“x²Ò°¼Ş±²ÃÑ‚ğíœ‚µ‚ÄÄ“o˜^
	int nCnt = m_ilToolBar.GetImageCount();
	for ( i=0; i<nCnt; i++ )
		m_ilToolBar.Remove(0);
	for ( i=0; i<nSize; i++ )
		LoadToolBarImage(MAKEINTRESOURCE(nIDResource[i]));
}

//////////////////////////////////////////////////////////////////////
// CCustomMenuEx ƒNƒ‰ƒX‚Ì\’z/Á–Å

CCustomMenuEx::~CCustomMenuEx()
{
	typedef	std::pair<WORD, LPCUSTMENUINFO>	PAIR;
	BOOST_FOREACH(PAIR p, m_mpImage) {
		delete	p.second;
	}
	m_mpImage.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenuEx ÒİÊŞŠÖ”

void CCustomMenuEx::SetMapImageID(WORD wKey, WORD nImage, WORD nIndex)
{
	LPCUSTMENUINFO pLookup;
	LPCUSTMENUINFO pInfo = new CUSTMENUINFO;
	pInfo->nImage = nImage;
	pInfo->nIndex = nIndex;
	if ( m_mpImage.Lookup(wKey, pLookup) )	// •ÛŒ¯
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

	// ¶½ÀÑ²Ò°¼Ş‚Ì•`‰æ
	CRect	rc(lpDIS->rcItem);
	CPoint	pt(GetIconPoint(rc));
	if ( lpDIS->itemState & ODS_GRAYED )
		m_pilDisable[pInfo->nImage]->Draw(pDC, (int)pInfo->nIndex, pt, ILD_NORMAL);	// ’WFó‘Ô
	else
		m_pilEnable[pInfo->nImage]->Draw(pDC, (int)pInfo->nIndex, pt, ILD_NORMAL);	// ’Êíó‘Ô

	return TRUE;
}
