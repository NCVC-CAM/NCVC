// CustomMenu.cpp: CCustomMenu �N���X�̃C���v�������e�[�V����
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

// ���ݻ���
extern	const	int		gg_nIconX;
extern	const	int		gg_nIconY;

// ư�Ư� (�ƭ�������) �`��ʒu
static	const	int		g_nMenuBack = 1;
static	const	int		g_nMenuLeft = 5;
static	const	int		g_nMenuRight = gg_nIconX - 1;

//
HICON	CCustomMenu::m_hCheckIcon = NULL;
int		CCustomMenu::m_nIconFrameX = 0;
int		CCustomMenu::m_nIconFrameY = 0;

//////////////////////////////////////////////////////////////////////
// CCustomMenu �N���X�̍\�z/����

CCustomMenu::CCustomMenu()
{
#ifdef _DEBUGOLD
	printf("CCustomMenu::CCustomMenu() Start\n");
#endif

	// ��ײ��ė̈�Ɋւ�����د��l (�ƭ��ް�̍�����) �擾
	NONCLIENTMETRICS	nclm;
	::ZeroMemory(&nclm, sizeof(NONCLIENTMETRICS));
	nclm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY( ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
							sizeof(NONCLIENTMETRICS), &nclm, 0) );

	// ̫�č쐬
	m_fontMenu.CreateFontIndirect(&nclm.lfMenuFont);

	// �ÓI�ϐ��̏�����
	if ( !m_hCheckIcon ) {
		// �����ƭ��̱�������َ擾
		m_hCheckIcon = (HICON)::LoadImage(AfxGetResourceHandle(),
				MAKEINTRESOURCE(IDI_CHECK), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
		ASSERT(m_hCheckIcon);
		// �����ڰѻ��� (���݂��̂��̂ł͂Ȃ��A�܂����ڰт̑傫��)
		m_nIconFrameX = m_nIconFrameY = nclm.iMenuHeight + 2;
	}

	// �Ұ��ؽĂ̏�����
	m_ilToolBar.Create(gg_nIconX, gg_nIconY, ILC_COLORDDB|ILC_MASK, 1, 1);
}

