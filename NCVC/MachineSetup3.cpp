// MachineSetup3.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "MachineSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

struct TOOLHEADER
{
	LPCTSTR		lpszName;
	int			nFormat;
};
static	TOOLHEADER	g_ToolHeader[] = {
	{"Ｔ", LVCFMT_CENTER},
	{"名　称", LVCFMT_LEFT},
	{"径補正値 ", LVCFMT_RIGHT},
	{"長補正値 ", LVCFMT_RIGHT},
	{"工具ﾀｲﾌﾟ ", LVCFMT_RIGHT}
};
static	LPCTSTR		g_szMillType[] = {
	"ｽｸｳｪｱ", "ﾎﾞｰﾙ", "面取り"
};

BEGIN_MESSAGE_MAP(CMachineSetup3, CPropertyPage)
	//{{AFX_MSG_MAP(CMachineSetup3)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MCST3_LIST, &CMachineSetup3::OnItemChangedToolList)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_MCST3_LIST, &CMachineSetup3::OnGetDispInfoToolList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MCST3_LIST, &CMachineSetup3::OnColumnClickToolList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_MCST3_LIST, &CMachineSetup3::OnKeyDownToolList)
	ON_BN_CLICKED(IDC_EXE_ADD, &CMachineSetup3::OnAdd)
	ON_BN_CLICKED(IDC_EXE_MOD, &CMachineSetup3::OnMod)
	ON_BN_CLICKED(IDC_EXE_DEL, &CMachineSetup3::OnDel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup3 プロパティ ページ

CMachineSetup3::CMachineSetup3() : CPropertyPage(CMachineSetup3::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	m_bChange = FALSE;
	m_nSortColumn = 0;

	// 現在登録済みの工具情報は，登録・削除ﾌﾗｸﾞをｸﾘｱ
	const CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();
	PLIST_FOREACH(CMCTOOLINFO* pToolInfo, &pMCopt->m_ltTool)
		pToolInfo->m_bDlgAdd = pToolInfo->m_bDlgDel = FALSE;
	END_FOREACH
	m_nType = pMCopt->m_nCorrectType;
}

void CMachineSetup3::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMachineSetup3)
	DDX_Control(pDX, IDC_MCST3_H, m_dToolH);
	DDX_Control(pDX, IDC_MCST3_D, m_dToolD);
	DDX_Control(pDX, IDC_MCST3_TOOLNO, m_nToolNo);
	DDX_Control(pDX, IDC_MCST3_LIST, m_ctToolList);
	DDX_Text(pDX, IDC_MCST3_NAME, m_strToolName);
	DDX_Radio(pDX, IDC_MCST3_TYPEA, m_nType);
	DDX_CBIndex(pDX, IDC_MCST3_MILLTYPE, m_nMillType);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup3 ﾒﾝﾊﾞ関数

void CMachineSetup3::SetDetailData(const CMCTOOLINFO* pToolInfo)
{
	if ( pToolInfo ) {
		m_nToolNo		= pToolInfo->m_nTool;
		m_strToolName	= pToolInfo->m_strName;
		m_dToolD		= pToolInfo->m_dToolD;
		m_dToolH		= pToolInfo->m_dToolH;
		m_nMillType		= pToolInfo->m_nType;
	}
	else {
		m_nToolNo		= 0;
		m_strToolName.Empty();
		m_dToolD		= 0;
		m_dToolH		= 0;
		m_nMillType		= 0;
	}
	UpdateData(FALSE);
}

BOOL CMachineSetup3::CheckData(void)
{
	UpdateData();

	if ( m_nToolNo <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_nToolNo.SetFocus();
		m_nToolNo.SetSel(0, -1);
		return FALSE;
	}
	return TRUE;
}

