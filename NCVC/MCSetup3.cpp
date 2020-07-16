// MCSetup3.cpp : ƒCƒ“ƒvƒŠƒƒ“ƒe[ƒVƒ‡ƒ“ ƒtƒ@ƒCƒ‹
//

#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "MCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

typedef	struct tagTOOLHEADER {
	LPCTSTR		lpszName;
	int			nFormat;
} TOOLHEADER;
static	TOOLHEADER	g_ToolHeader[] = {
	{"‚s”Ô†", LVCFMT_CENTER},
	{"–¼@Ì", LVCFMT_LEFT},
	{"@@@Œa•â³’l ", LVCFMT_RIGHT},
	{"@@@’·•â³’l ", LVCFMT_RIGHT}
};

BEGIN_MESSAGE_MAP(CMCSetup3, CPropertyPage)
	//{{AFX_MSG_MAP(CMCSetup3)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MCST3_LIST, OnItemChangedToolList)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_MCST3_LIST, OnGetDispInfoToolList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MCST3_LIST, OnColumnClickToolList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_MCST3_LIST, OnKeyDownToolList)
	ON_BN_CLICKED(IDC_EXE_ADD, OnAdd)
	ON_BN_CLICKED(IDC_EXE_MOD, OnMod)
	ON_BN_CLICKED(IDC_EXE_DEL, OnDel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup3 ƒvƒƒpƒeƒB ƒy[ƒW

CMCSetup3::CMCSetup3() : CPropertyPage(CMCSetup3::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMCSetup3)
	//}}AFX_DATA_INIT
	m_bChange = FALSE;

	// Œ»Ý“o˜^Ï‚Ý‚ÌH‹ïî•ñ‚ÍC“o˜^EíœÌ×¸Þ‚ð¸Ø±
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	CMCTOOLINFO*	pToolInfo;
	for ( POSITION pos=pMCopt->m_ltTool.GetHeadPosition(); pos; ) {
		pToolInfo = pMCopt->m_ltTool.GetNext(pos);
		pToolInfo->m_bDlgAdd = pToolInfo->m_bDlgDel = FALSE;
	}
	m_nType = pMCopt->m_nCorrectType;
}

CMCSetup3::~CMCSetup3()
{
}

void CMCSetup3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMCSetup3)
	DDX_Control(pDX, IDC_MCST3_H, m_dToolH);
	DDX_Control(pDX, IDC_MCST3_D, m_dToolD);
	DDX_Control(pDX, IDC_MCST3_TOOLNO, m_nToolNo);
	DDX_Control(pDX, IDC_MCST3_LIST, m_ctToolList);
	DDX_Text(pDX, IDC_MCST3_NAME, m_strToolName);
	DDX_Radio(pDX, IDC_MCST3_TYPEA, m_nType);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup3 ÒÝÊÞŠÖ”

void CMCSetup3::SetDetailData(const CMCTOOLINFO* pToolInfo)
{
	if ( pToolInfo ) {
		m_nToolNo		= pToolInfo->m_nTool;
		m_strToolName	= pToolInfo->m_strName;
		m_dToolD		= pToolInfo->m_dToolD;
		m_dToolH		= pToolInfo->m_dToolH;
	}
	else {
		m_nToolNo		= 0;
		m_strToolName.Empty();
		m_dToolD		= 0;
		m_dToolH		= 0;
	}
	UpdateData(FALSE);
}

BOOL CMCSetup3::CheckData(void)
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

/////////////////////////////////////////////////////////////////////////////
// CMCSetup3 ƒƒbƒZ[ƒW ƒnƒ“ƒhƒ‰

BOOL CMCSetup3::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	int			i;
	POSITION	pos;
	CRect		rc;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// Í¯ÀÞ°’è‹`
	for ( i=0; i<SIZEOF(g_ToolHeader); i++ ) {
		m_ctToolList.InsertColumn(i,
			g_ToolHeader[i].lpszName, g_ToolHeader[i].nFormat);
	}
	// —ñ•
	m_ctToolList.GetClientRect(&rc);
	m_ctToolList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	m_ctToolList.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
	m_ctToolList.SetColumnWidth(3, m_ctToolList.GetColumnWidth(2));
	m_ctToolList.SetColumnWidth(1, rc.Width() -
		m_ctToolList.GetColumnWidth(0)-
		m_ctToolList.GetColumnWidth(2)-
		m_ctToolList.GetColumnWidth(3));
	// Ø½ÄºÝÄÛ°Ù‚Ö‚Ì“o˜^
	LV_ITEM	lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	for ( i=0, pos=pMCopt->m_ltTool.GetHeadPosition(); pos; i++ ) {
		lvi.iItem  = i;
		lvi.lParam = (LPARAM)(pMCopt->m_ltTool.GetNext(pos));
		if ( m_ctToolList.InsertItem(&lvi) < 0 ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_ADDITEM, i+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			break;
		}
	}
	// ‘S‚Ä‚Ì—ñ‚ð‘I‘ð‰Â”\‚É‚·‚é
	DWORD	dwStyle = m_ctToolList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	m_ctToolList.SetExtendedStyle(dwStyle);
	// Ú×•\Ž¦
	if ( pMCopt->m_ltTool.IsEmpty() )
		m_nToolNo.SetFocus();
	else {
		m_ctToolList.SortItems(CompareToolListFunc, 0);
		m_ctToolList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	}

	return FALSE;
}

BOOL CMCSetup3::OnApply() 
{
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// ÀÞ²±Û¸Þ‚Åíœ‚³‚ê‚½H‹ïî•ñ‚ðíœ
	pMCopt->ReductionTools(FALSE);
	pMCopt->m_nCorrectType = m_nType;

	// Ä“ÇžÁª¯¸
	if ( m_bChange )
		((CMCSetup *)GetParent())->m_bReload = TRUE;

	return TRUE;
}

