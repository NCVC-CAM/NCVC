// ExecSetupDlg.cpp : ƒCƒ“ƒvƒŠƒƒ“ƒe[ƒVƒ‡ƒ“ ƒtƒ@ƒCƒ‹
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ExecOption.h"
#include "ExecSetupDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CExecSetupDlg, CDialog)
	//{{AFX_MSG_MAP(CExecSetupDlg)
	ON_BN_CLICKED(IDC_EXE_ADD, OnAdd)
	ON_BN_CLICKED(IDC_EXE_MOD, OnMod)
	ON_BN_CLICKED(IDC_EXE_DEL, OnDel)
	ON_BN_CLICKED(IDC_EXE_UP, OnUp)
	ON_BN_CLICKED(IDC_EXE_DOWN, OnDown)
	ON_BN_CLICKED(IDC_EXE_FILEUP, OnFileUP)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_EXE_LIST, OnGetDispInfoExeList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EXE_LIST, OnItemChangedExeList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_EXE_LIST, OnKeyDownList)
	//}}AFX_MSG_MAP
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExecSetupDlg ƒ_ƒCƒAƒƒO

CExecSetupDlg::CExecSetupDlg() : CDialog(CExecSetupDlg::IDD, NULL)
{
	m_ilExec.Create(AfxGetNCVCMainWnd()->GetEnableToolBarImage(TOOLIMAGE_EXEC));
	//{{AFX_DATA_INIT(CExecSetupDlg)
	m_bNCType = FALSE;
	m_bDXFType = FALSE;
	m_bShort = FALSE;
	//}}AFX_DATA_INIT

	// Œ»Ý“o˜^Ï‚Ý‚Ìî•ñ‚ÍC“o˜^EíœÌ×¸Þ‚ð¸Ø±
	CExecList*		pExeList = AfxGetNCVCApp()->GetExecList();
	CExecOption*	pExec;
	for ( POSITION pos=pExeList->GetHeadPosition(); pos; ) {
		pExec = pExeList->GetNext(pos);
		pExec->m_bDlgAdd = FALSE;
		pExec->m_bDlgDel = FALSE;
	}
}

CExecSetupDlg::~CExecSetupDlg()
{
}

void CExecSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExecSetupDlg)
	DDX_Control(pDX, IDC_EXE_FILE, m_ctFile);
	DDX_Control(pDX, IDC_EXE_LIST, m_ctList);
	DDX_Text(pDX, IDC_EXE_FILE, m_strFile);
	DDX_Text(pDX, IDC_EXE_COMMAND, m_strCommand);
	DDX_Text(pDX, IDC_EXE_TOOLTIP, m_strToolTip);
	DDX_Check(pDX, IDC_EXE_NCTYPE, m_bNCType);
	DDX_Check(pDX, IDC_EXE_DXFTYPE, m_bDXFType);
	DDX_Check(pDX, IDC_EXE_FILE83, m_bShort);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CExecSetupDlg ÒÝÊÞŠÖ”

void CExecSetupDlg::SetDetailData(const CExecOption* pExec)
{
	if ( pExec ) {
		m_strFile		= pExec->m_strFileName;
		m_strCommand	= pExec->m_strCommand;
		m_strToolTip	= pExec->m_strToolTip;
		m_bNCType		= pExec->m_bNCType;
		m_bDXFType		= pExec->m_bDXFType;
		m_bShort		= pExec->m_bShort;
	}
	else {
		m_strFile.Empty();
		m_strCommand.Empty();
		m_strToolTip.Empty();
		m_bNCType		= FALSE;
		m_bDXFType		= FALSE;
		m_bShort		= FALSE;
	}
	UpdateData(FALSE);
}

BOOL CExecSetupDlg::CheckData(void)
{
	UpdateData();
	CString		strPath, strFile;
	::Path_Name_From_FullPath(m_strFile, strPath, strFile);
	if ( strFile.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
		m_ctFile.SetFocus();
		m_ctFile.SetSel(0, -1);
		return FALSE;
	}
	return TRUE;
}