CCustomMenu::~CCustomMenu()
{
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenu �N���X�̐f�f

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
// CCustomMenu ���ފ֐�

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
		// ���ڰ��ȊO
		if ( itemID ) {
			// �ƭ����� ID - �Ұ�ޏ��Ή�ϯ��
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
	// ���Ѷװ�Œ��������Ұ�ގ擾
	HBITMAP hBitmap = AfxLoadSysColorBitmap(hInst, hRsrc);	// �� MFC\SRC\BARTOOL.CPP(59)
	ASSERT(hBitmap);

	// �Ұ��ؽĂ֓o�^
	CBitmap* pBitmap = CBitmap::FromHandle(hBitmap);
	m_ilToolBar.Add(pBitmap, ::GetSysColor(COLOR_BTNFACE));	// �� �����ɂ͐F���Ⴄ��������

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

	// �ƭ�������擾
	CString strMenu;
	pMenu->GetMenuString(nItem, strMenu, MF_BYPOSITION);
	// �ƭ�������ɓo�^
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
	HICON hIcon = NULL;					// ���������
	BOOL bRet = FALSE;					// ���ݕ`���׸�
	BOOL bCheck = FALSE;				// �����ƭ��̔w�i�Ұ�ޕ`���׸�
	UINT itemID = lpDIS->itemID;		// �ƭ����� ID
	UINT itemState = lpDIS->itemState;	// �`�擮��
	CRect rcItem(lpDIS->rcItem);		// �`���`
	// ���ݕ`��ʒu
	CPoint ptIcon(GetIconPoint(rcItem));
	// �ڰѕ`��̈�
	CRect rcFrame(rcItem.left, rcItem.top,
			rcItem.left + m_nIconFrameX, rcItem.top + m_nIconFrameX);

	// ���ݎ擾
	int nRet = FindItemID(itemID);
	if ( nRet >= 0 ) {
		// �ƭ����� ID - �Ұ�ޏ��Ή�ϯ�߂ɂ���
		hIcon = m_ilToolBar.ExtractIcon(nRet);
	}

	// ���ݕ`���Ծ��
	UINT nFlags = DST_ICON | (itemState & ODS_GRAYED ? DSS_DISABLED : DSS_NORMAL);

	// ���ݕ`���׸޾��
	if ( itemState & ODS_CHECKED ) {
		// �������
		if ( !hIcon )
			hIcon = m_hCheckIcon;	// ���݂��Ȃ���������ƭ����݂��g�p
		bRet = TRUE;
		bCheck = TRUE;
	}
	else {
		// ���݂�����Ε`��
		if ( hIcon )
			bRet = TRUE;
	}

	// �����ƭ��̔w�i��`��
	if ( bCheck ) {
		if ( itemState & ODS_SELECTED )
			pDC->FillSolidRect(&rcFrame, ::GetSysColor(COLOR_MENU));
		else {
			COLORREF colSysColor = (COLORREF)::GetSysColor(COLOR_3DHILIGHT);
			// �ި���ݸޑΉ�
			for ( int y=rcFrame.top; y<rcFrame.bottom; y++ )
				for ( int x=rcFrame.left; x<rcFrame.right; x++ )
					if ( (x + y) % 2 )
						pDC->SetPixelV(x, y, colSysColor);
		}
	}

	// ���ݕ`��
	if ( bRet )
		pDC->DrawState(ptIcon, 0, hIcon, nFlags, (CBrush *)NULL);

	return bRet;
}

void CCustomMenu::DrawItemIconFrame(CDC* pDC, LPDRAWITEMSTRUCT lpDIS)
{
	UINT itemID = lpDIS->itemID;		// �ƭ����� ID
	UINT itemState = lpDIS->itemState;	// �`�擮��
	CRect rcItem(lpDIS->rcItem);		// �`���`
	COLORREF clrTopLeft, clrBottomRight;// �ڰэ���`��F�A�E�����`��F
	// �ڰѕ`��̈�
	CRect rcFrame(rcItem.left, rcItem.top, rcItem.left + m_nIconFrameX, rcItem.top + m_nIconFrameX);

	// �ڰѕ`��F���
	if ( itemState & ODS_CHECKED ) {
		// �������
		clrTopLeft = ::GetSysColor(COLOR_3DSHADOW);
		clrBottomRight = ::GetSysColor(COLOR_3DHILIGHT);
	}
	else if ( itemState & ODS_GRAYED ) {
		// �W�F���
		clrTopLeft = clrBottomRight = ::GetSysColor(COLOR_MENU);
	}
	else if ( itemState & ODS_SELECTED ) {
		// �I�����
		clrTopLeft = ::GetSysColor(COLOR_3DHILIGHT);
		clrBottomRight = ::GetSysColor(COLOR_3DSHADOW);
	}
	else {
		// �ʏ���
		clrTopLeft = clrBottomRight = ::GetSysColor(COLOR_MENU);
	}

	// �ڰѕ`��
	pDC->Draw3dRect(&rcFrame, clrTopLeft, clrBottomRight);
}

void CCustomMenu::DrawItemString(CDC* pDC, LPDRAWITEMSTRUCT lpDIS, BOOL bIcon)
{
	UINT itemID = lpDIS->itemID;		// �ƭ����� ID
	UINT itemState = lpDIS->itemState;	// �`�擮��
	CRect rcItem(lpDIS->rcItem);		// �`���`
	int nIndex = (int)lpDIS->itemData;	// ������擾�p
	COLORREF clrText, clrBack;			// ������O�i�F�A�w�i�F
#ifdef _DEBUGOLD
	printf("DrawItemString() itemID=%d nIndex=%d", itemID, nIndex);
#endif

	// �`���Ծ��
	if ( itemState & ODS_GRAYED ) {
		// �W�F���
		if( itemState & ODS_SELECTED ) {
			// �I�����
			clrText = ::GetSysColor(COLOR_GRAYTEXT);
			clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
			if ( clrText == clrBack )
				clrText = ::GetSysColor(COLOR_MENU);
		}
		else {
			// �ʏ���
			clrText = ::GetSysColor(COLOR_GRAYTEXT);
			clrBack = ::GetSysColor(COLOR_MENU);
		}
	}
	else {
		// �W�F��ԂłȂ�
		if ( itemState & ODS_SELECTED ) {
			// �I�����
			clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
		}
		else {
			// �ʏ���
			clrText = ::GetSysColor(COLOR_MENUTEXT);
			clrBack = ::GetSysColor(COLOR_MENU);
		}
	}

	// �w�i�`��
	CRect rcBack(rcItem);
	if ( bIcon )
		rcBack.left += m_nIconFrameX + g_nMenuBack;
	pDC->FillSolidRect(&rcBack, clrBack);

	// �ƭ�������擾
	CString strMenu;
	if( 0 <= nIndex && nIndex < m_arrayString.GetSize() )
		strMenu = m_arrayString[nIndex];

	// �ƭ�������`��
	CRect rcMnemonic(rcItem);
	rcMnemonic.left += m_nIconFrameX + g_nMenuLeft;
	rcMnemonic.right -= g_nMenuRight;
	pDC->SetBkMode(TRANSPARENT);
	if ( (itemState & ODS_GRAYED) && !(itemState & ODS_SELECTED) ) {
		// �W�F�`�� (�ި���ݸޑΉ�)
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
		// ���Ķ�ķ����� ('\t' �����݂���΂���ȍ~�𼮰Ķ�ķ��Ƃ���)
		pDC->DrawText(strText.Left(nPos), rcText,
			DT_LEFT|DT_SINGLELINE|DT_VCENTER);
		pDC->DrawText(strText.Right(strText.GetLength() - nPos - 1), rcText,
			DT_RIGHT|DT_SINGLELINE|DT_VCENTER);
	}
	else {
		// ���Ķ�ķ��Ȃ�
		pDC->DrawText(strText, rcText, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
	}
}

void CCustomMenu::DrawItemSeparator(CDC* pDC, LPDRAWITEMSTRUCT lpDIS)
{
	CRect rcItem(lpDIS->rcItem);		// �`���`

	rcItem.top = rcItem.bottom = (rcItem.top + rcItem.bottom) >> 1;
	rcItem.left += gg_nIconX >> 1;		// ���ڰ��̉E/���̌��Ԃͱ��݂� 1/2 ���炢���K����?
	rcItem.right -= gg_nIconX >> 1;
	pDC->DrawEdge(&rcItem, EDGE_ETCHED, BF_TOP);
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenu ���b�Z�[�W �n���h��

void CCustomMenu::OnInitMenuPopup(CMenu* pMenu)
{
	ASSERT(pMenu);
	int		i, nIndex;

	HMENU hMenu = pMenu->GetSafeHmenu();
	ASSERT(hMenu);

	MENUITEMINFO mii;
	// �Y���ƭ���ٰ��
	for ( i=0; i<pMenu->GetMenuItemCount(); i++ ) {
		// �ƭ��������޸��ɓo�^
		nIndex = (int)SetMenuString(pMenu, i);
		if ( nIndex >= 0 ) {
			// ��Ű�`��ݒ�
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
	UINT itemID = lpMIS->itemID;		// �ƭ����� ID
	int nIndex = (int)lpMIS->itemData;	// ������擾�p

	// ���ڰ��̔���
	if ( itemID ) {
		// �ʏ��ƭ�
		// �ƭ�������
		CString strMenu;
		if ( 0 <= nIndex && nIndex < m_arrayString.GetSize() )
			strMenu = m_arrayString[nIndex];

		// �傫���v�Z
		CClientDC dc(AfxGetMainWnd());
		CFont* pOldFont = dc.SelectObject(&m_fontMenu);
		CSize size = dc.GetTextExtent(strMenu);
		dc.SelectObject(pOldFont);
		lpMIS->itemWidth = m_nIconFrameX + size.cx + g_nMenuLeft + g_nMenuRight;
		lpMIS->itemHeight = m_nIconFrameY;		// �����ڰтƓ�������
	}
	else {
		// ���ڰ�
		lpMIS->itemWidth = m_nIconFrameX;		// �Ƃ��ɈӖ��͂Ȃ�
		lpMIS->itemHeight = m_nIconFrameY >> 1;	// ���ڰ��̍����͒ʏ��ƭ��� 1/2 ���炢���K����?
	}
}

void CCustomMenu::OnDrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(lpDIS);

	// ��������
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	// ���ڰ��̔���
	if ( lpDIS->itemID ) {
		// �ʏ��ƭ�
		// ���ݕ`��
		BOOL bIcon;								// ���ݕ`���׸�
		if ( bIcon = DrawItemIcon(pDC, lpDIS) )
			DrawItemIconFrame(pDC, lpDIS);		// ���݂�`�悵���籲���ڰт��`��

		// �ƭ�������`��
		DrawItemString(pDC, lpDIS, bIcon);
	}
	else {
		// ���ڰ��`��
		DrawItemSeparator(pDC, lpDIS);			
	}
}

void CCustomMenu::OnSysColorChange(int nSize, UINT nIDResource[])
{
	int		i;
	// ��x�Ұ�ޱ��т��폜���čēo�^
	int nCnt = m_ilToolBar.GetImageCount();
	for ( i=0; i<nCnt; i++ )
		m_ilToolBar.Remove(0);
	for ( i=0; i<nSize; i++ )
		LoadToolBarImage(MAKEINTRESOURCE(nIDResource[i]));
}

//////////////////////////////////////////////////////////////////////
// CCustomMenuEx �N���X�̍\�z/����

CCustomMenuEx::~CCustomMenuEx()
{
	typedef	std::pair<WORD, LPCUSTMENUINFO>	PAIR;
	BOOST_FOREACH(PAIR p, m_mpImage) {
		delete	p.second;
	}
	m_mpImage.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
// CCustomMenuEx ���ފ֐�

void CCustomMenuEx::SetMapImageID(WORD wKey, WORD nImage, WORD nIndex)
{
	LPCUSTMENUINFO pLookup;
	LPCUSTMENUINFO pInfo = new CUSTMENUINFO;
	pInfo->nImage = nImage;
	pInfo->nIndex = nIndex;
	if ( m_mpImage.Lookup(wKey, pLookup) )	// �ی�
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

	// ���ѲҰ�ނ̕`��
	CRect	rc(lpDIS->rcItem);
	CPoint	pt(GetIconPoint(rc));
	if ( lpDIS->itemState & ODS_GRAYED )
		m_pilDisable[pInfo->nImage]->Draw(pDC, (int)pInfo->nIndex, pt, ILD_NORMAL);	// �W�F���
	else
		m_pilEnable[pInfo->nImage]->Draw(pDC, (int)pInfo->nIndex, pt, ILD_NORMAL);	// �ʏ���

	return TRUE;
}