void CMachineSetup3::SetHeaderMark(int nNewColumn)
{
	CHeaderCtrl*	pHeader = m_ctToolList.GetHeaderCtrl();
	HDITEM			hdi;
	int				nPos = abs(m_nSortColumn) - 1;

	hdi.mask = HDI_FORMAT;

	// ﾏｰｶｰ削除
	if ( m_nSortColumn!=0 && m_nSortColumn!=nNewColumn ) {
		pHeader->GetItem(nPos, &hdi);
		hdi.fmt &= ~( HDF_SORTUP | HDF_SORTDOWN );
		pHeader->SetItem(nPos, &hdi);
	}

	// ﾏｰｶｰ挿入
	if ( nNewColumn != 0 ) {
		nPos = abs(nNewColumn) - 1;
		pHeader->GetItem(nPos, &hdi);
		hdi.fmt |= ( nNewColumn<0 ? HDF_SORTUP : HDF_SORTDOWN );
		pHeader->SetItem(nPos, &hdi);
	}

	m_nSortColumn = nNewColumn;
}

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup3 メッセージ ハンドラ

BOOL CMachineSetup3::OnInitDialog() 
{
	__super::OnInitDialog();

	int			i;
	const CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();

	// ｿｰﾄ列の取得
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCTOOLSORT));
	int nSortLayer = (int)(AfxGetApp()->GetProfileInt(strRegKey, strEntry, 1));

	// ﾍｯﾀﾞｰ定義
	for ( i=0; i<SIZEOF(g_ToolHeader); i++ ) {
		m_ctToolList.InsertColumn(i,
			g_ToolHeader[i].lpszName, g_ToolHeader[i].nFormat);
	}
	// 列幅
	CRect		rc;
	m_ctToolList.GetClientRect(rc);
	i = m_ctToolList.GetStringWidth(g_ToolHeader[2].lpszName) + 16;	// "径補正値 "
	m_ctToolList.SetColumnWidth(0, i/2);
	m_ctToolList.SetColumnWidth(2, i);
	m_ctToolList.SetColumnWidth(3, i);
	m_ctToolList.SetColumnWidth(4, i);
	m_ctToolList.SetColumnWidth(1, rc.Width() -
		m_ctToolList.GetColumnWidth(0)-
		m_ctToolList.GetColumnWidth(2)-
		m_ctToolList.GetColumnWidth(3)-
		m_ctToolList.GetColumnWidth(4)-32);
	// ﾘｽﾄｺﾝﾄﾛｰﾙへの登録
	LV_ITEM	lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	i = 0;
	PLIST_FOREACH(CMCTOOLINFO* pToolInfo, &pMCopt->m_ltTool)
		lvi.iItem  = i++;
		lvi.lParam = (LPARAM)pToolInfo;
		if ( m_ctToolList.InsertItem(&lvi) < 0 ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_ADDITEM, i);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			break;
		}
	END_FOREACH
	// １行選択
	DWORD	dwStyle = m_ctToolList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	m_ctToolList.SetExtendedStyle(dwStyle);
	// 詳細表示
	if ( pMCopt->m_ltTool.IsEmpty() )
		m_nToolNo.SetFocus();
	else {
		m_ctToolList.SortItems(CompareToolListFunc, nSortLayer);
		m_ctToolList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		// ﾍｯﾀﾞｰﾏｰｸ挿入
		SetHeaderMark(nSortLayer);		// m_nSortColumnへ代入
	}

	return FALSE;
}

BOOL CMachineSetup3::OnApply() 
{
	CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();

	// ﾀﾞｲｱﾛｸﾞで削除された工具情報を削除
	pMCopt->ReductionTools(FALSE);
	pMCopt->m_nCorrectType = m_nType;

	// ｿｰﾄ列の保存
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCTOOLSORT));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_nSortColumn);

	// 再読込ﾁｪｯｸ
	if ( m_bChange )
		static_cast<CMachineSetup *>(GetParentSheet())->m_bReload = TRUE;

	return TRUE;
}

