// CustomToolBar.cpp: CCustomToolBar �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "CustomToolBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CCustomToolBar, CToolBar)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TBN_QUERYINSERT, &CCustomToolBar::OnNotifyQueryInsertOrDelete)
	ON_NOTIFY_REFLECT(TBN_QUERYDELETE, &CCustomToolBar::OnNotifyQueryInsertOrDelete)
	ON_NOTIFY_REFLECT(TBN_GETBUTTONINFO, &CCustomToolBar::OnNotifyGetButtonInfo)
	ON_NOTIFY_REFLECT(TBN_RESET, &CCustomToolBar::OnNotifyReset)
	ON_NOTIFY_REFLECT(TBN_TOOLBARCHANGE, &CCustomToolBar::OnNotifyToolBarChange)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CCustomToolBar �N���X�̍\�z/����

CCustomToolBar::CCustomToolBar()
{
	m_lpszResourceName = NULL;
	m_lpCustTbButtons = NULL;
}

CCustomToolBar::~CCustomToolBar()
{
	for ( int i=0; i<m_arButton.GetSize(); i++ )
		delete	m_arButton[i];
}

/////////////////////////////////////////////////////////////////////////////
// CCustToolBar �N���X�̐f�f

#ifdef _DEBUG
void CCustomToolBar::AssertValid() const
{
	CToolBar::AssertValid();
}

