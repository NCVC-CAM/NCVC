// CustomToolBar.cpp: CCustomToolBar クラスのインプリメンテーション
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
// CCustomToolBar クラスの構築/消滅

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
// CCustToolBar クラスの診断

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
// CCustomToolBar ﾒﾝﾊﾞ関数

BOOL CCustomToolBar::CreateExEx
	(CWnd* pParentWnd, LPCTSTR lpszTitle, UINT nID,
			DWORD dwCtrlStyle, DWORD dwStyle, CRect rcBorders)
{
	if ( !CreateEx(pParentWnd, 0, dwStyle, rcBorders, nID) )
		return FALSE;

	// CToolBar::CreateEx()の第２引数で指定するとゴミが残る(XPStyle.manifest対応)
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

	// ﾂｰﾙﾊﾞｰﾘｿｰｽ読込み
	ASSERT(lpszResourceName);
	ASSERT(lpCustTbButtons);

	// 初期化
	m_lpszResourceName = lpszResourceName;
	m_lpCustTbButtons = lpCustTbButtons;
	// ﾎﾞﾀﾝ削除
	RemoveAllButtons();

	// ﾂｰﾙﾊﾞｰ読込み
	if ( !LoadBitmap(lpszResourceName) )
		return FALSE;
	// ﾒﾆｭｰ項目(ｺﾏﾝﾄﾞ)ID取得
	CMap<WORD, WORD&, int, int&> mapImage;
	if ( !LoadToolBarItem(lpszResourceName, mapImage) )
		return FALSE;

	try {
		// ﾎﾞﾀﾝ情報追加 & 挿入
		for( i=0; ; i++) {
			ctb = lpCustTbButtons[i];
			// 定義終了判定
			if ( ctb.idCommand==0 && ctb.fsState==0 && ctb.fsStyle==0 && ctb.bDisplay==FALSE )
				break;
			// ﾎﾞﾀﾝ情報作成
			lpInfo = new CUSTTBINFO;
			lpInfo->tb.idCommand	= ctb.idCommand;
			lpInfo->tb.fsState		= ctb.fsState;
			lpInfo->tb.fsStyle		= ctb.fsStyle;
			lpInfo->tb.dwData		= NULL;
			lpInfo->tb.iString		= NULL;
			if ( ctb.idCommand > 0 ) {		// ｾﾊﾟﾚｰﾀ以外
				// 通常ﾎﾞﾀﾝ
				if ( mapImage.Lookup((WORD &)ctb.idCommand, nIndex) )
					lpInfo->tb.iBitmap = nIndex;
				else
					lpInfo->tb.iBitmap = NULL;
				// ﾎﾞﾀﾝ文字列情報追加
				if ( strText.LoadString(ctb.idCommand) ) {
					nIndex = strText.Find(gg_szReturn);	// ﾂｰﾙﾁｯﾌﾟ部分だけを設定
					if ( nIndex >= 0 )
						lpInfo->strInfo = strText.Mid(nIndex+1);
				}
			}
			else
				lpInfo->tb.iBitmap = NULL;
			// ﾎﾞﾀﾝ情報追加
			m_arButton.Add(lpInfo);
			// ﾎﾞﾀﾝ挿入
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

	// ﾂｰﾙﾊﾞｰ状態復旧
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

	// 初期化
	m_lpszResourceName = lpszResourceName;
	m_lpCustTbButtons = NULL;
	// ﾎﾞﾀﾝ削除
	RemoveAllButtons();

	nCnt = pilEnable->GetImageCount();
	if ( nCnt <= 0 ) {
		GetToolBarCtrl().AutoSize();
		GetParentFrame()->RecalcLayout();
		return TRUE;
	}

	// ｲﾒｰｼﾞﾘｽﾄのｾｯﾄ
	GetToolBarCtrl().SetImageList(pilEnable);
	GetToolBarCtrl().SetDisabledImageList(pilDisable);

	LPCUSTTBINFO	lpInfo;
	LPTBBUTTON		lptb = NULL;
	try {
		// ﾎﾞﾀﾝ情報作成
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

	// ﾂｰﾙﾊﾞｰ状態復旧
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
	// ﾂｰﾙﾊﾞｰから，ﾒﾆｭｰ項目 - ｲﾒｰｼﾞ順 の対応取得
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
			if ( itemID > 0 ) {	// ｾﾊﾟﾚｰﾀ以外
				// ﾒﾆｭｰID - ｲﾒｰｼﾞ順の対応ﾏｯﾌﾟ
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
	// ﾂｰﾙﾊﾞｰ状態復旧
	// 初めてかどうかをﾚｼﾞｽﾄﾘ内容で判定
	CString	strKey(GetSubKey()), strValue(GetValueName());
	CRegKey	reg;
	if ( reg.Open(HKEY_CURRENT_USER, strKey) == ERROR_SUCCESS ) {
		DWORD dwType = REG_BINARY;
		DWORD dwCbData;
		// RestoreState API にはﾎﾞﾀﾝが1個の時，ﾛｰﾄﾞできないﾊﾞｸﾞがあるため
		if ( ::RegQueryValueEx(reg, strValue, NULL, &dwType, NULL, &dwCbData)
					== ERROR_SUCCESS && dwCbData > sizeof(DWORD) ) {
			// ﾂｰﾙﾊﾞｰ状態復旧
			GetToolBarCtrl().RestoreState(HKEY_CURRENT_USER, strKey, strValue);
			// ﾀﾞﾐｰﾎﾞﾀﾝ削除
			GetToolBarCtrl().DeleteButton(0);
		}
	}
}

void CCustomToolBar::SaveState(void)
{
	// ﾂｰﾙﾊﾞｰ状態保存
	// ﾀﾞﾐｰﾎﾞﾀﾝ追加
	TBBUTTON tbSeparator = {NULL, 0, TBSTATE_HIDDEN, TBSTYLE_SEP, 0, NULL};
	VERIFY( GetToolBarCtrl().InsertButton(0, &tbSeparator) );

	// ﾂｰﾙﾊﾞｰ状態保存
	GetToolBarCtrl().SaveState(HKEY_CURRENT_USER, GetSubKey(), GetValueName());

	// ﾀﾞﾐｰﾎﾞﾀﾝ削除
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
// CCustomToolBar メッセージ ハンドラ

void CCustomToolBar::OnDestroy()
{
	// ﾂｰﾙﾊﾞｰ状態保存
	SaveState();
	CToolBar::OnDestroy();
}

void CCustomToolBar::OnNotifyQueryInsertOrDelete(NMHDR* pTbNotify, LRESULT* pResult)
{
	// ﾂｰﾙﾊﾞｰ / ﾎﾞﾀﾝ追加 / 削除
	ASSERT(pResult);
	*pResult = TRUE;
}

void CCustomToolBar::OnNotifyGetButtonInfo(NMHDR* pTbNotify, LRESULT* pResult)
{
	// ﾂｰﾙﾊﾞｰ / ｶｽﾀﾏｲｽﾞ / ﾎﾞﾀﾝ情報
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
		// ﾂｰﾙﾊﾞｰの再ﾛｰﾄﾞ
		LoadToolBarEx(m_lpszResourceName, m_lpCustTbButtons, FALSE);
		// ﾚｲｱｳﾄ再計算
		GetParentFrame()->RecalcLayout();
	}
}

void CCustomToolBar::OnNotifyToolBarChange(NMHDR* pNmhdr, LRESULT* pResult)
{
	// ﾂｰﾙﾊﾞｰｶｽﾀﾏｲｽﾞ完了
	// ﾚｲｱｳﾄ再計算
	GetToolBarCtrl().AutoSize();
	GetParentFrame()->RecalcLayout();
}