void CExecSetupDlg::SwapObject(int nList1, int nList2)
{
	// µÌÞ¼Þª¸Ä‚Ì“ü‚ê‘Ö‚¦
	CExecOption* pExec1 = (CExecOption *)(m_ctList.GetItemData(nList1));
	CExecOption* pExec2 = (CExecOption *)(m_ctList.GetItemData(nList2));
	m_ctList.SetItemData(nList1, (DWORD)pExec2);
	m_ctList.SetItemData(nList2, (DWORD)pExec1);

	// Ä•`‰æŽwŽ¦
	m_ctList.RedrawItems(min(nList1, nList2), max(nList1, nList2));
	m_ctList.SetItemState(nList1, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
}

/////////////////////////////////////////////////////////////////////////////
// CExecSetupDlg ƒƒbƒZ[ƒW ƒnƒ“ƒhƒ‰

BOOL CExecSetupDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int			i;
	POSITION	pos;
	CExecList*	pExeList = AfxGetNCVCApp()->GetExecList();

	// Ø½ÄºÝÄÛ°Ù‚Ì²Ò°¼Þ¾¯Ä
	m_ctList.SetImageList(&m_ilExec, LVSIL_SMALL);
	// Ø½ÄºÝÄÛ°Ù‚Ì—ñ‘}“ü
	CRect	rc;
	m_ctList.GetClientRect(&rc);
	m_ctList.InsertColumn(0, "", LVCFMT_LEFT, rc.Width());
	// Ø½ÄºÝÄÛ°Ù‚Ö‚Ì“o˜^
	LV_ITEM	lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.iImage = I_IMAGECALLBACK;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	for ( i=0, pos=pExeList->GetHeadPosition(); pos; i++ ) {
		lvi.iItem  = i;
		lvi.lParam = (LPARAM)(pExeList->GetNext(pos));
		if ( m_ctList.InsertItem(&lvi) < 0 ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_ADDITEM, i+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			break;
		}
	}
	// —ñ•
	m_ctList.SetColumnWidth(0, LVSCW_AUTOSIZE);
	// Ú×•\Ž¦
	if ( pExeList->IsEmpty() )
		m_ctFile.SetFocus();
	else
		m_ctList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

	// D&DŽó•t
	DragAcceptFiles();

	return FALSE;
}

void CExecSetupDlg::OnOK() 
{
	CExecList*		pExeList = AfxGetNCVCApp()->GetExecList();
	CExecOption*	pExec;
	POSITION	pos1, pos2;

	// ÀÞ²±Û¸Þ‚Åíœ‚³‚ê‚½î•ñ‚ðíœ
	for ( pos1=pExeList->GetHeadPosition(); (pos2 = pos1); ) {
		pExec = pExeList->GetNext(pos1);
		if ( pExec->m_bDlgDel ) {
			delete	pExec;
			pExeList->RemoveAt(pos2);
		}
	}
	// ÀÞ²±Û¸Þî•ñ‚ð”½‰f
	pExeList->RemoveAll();
	for ( int i=0; i<m_ctList.GetItemCount(); i++ )
		pExeList->AddTail( (CExecOption *)(m_ctList.GetItemData(i)) );

	CDialog::OnOK();
}

void CExecSetupDlg::OnCancel() 
{
	CExecList*		pExeList = AfxGetNCVCApp()->GetExecList();
	CExecOption*	pExec;
	POSITION	pos1, pos2;

	// ÀÞ²±Û¸Þ‚Å“o˜^‚³‚ê‚½î•ñ‚ðíœ
	for ( pos1=pExeList->GetHeadPosition(); (pos2 = pos1); ) {
		pExec = pExeList->GetNext(pos1);
		if ( pExec->m_bDlgAdd ) {
			delete	pExec;
			pExeList->RemoveAt(pos2);
		}
	}

	CDialog::OnCancel();
}

void CExecSetupDlg::OnItemChangedExeList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ( pNMListView->uNewState & LVIS_SELECTED )
		SetDetailData( (CExecOption *)(pNMListView->lParam) );
	*pResult = 0;
}