void CMachineSetup3::OnCancel() 
{
	// ﾀﾞｲｱﾛｸﾞで登録された工具情報を削除
	AfxGetNCVCApp()->GetMachineOption()->ReductionTools(TRUE);
	__super::OnCancel();
}

void CMachineSetup3::OnItemChangedToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ( pNMListView->uNewState & LVIS_SELECTED )
		SetDetailData( (CMCTOOLINFO *)(pNMListView->lParam) );
	*pResult = 0;
}

void CMachineSetup3::OnGetDispInfoToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	if ( !(plvdi->item.mask & LVIF_TEXT) )
		return;
	CMCTOOLINFO*	pToolInfo = (CMCTOOLINFO *)(plvdi->item.lParam);
	if ( !pToolInfo )
		return;

	CString		strFmt;
	switch ( plvdi->item.iSubItem ) {
	case MCTOOL_T:		// Ｔ番号
		lstrcpy(plvdi->item.pszText, boost::lexical_cast<std::string>(pToolInfo->m_nTool).c_str());
		break;
	case MCTOOL_NAME:	// 工具名
		lstrcpy(plvdi->item.pszText, pToolInfo->m_strName);
		break;
	case MCTOOL_D:		// 径補正
		strFmt.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolD);
		lstrcpy(plvdi->item.pszText, strFmt);
		break;
	case MCTOOL_H:		// 長補正
		strFmt.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolH);
		lstrcpy(plvdi->item.pszText, strFmt);
		break;
	case MCTOOL_TYPE:	// 工具ﾀｲﾌﾟ
		if ( pToolInfo->m_nType>=0 && pToolInfo->m_nType<SIZEOF(g_szMillType) )
			lstrcpy(plvdi->item.pszText, g_szMillType[pToolInfo->m_nType]);
		break;
	}
}

void CMachineSetup3::OnColumnClickToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int	nNewColumn = pNMListView->iSubItem + 1;
	if ( abs(m_nSortColumn) == nNewColumn )
		nNewColumn = -m_nSortColumn;	// 昇順・降順の切り替え
	// 並べ替え
	m_ctToolList.SortItems(CompareToolListFunc, nNewColumn);
	// ﾍｯﾀﾞｰﾏｰｸ挿入
	SetHeaderMark(nNewColumn);

	*pResult = 0;
}

int	CALLBACK CMachineSetup3::CompareToolListFunc
	(LPARAM lParam1, LPARAM lParam2, LPARAM lSubItem)
{
	CMCTOOLINFO*	pToolInfo1 = (CMCTOOLINFO *)lParam1;
	CMCTOOLINFO*	pToolInfo2 = (CMCTOOLINFO *)lParam2;
	int		nResult = 0, nSort = (int)lSubItem;

	switch ( abs(nSort)-1 ) {
	case MCTOOL_T:		// Ｔ番号
		nResult = pToolInfo1->m_nTool - pToolInfo2->m_nTool;
		break;
	case MCTOOL_NAME:	// 工具名
		nResult = pToolInfo1->m_strName.Compare(pToolInfo2->m_strName);
		break;
	case MCTOOL_D:		// 径補正
		nResult = (int)( (pToolInfo1->m_dToolD - pToolInfo2->m_dToolD) * 1000 );
		break;
	case MCTOOL_H:		// 長補正
		nResult = (int)( (pToolInfo1->m_dToolH - pToolInfo2->m_dToolH) * 1000 );
		break;
	case MCTOOL_TYPE:	// 工具ﾀｲﾌﾟ
		nResult = pToolInfo1->m_nType - pToolInfo2->m_nType;
		break;
	}

	return nSort != 0 ? (nResult * nSort) : nResult;
}

void CMachineSetup3::OnKeyDownToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	if ( pLVKeyDown->wVKey == VK_DELETE )
		OnDel();	// 削除ﾎﾞﾀﾝｲﾍﾞﾝﾄ
	*pResult = 0;
}