void CCustomToolBar::Dump(CDumpContext& dc) const
{
	CToolBar::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCustomToolBar ���ފ֐�

BOOL CCustomToolBar::CreateExEx
	(CWnd* pParentWnd, LPCTSTR lpszTitle, UINT nID,
			DWORD dwCtrlStyle, DWORD dwStyle, CRect rcBorders)
{
	if ( !CreateEx(pParentWnd, 0, dwStyle, rcBorders, nID) )
		return FALSE;

	// CToolBar::CreateEx()�̑�Q�����Ŏw�肷��ƃS�~���c��(XPStyle.manifest�Ή�)
	ModifyStyle(0, dwCtrlStyle);
	SetWindowText(lpszTitle);
	EnableDocking(CBRS_ALIGN_ANY);

	return TRUE;
}

BOOL CCustomToolBar::LoadToolBarEx
	(LPCTSTR lpszResourceName, LPCUSTTBBUTTON lpCustTbButtons, BOOL bRestore)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
	int		i, nIndex;
	CString strText;
	CUSTTBBUTTON	ctb;
	LPCUSTTBINFO	lpInfo;

	// °��ްؿ���Ǎ���
	ASSERT(lpszResourceName);
	ASSERT(lpCustTbButtons);

	// ������
	m_lpszResourceName = lpszResourceName;
	m_lpCustTbButtons = lpCustTbButtons;
	// ���ݍ폜
	RemoveAllButtons();

	// °��ް�Ǎ���
	if ( !LoadBitmap(lpszResourceName) )
		return FALSE;
	// �ƭ�����(�����)ID�擾
	CMap<WORD, WORD&, int, int&> mapImage;
	if ( !LoadToolBarItem(lpszResourceName, mapImage) )
		return FALSE;

	try {
		// ���ݏ��ǉ� & �}��
		for( i=0; ; i++) {
			ctb = lpCustTbButtons[i];
			// ��`�I������
			if ( ctb.idCommand==0 && ctb.fsState==0 && ctb.fsStyle==0 && ctb.bDisplay==FALSE )
				break;
			// ���ݏ��쐬
			lpInfo = new CUSTTBINFO;
			lpInfo->tb.idCommand	= ctb.idCommand;
			lpInfo->tb.fsState		= ctb.fsState;
			lpInfo->tb.fsStyle		= ctb.fsStyle;
			lpInfo->tb.dwData		= NULL;
			lpInfo->tb.iString		= NULL;
			if ( ctb.idCommand > 0 ) {		// ���ڰ��ȊO
				// �ʏ�����
				if ( mapImage.Lookup((WORD &)ctb.idCommand, nIndex) )
					lpInfo->tb.iBitmap = nIndex;
				else
					lpInfo->tb.iBitmap = NULL;
				// ���ݕ�������ǉ�
				if ( strText.LoadString(ctb.idCommand) ) {
					nIndex = strText.Find(gg_szReturn);	// °����ߕ���������ݒ�
					if ( nIndex >= 0 )
						lpInfo->strInfo = strText.Mid(nIndex+1);
				}
			}
			else
				lpInfo->tb.iBitmap = NULL;
			// ���ݏ��ǉ�
			m_arButton.Add(lpInfo);
			// ���ݑ}��
			if ( ctb.bDisplay ) {
				if ( !GetToolBarCtrl().AddButtons(1, &(lpInfo->tb)) )
					return FALSE;
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	// °��ް��ԕ���
	if ( bRestore )
		RestoreState();

	return TRUE;
}

BOOL CCustomToolBar::SetCustomButtons(LPCTSTR lpszResourceName,
		CImageList* pilEnable, CImageList* pilDisable, CUSTTBINFO tbInfo[],
		BOOL bRestore)
{
	ASSERT( pilEnable->GetImageCount() == pilDisable->GetImageCount() );

	int		i, nCnt;

	// ������
	m_lpszResourceName = lpszResourceName;
	m_lpCustTbButtons = NULL;
	// ���ݍ폜
	RemoveAllButtons();

	nCnt = pilEnable->GetImageCount();
	if ( nCnt <= 0 ) {
		GetToolBarCtrl().AutoSize();
		GetParentFrame()->RecalcLayout();
		return TRUE;
	}

	// �Ұ��ؽĂ̾��
	GetToolBarCtrl().SetImageList(pilEnable);
	GetToolBarCtrl().SetDisabledImageList(pilDisable);

	LPCUSTTBINFO	lpInfo;
	LPTBBUTTON		lptb = NULL;
	try {
		// ���ݏ��쐬
		lptb = new TBBUTTON[nCnt];
		for ( i=0; i<nCnt; i++ ) {
			lptb[i].iBitmap = i;
			lptb[i].idCommand = tbInfo[i].tb.idCommand;
			lptb[i].fsState = NULL/*TBSTATE_ENABLED*/;
			lptb[i].fsStyle = TBSTYLE_BUTTON;
			lptb[i].dwData  = NULL;
			lptb[i].iString = NULL;
			lpInfo = new CUSTTBINFO;
			memcpy(&(lpInfo->tb), &(lptb[i]), sizeof(TBBUTTON));
			lpInfo->strInfo = tbInfo[i].strInfo;
			m_arButton.Add(lpInfo);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( lptb )
			delete[]	lptb;
		return FALSE;
	}

#ifdef _DEBUG
	printf("CCustomToolBar::SetCustomButtons() NewButtonCount()=%d\n", nCnt);
	BOOL bResult = GetToolBarCtrl().AddButtons(nCnt, lptb);
#else
	GetToolBarCtrl().AddButtons(nCnt, lptb);
#endif

	// °��ް��ԕ���
	if ( bRestore )
		RestoreState();

	GetToolBarCtrl().AutoSize();

	if ( lptb )
		delete[]	lptb;

	return TRUE;
}

BOOL CCustomToolBar::LoadToolBarItem
	(LPCTSTR lpszResourceName, CMap<WORD, WORD&, int, int&>& mapImage)
{
	// °��ް����C�ƭ����� - �Ұ�ޏ� �̑Ή��擾
	HINSTANCE hInst = AfxFindResourceHandle(lpszResourceName, RT_TOOLBAR);
	HRSRC hRsrc = ::FindResource(hInst, lpszResourceName, RT_TOOLBAR);
	if ( !hRsrc )
		return FALSE;

	HGLOBAL hGlobal = ::LoadResource(hInst, hRsrc);
	if ( !hGlobal )
		return FALSE;

	CToolBarItem* pData = (CToolBarItem *)::LockResource(hGlobal);
	if ( !pData )
		return FALSE;
	ASSERT(pData->wVersion==1);

	BOOL	bResult = TRUE;
	try {
		WORD	itemID;
		for ( int i=0, iImage=0; i<pData->wItemCount; i++ ) {
			itemID = pData->items()[i];
			if ( itemID > 0 ) {	// ���ڰ��ȊO
				// �ƭ�ID - �Ұ�ޏ��̑Ή�ϯ��
				mapImage.SetAt(itemID, iImage);
				iImage++;
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}

	::UnlockResource(hGlobal);
	::FreeResource(hGlobal);

	return bResult;
}

void CCustomToolBar::RestoreState(void)
{
	// °��ް��ԕ���
	// ���߂Ă��ǂ�����ڼ޽�ؓ��e�Ŕ���
	CString	strKey(GetSubKey()), strValue(GetValueName());
	CRegKey	reg;
	if ( reg.Open(HKEY_CURRENT_USER, strKey) == ERROR_SUCCESS ) {
		DWORD dwType = REG_BINARY;
		DWORD dwCbData;
		// RestoreState API �ɂ����݂�1�̎��C۰�ނł��Ȃ��޸ނ����邽��
		if ( ::RegQueryValueEx(reg, strValue, NULL, &dwType, NULL, &dwCbData)
					== ERROR_SUCCESS && dwCbData > sizeof(DWORD) ) {
			// °��ް��ԕ���
			GetToolBarCtrl().RestoreState(HKEY_CURRENT_USER, strKey, strValue);
			// ��а���ݍ폜
			GetToolBarCtrl().DeleteButton(0);
		}
	}
}

void CCustomToolBar::SaveState(void)
{
	// °��ް��ԕۑ�
	// ��а���ݒǉ�
	TBBUTTON tbSeparator = {NULL, 0, TBSTATE_HIDDEN, TBSTYLE_SEP, 0, NULL};
	VERIFY( GetToolBarCtrl().InsertButton(0, &tbSeparator) );

	// °��ް��ԕۑ�
	GetToolBarCtrl().SaveState(HKEY_CURRENT_USER, GetSubKey(), GetValueName());

	// ��а���ݍ폜
	GetToolBarCtrl().DeleteButton(0);
}

void CCustomToolBar::RemoveAllButtons(void)
{
	int	i, nBtnCnt = GetToolBarCtrl().GetButtonCount();
	for ( i=0; i<nBtnCnt; i++ )
		GetToolBarCtrl().DeleteButton(0);
	for ( i=0; i<m_arButton.GetSize(); i++ )
		delete	m_arButton[i];
	m_arButton.RemoveAll();
}

CString	CCustomToolBar::GetSubKey(void)
{
	extern	LPCTSTR	gg_szRegKey;
	CString	strKey(GetSubTreeRegKey(IDS_REGKEY_WINDOW, IDS_REGKEY_WINDOW_TOOLBAR));	// StdAfx.h
	return gg_szRegKey+strKey;
}

CString CCustomToolBar::GetValueName(void)
{
	CString strValue;
	strValue.Format("%u", m_lpszResourceName);	// unsigned short(??)
	return strValue;
}

/////////////////////////////////////////////////////////////////////////////
// CCustomToolBar ���b�Z�[�W �n���h��

void CCustomToolBar::OnDestroy()
{
	// °��ް��ԕۑ�
	SaveState();
	CToolBar::OnDestroy();
}

void CCustomToolBar::OnNotifyQueryInsertOrDelete(NMHDR* pTbNotify, LRESULT* pResult)
{
	// °��ް / ���ݒǉ� / �폜
	ASSERT(pResult);
	*pResult = TRUE;
}

void CCustomToolBar::OnNotifyGetButtonInfo(NMHDR* pTbNotify, LRESULT* pResult)
{
	// °��ް / ���ϲ�� / ���ݏ��
	ASSERT(pTbNotify);
	ASSERT(pResult);

	LPNMTOOLBAR lpnmtb = reinterpret_cast<LPNMTOOLBAR>(pTbNotify);
	if ( lpnmtb->iItem < m_arButton.GetSize() ) {
		LPCUSTTBINFO lpInfo = m_arButton[lpnmtb->iItem];
		memcpy(&(lpnmtb->tbButton), &(lpInfo->tb), sizeof(TBBUTTON));
		if ( lpnmtb->pszText ) {
			lstrcpy(lpnmtb->pszText, lpInfo->strInfo);
			lpnmtb->cchText = lstrlen(lpnmtb->pszText);
		}
		*pResult = TRUE;
	}
	else	
		*pResult = FALSE;

}

void CCustomToolBar::OnNotifyReset(NMHDR* pNmhdr, LRESULT* pResult)
{
	if ( m_lpCustTbButtons ) {
		// °��ް�̍�۰��
		LoadToolBarEx(m_lpszResourceName, m_lpCustTbButtons, FALSE);
		// ڲ��čČv�Z
		GetParentFrame()->RecalcLayout();
	}
}

void CCustomToolBar::OnNotifyToolBarChange(NMHDR* pNmhdr, LRESULT* pResult)
{
	// °��ް���ϲ�ފ���
	// ڲ��čČv�Z
	GetToolBarCtrl().AutoSize();
	GetParentFrame()->RecalcLayout();
}