void CExecSetupDlg::OnGetDispInfoExeList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	if ( !(plvdi->item.mask & (LVIF_TEXT|LVIF_IMAGE)) )
		return;
	CExecOption* pExec = (CExecOption *)(plvdi->item.lParam);
	if ( !pExec )
		return;

	if ( plvdi->item.mask & LVIF_TEXT )
		lstrcpy(plvdi->item.pszText, pExec->m_strFileName);
	if ( plvdi->item.mask & LVIF_IMAGE )
		plvdi->item.iImage = pExec->m_nImage;
}

void CExecSetupDlg::OnKeyDownList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	if ( pLVKeyDown->wVKey == VK_DELETE )
		OnDel();	// íœÎÞÀÝ²ÍÞÝÄ
	*pResult = 0;
}

void CExecSetupDlg::OnAdd() 
{
	// ÃÞ°ÀÁª¯¸
	CExecList*	pExeList = AfxGetNCVCApp()->GetExecList();
	if ( pExeList->GetCount() > ADDINSTARTID - EXECSTARTID /*100*/ ) {
		AfxMessageBox(IDS_ERR_MAXADD, MB_OK|MB_ICONSTOP);
		return;
	}
	if ( !CheckData() )
		return;

	CString	strMsg;
	CExecOption*	pExec = NULL;
	POSITION	pos = NULL;
	int		nImage = -1, nIndex = -1;
	LV_ITEM		lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.iSubItem = 0;
	lvi.iImage = I_IMAGECALLBACK;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.stateMask = lvi.state = LVIS_SELECTED|LVIS_FOCUSED;
	
	try {
		pExec = new CExecOption(m_strFile,	// ÀÞ²±Û¸Þ“o˜^‚ÌÏ°¸‚ÍºÝ½Ä×¸À‚É‚Ä
						m_strCommand, m_strToolTip, m_bNCType, m_bDXFType, m_bShort);
		pos = pExeList->AddTail(pExec);
		// Ø½ÄºÝÄÛ°Ù—p²Ò°¼ÞØ½Ä
		nImage = m_ilExec.Add(AfxGetNCVCMainWnd()->GetIconHandle(FALSE, m_strFile));
		if ( nImage < 0 ) {
			strMsg.Format(IDS_ERR_ADDITEM, m_ctList.GetItemCount()+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			delete	pExec;
			pExeList->RemoveAt(pos);
			return;
		}
		pExec->SetImageNo(nImage);
		// Ø½ÄºÝÄÛ°Ù‚Ö‚Ì“o˜^
		lvi.iItem  = m_ctList.GetItemCount();
		lvi.lParam = (LPARAM)pExec;
		nIndex = m_ctList.InsertItem(&lvi);
		if ( nIndex < 0 ) {
			strMsg.Format(IDS_ERR_ADDITEM, m_ctList.GetItemCount()+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			delete	pExec;
			pExeList->RemoveAt(pos);
			m_ilExec.Remove(nImage);
			return;
		}
		m_ctList.EnsureVisible(nIndex, FALSE);
		m_ctFile.SetFocus();
		m_ctFile.SetSel(0, -1);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pExec )
			delete	pExec;
		if ( pos )
			pExeList->RemoveAt(pos);
		if ( nImage > 0 )
			m_ilExec.Remove(nImage);
		if ( nIndex > 0 )
			m_ctList.DeleteItem(nIndex);
		SetDetailData(NULL);
	}
}

void CExecSetupDlg::OnMod() 
{
	int			nIndex, nImage = -1;
	POSITION	pos;
	if ( !(pos=m_ctList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctList.GetNextSelectedItem(pos)) < 0 )
		return;
	// ÃÞ°ÀÁª¯¸
	if ( !CheckData() )
		return;

	CString	strMsg;
	CExecList*	pExeList = AfxGetNCVCApp()->GetExecList();

	// ÃÞ°À’uŠ· -> íœÌ×¸Þ‚ð—§‚Ä‚ÄV‚½‚É“o˜^
	CExecOption* pExec = (CExecOption *)(m_ctList.GetItemData(nIndex));
	pExec->m_bDlgDel = TRUE;
	pos = NULL;
	pExec = NULL;
	try {
		pExec = new CExecOption(m_strFile,	// ÀÞ²±Û¸Þ“o˜^‚ÌÏ°¸‚ÍºÝ½Ä×¸À‚É‚Ä
						m_strCommand, m_strToolTip, m_bNCType, m_bDXFType, m_bShort);
		pos = pExeList->AddTail(pExec);
		// Ø½ÄºÝÄÛ°Ù—p²Ò°¼ÞØ½Ä
		nImage = m_ilExec.Add(AfxGetNCVCMainWnd()->GetIconHandle(FALSE, m_strFile));
		if ( nImage < 0 ) {
			strMsg.Format(IDS_ERR_ADDITEM, nIndex+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			delete	pExec;
			pExeList->RemoveAt(pos);
			return;
		}
		pExec->SetImageNo(nImage);
		// Ø½ÄºÝÄÛ°Ù‚ÌŠY“–ÃÞ°À‚ð’uŠ·
		if ( !m_ctList.SetItemData(nIndex, (DWORD)pExec) ) {
			strMsg.Format(IDS_ERR_ADDITEM, nIndex+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			delete	pExec;
			pExeList->RemoveAt(pos);
			m_ilExec.Remove(nImage);
			return;
		}
		m_ctList.Update(nIndex);
		m_ctFile.SetFocus();
		m_ctFile.SetSel(0, -1);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pExec )
			delete	pExec;
		if ( pos )
			pExeList->RemoveAt(pos);
		if ( nImage > 0 )
			m_ilExec.Remove(nImage);
	}
}

void CExecSetupDlg::OnDel() 
{
	int			nIndex;
	POSITION	pos;
	if ( !(pos=m_ctList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctList.GetNextSelectedItem(pos)) < 0 )
		return;

	// Îß²ÝÀ‚ðíœ‚·‚é‚Æ·¬Ý¾Ù‚Å‚«‚È‚­‚È‚é‚Ì‚Å”z—ñ‚Ìíœ‚ÍÀÞ²±Û¸ÞOKŒã
	// ²Ò°¼ÞØ½Ä‚àíœ‚·‚é‚Æ‚±‚ê‚É‘±‚­²Ò°¼Þ‚Ì‡˜‚ª•Ï‚í‚é‚Ì‚Åíœ‚µ‚È‚¢
	((CExecOption *)(m_ctList.GetItemData(nIndex)))->m_bDlgDel = TRUE;

	m_ctList.DeleteItem(nIndex);
}

void CExecSetupDlg::OnUp() 
{
	int			nIndex;
	POSITION	pos;
	if ( !(pos=m_ctList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctList.GetNextSelectedItem(pos)) <= 0 )
		return;
	SwapObject(nIndex-1, nIndex);
}

void CExecSetupDlg::OnDown() 
{
	int			nIndex;
	POSITION	pos;
	if ( !(pos=m_ctList.GetFirstSelectedItemPosition()) )
		return;
	nIndex = m_ctList.GetNextSelectedItem(pos);
	if ( nIndex<0 || nIndex>=m_ctList.GetItemCount() )
		return;
	SwapObject(nIndex+1, nIndex);
}

void CExecSetupDlg::OnFileUP() 
{
	UpdateData();
	CString		strPath, strFile;
	::Path_Name_From_FullPath(m_strFile, strPath, strFile);
	if ( strFile.IsEmpty() )
		strFile = m_strFile;
	if ( ::NCVC_FileDlgCommon(IDS_OPTION_EXEC, IDS_EXE_FILTER, strFile, strPath) == IDOK ) {
		m_strFile = strFile;
		::Path_Name_From_FullPath(m_strFile, strPath, strFile, FALSE);	// Šg’£Žq–³Ž‹
		m_strToolTip = strFile;
		UpdateData(FALSE);
	}
}

void CExecSetupDlg::OnDropFiles(HDROP hDropInfo) 
{
	if ( ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0) == 1 ) {
		CString	strPath, strFile;
		TCHAR	szFileName[_MAX_PATH];
		::DragQueryFile(hDropInfo, 0, szFileName, _MAX_PATH);
		m_strFile = szFileName;
		::Path_Name_From_FullPath(m_strFile, strPath, strFile, FALSE);
		m_strToolTip = strFile;
		UpdateData(FALSE);
		m_ctFile.SetFocus();
		m_ctFile.SetSel(0, -1);
	}
	::DragFinish(hDropInfo);
}