void CMachineSetup3::OnAdd() 
{
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( !CheckData() )
		return;

	POSITION	pos = NULL;
	int			nIndex = -1;
	CMCTOOLINFO*	pToolInfo = NULL;
	CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();
	LV_ITEM		lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.stateMask = lvi.state = LVIS_SELECTED|LVIS_FOCUSED;

	// 工具情報の登録
	try {
		pToolInfo = new CMCTOOLINFO(m_nToolNo, m_strToolName, m_dToolD, m_dToolH, m_nMillType, TRUE);
		pos = pMCopt->m_ltTool.AddTail(pToolInfo);
		// ﾘｽﾄｺﾝﾄﾛｰﾙへの登録
		lvi.iItem  = m_ctToolList.GetItemCount();
		lvi.lParam = (LPARAM)pToolInfo;
		nIndex = m_ctToolList.InsertItem(&lvi);
		if ( nIndex < 0 ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_ADDITEM, m_ctToolList.GetItemCount()+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			delete	pToolInfo;
			pMCopt->m_ltTool.RemoveAt(pos);
			return;
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pToolInfo )
			delete	pToolInfo;
		if ( pos )
			pMCopt->m_ltTool.RemoveAt(pos);
		if ( nIndex > 0 )
			m_ctToolList.DeleteItem(nIndex);
		SetDetailData(NULL);
		return;
	}

	m_ctToolList.EnsureVisible(nIndex, FALSE);
	m_nToolNo.SetFocus();
	m_nToolNo.SetSel(0, -1);
	m_bChange = TRUE;
}

void CMachineSetup3::OnMod() 
{
	int			nIndex;
	POSITION	pos;

	if ( !(pos=m_ctToolList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctToolList.GetNextSelectedItem(pos)) < 0 )
		return;
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( !CheckData() )
		return;

	CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();

	// ﾃﾞｰﾀ置換 -> 削除ﾌﾗｸﾞを立てて新たに登録
	CMCTOOLINFO*	pToolInfo = reinterpret_cast<CMCTOOLINFO *>(m_ctToolList.GetItemData(nIndex));
	pToolInfo->m_bDlgDel = TRUE;
	pos = NULL;
	pToolInfo = NULL;
	try {
		pToolInfo = new CMCTOOLINFO(m_nToolNo, m_strToolName, m_dToolD, m_dToolH, m_nMillType, TRUE);
		pos = pMCopt->m_ltTool.AddTail(pToolInfo);
		// ﾘｽﾄｺﾝﾄﾛｰﾙの該当ﾃﾞｰﾀを置換
		if ( !m_ctToolList.SetItemData(nIndex, reinterpret_cast<DWORD_PTR>(pToolInfo)) ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_ADDITEM, nIndex+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			delete	pToolInfo;
			pMCopt->m_ltTool.RemoveAt(pos);
			return;
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pToolInfo )
			delete	pToolInfo;
		if ( pos )
			pMCopt->m_ltTool.RemoveAt(pos);
		return;
	}

	m_ctToolList.Update(nIndex);
	m_nToolNo.SetFocus();
	m_nToolNo.SetSel(0, -1);
	m_bChange = TRUE;
}

void CMachineSetup3::OnDel() 
{
	int			nIndex;
	POSITION	pos;
	if ( !(pos=m_ctToolList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctToolList.GetNextSelectedItem(pos)) < 0 )
		return;

	// ﾎﾟｲﾝﾀを削除するとｷｬﾝｾﾙできなくなるので配列の削除はﾀﾞｲｱﾛｸﾞOK後
	reinterpret_cast<CMCTOOLINFO *>(m_ctToolList.GetItemData(nIndex))->m_bDlgDel = TRUE;

	m_ctToolList.DeleteItem(nIndex);
	m_nToolNo.SetFocus();
	m_nToolNo.SetSel(0, -1);
	m_bChange = TRUE;
}