void CMCSetup3::OnCancel() 
{
	// ÀÞ²±Û¸Þ‚Å“o˜^‚³‚ê‚½H‹ïî•ñ‚ðíœ
	AfxGetNCVCApp()->GetMCOption()->ReductionTools(TRUE);
	CPropertyPage::OnCancel();
}

void CMCSetup3::OnItemChangedToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ( pNMListView->uNewState & LVIS_SELECTED )
		SetDetailData( (CMCTOOLINFO *)(pNMListView->lParam) );
	*pResult = 0;
}

void CMCSetup3::OnGetDispInfoToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	if ( !(plvdi->item.mask & LVIF_TEXT) )
		return;
	CMCTOOLINFO*	pToolInfo = (CMCTOOLINFO *)(plvdi->item.lParam);
	if ( !pToolInfo )
		return;

	CString			strFmt;
	switch ( plvdi->item.iSubItem ) {
	case 0:		// ‚s”Ô†
		strFmt.Format("%d", pToolInfo->m_nTool);
		lstrcpy(plvdi->item.pszText, strFmt);
		break;
	case 1:		// H‹ï–¼
		lstrcpy(plvdi->item.pszText, pToolInfo->m_strName);
		break;
	case 2:		// Œa•â³
		strFmt.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolD);
		lstrcpy(plvdi->item.pszText, strFmt);
		break;
	case 3:		// ’·•â³
		strFmt.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolH);
		lstrcpy(plvdi->item.pszText, strFmt);
		break;
	}
}

void CMCSetup3::OnColumnClickToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	m_ctToolList.SortItems(CompareToolListFunc, pNMListView->iSubItem);
	*pResult = 0;
}

int	CALLBACK CMCSetup3::CompareToolListFunc
	(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CMCTOOLINFO*	pToolInfo1 = (CMCTOOLINFO *)lParam1;
	CMCTOOLINFO*	pToolInfo2 = (CMCTOOLINFO *)lParam2;
	int		nResult = 0;

	switch ( lParamSort ) {
	case 0:		// ‚s”Ô†
		nResult = pToolInfo1->m_nTool - pToolInfo2->m_nTool;
		break;
	case 1:		// H‹ï–¼
		nResult = pToolInfo1->m_strName.Compare(pToolInfo2->m_strName);
		break;
	case 2:		// Œa•â³
		nResult = (int)( (pToolInfo1->m_dToolD - pToolInfo2->m_dToolD) * 1000 );
		break;
	case 3:		// ’·•â³
		nResult = (int)( (pToolInfo1->m_dToolH - pToolInfo2->m_dToolH) * 1000 );
		break;
	}

	return nResult;
}

void CMCSetup3::OnKeyDownToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	if ( pLVKeyDown->wVKey == VK_DELETE )
		OnDel();	// íœÎÞÀÝ²ÍÞÝÄ
	*pResult = 0;
}

void CMCSetup3::OnAdd() 
{
	// ÃÞ°ÀÁª¯¸
	if ( !CheckData() )
		return;

	POSITION	pos = NULL;
	int			nIndex = -1;
	CMCTOOLINFO*	pToolInfo = NULL;
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	LV_ITEM		lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.stateMask = lvi.state = LVIS_SELECTED|LVIS_FOCUSED;

	// H‹ïî•ñ‚Ì“o˜^
	try {
		pToolInfo = new CMCTOOLINFO(m_nToolNo, m_strToolName, m_dToolD, m_dToolH, TRUE);
		pos = pMCopt->m_ltTool.AddTail(pToolInfo);
		// Ø½ÄºÝÄÛ°Ù‚Ö‚Ì“o˜^
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

void CMCSetup3::OnMod() 
{
	int			nIndex;
	POSITION	pos;

	if ( !(pos=m_ctToolList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctToolList.GetNextSelectedItem(pos)) < 0 )
		return;
	// ÃÞ°ÀÁª¯¸
	if ( !CheckData() )
		return;

	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// ÃÞ°À’uŠ· -> íœÌ×¸Þ‚ð—§‚Ä‚ÄV‚½‚É“o˜^
	CMCTOOLINFO*	pToolInfo = (CMCTOOLINFO *)(m_ctToolList.GetItemData(nIndex));
	pToolInfo->m_bDlgDel = TRUE;
	pos = NULL;
	pToolInfo = NULL;
	try {
		pToolInfo = new CMCTOOLINFO(m_nToolNo, m_strToolName, m_dToolD, m_dToolH, TRUE);
		pos = pMCopt->m_ltTool.AddTail(pToolInfo);
		// Ø½ÄºÝÄÛ°Ù‚ÌŠY“–ÃÞ°À‚ð’uŠ·
		if ( !m_ctToolList.SetItemData(nIndex, (DWORD)pToolInfo) ) {
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

void CMCSetup3::OnDel() 
{
	int			nIndex;
	POSITION	pos;
	if ( !(pos=m_ctToolList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctToolList.GetNextSelectedItem(pos)) < 0 )
		return;

	// Îß²ÝÀ‚ðíœ‚·‚é‚Æ·¬Ý¾Ù‚Å‚«‚È‚­‚È‚é‚Ì‚Å”z—ñ‚Ìíœ‚ÍÀÞ²±Û¸ÞOKŒã
	((CMCTOOLINFO *)(m_ctToolList.GetItemData(nIndex)))->m_bDlgDel = TRUE;

	m_ctToolList.DeleteItem(nIndex);
	m_nToolNo.SetFocus();
	m_nToolNo.SetSel(0, -1);
	m_bChange = TRUE;
}
