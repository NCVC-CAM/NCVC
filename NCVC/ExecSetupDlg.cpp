// ExecSetupDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ExecOption.h"
#include "ExecSetupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CExecSetupDlg, CDialog)
	//{{AFX_MSG_MAP(CExecSetupDlg)
	ON_BN_CLICKED(IDC_EXE_ADD, &CExecSetupDlg::OnAdd)
	ON_BN_CLICKED(IDC_EXE_MOD, &CExecSetupDlg::OnMod)
	ON_BN_CLICKED(IDC_EXE_DEL, &CExecSetupDlg::OnDel)
	ON_BN_CLICKED(IDC_EXE_UP, &CExecSetupDlg::OnUp)
	ON_BN_CLICKED(IDC_EXE_DOWN, &CExecSetupDlg::OnDown)
	ON_BN_CLICKED(IDC_EXE_FILEUP, &CExecSetupDlg::OnFileUP)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_EXE_LIST, &CExecSetupDlg::OnGetDispInfoExeList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EXE_LIST, &CExecSetupDlg::OnItemChangedExeList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_EXE_LIST, &CExecSetupDlg::OnKeyDownList)
	//}}AFX_MSG_MAP
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExecSetupDlg ダイアログ

CExecSetupDlg::CExecSetupDlg() : CDialog(CExecSetupDlg::IDD, NULL)
{
	m_ilExec.Create(AfxGetNCVCMainWnd()->GetEnableToolBarImage(TOOLIMAGE_EXEC));
	//{{AFX_DATA_INIT(CExecSetupDlg)
	m_bNCType = FALSE;
	m_bDXFType = FALSE;
	m_bShort = FALSE;
	//}}AFX_DATA_INIT

	// 現在登録済みの情報は，登録・削除ﾌﾗｸﾞをｸﾘｱ
	PLIST_FOREACH(CExecOption* pExec, AfxGetNCVCApp()->GetExecList())
		pExec->m_bDlgAdd = FALSE;
		pExec->m_bDlgDel = FALSE;
	END_FOREACH
}

void CExecSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
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
// CExecSetupDlg ﾒﾝﾊﾞ関数

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
	// ｵﾌﾞｼﾞｪｸﾄの入れ替え
	CExecOption* pExec1 = reinterpret_cast<CExecOption *>(m_ctList.GetItemData(nList1));
	CExecOption* pExec2 = reinterpret_cast<CExecOption *>(m_ctList.GetItemData(nList2));
	m_ctList.SetItemData(nList1, reinterpret_cast<DWORD_PTR>(pExec2));
	m_ctList.SetItemData(nList2, reinterpret_cast<DWORD_PTR>(pExec1));

	// 再描画指示
	m_ctList.RedrawItems(min(nList1, nList2), max(nList1, nList2));
	m_ctList.SetItemState(nList1, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	m_ctList.EnsureVisible(nList1, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CExecSetupDlg メッセージ ハンドラ

BOOL CExecSetupDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	int			i = 0;
	CExecList*	pExeList = AfxGetNCVCApp()->GetExecList();

	// ﾘｽﾄｺﾝﾄﾛｰﾙのｲﾒｰｼﾞｾｯﾄ
	m_ctList.SetImageList(&m_ilExec, LVSIL_SMALL);
	// ﾘｽﾄｺﾝﾄﾛｰﾙの列挿入
	CRect	rc;
	m_ctList.GetClientRect(rc);
	m_ctList.InsertColumn(0, "", LVCFMT_LEFT, rc.Width());
	// ﾘｽﾄｺﾝﾄﾛｰﾙへの登録
	LV_ITEM	lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.iImage = I_IMAGECALLBACK;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	PLIST_FOREACH(CExecOption* pExec, pExeList)
		lvi.iItem  = i++;
		lvi.lParam = (LPARAM)pExec;
		if ( m_ctList.InsertItem(&lvi) < 0 ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_ADDITEM, i);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			break;
		}
	END_FOREACH
	// 列幅
	m_ctList.SetColumnWidth(0, LVSCW_AUTOSIZE);
	// 詳細表示
	if ( pExeList->IsEmpty() )
		m_ctFile.SetFocus();
	else
		m_ctList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

	// D&D受付
	DragAcceptFiles();

	return FALSE;
}

void CExecSetupDlg::OnOK() 
{
	CExecList*		pExeList = AfxGetNCVCApp()->GetExecList();
	CExecOption*	pExec;
	POSITION	pos1, pos2;

	// ﾀﾞｲｱﾛｸﾞで削除された情報を削除
	for ( pos1=pExeList->GetHeadPosition(); (pos2 = pos1); ) {
		pExec = pExeList->GetNext(pos1);
		if ( pExec->m_bDlgDel ) {
			delete	pExec;
			pExeList->RemoveAt(pos2);
		}
	}
	// ﾀﾞｲｱﾛｸﾞ情報を反映
	pExeList->RemoveAll();
	for ( int i=0; i<m_ctList.GetItemCount(); i++ )
		pExeList->AddTail( reinterpret_cast<CExecOption *>(m_ctList.GetItemData(i)) );

//	__super::OnOK();
	EndDialog(IDOK);
}

void CExecSetupDlg::OnCancel() 
{
	CExecList*		pExeList = AfxGetNCVCApp()->GetExecList();
	CExecOption*	pExec;
	POSITION	pos1, pos2;

	// ﾀﾞｲｱﾛｸﾞで登録された情報を削除
	for ( pos1=pExeList->GetHeadPosition(); (pos2 = pos1); ) {
		pExec = pExeList->GetNext(pos1);
		if ( pExec->m_bDlgAdd ) {
			delete	pExec;
			pExeList->RemoveAt(pos2);
		}
	}

	__super::OnCancel();
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
		plvdi->item.iImage = (int)(pExec->m_nImage);
}

void CExecSetupDlg::OnKeyDownList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	if ( pLVKeyDown->wVKey == VK_DELETE )
		OnDel();	// 削除ﾎﾞﾀﾝｲﾍﾞﾝﾄ
	*pResult = 0;
}

void CExecSetupDlg::OnAdd() 
{
	// ﾃﾞｰﾀﾁｪｯｸ
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
		pExec = new CExecOption(m_strFile,	// ﾀﾞｲｱﾛｸﾞ登録のﾏｰｸはｺﾝｽﾄﾗｸﾀにて
						m_strCommand, m_strToolTip, m_bNCType, m_bDXFType, m_bShort);
		pos = pExeList->AddTail(pExec);
		// ﾘｽﾄｺﾝﾄﾛｰﾙ用ｲﾒｰｼﾞﾘｽﾄ
		nImage = m_ilExec.Add(AfxGetNCVCMainWnd()->GetIconHandle(FALSE, m_strFile));
		if ( nImage < 0 ) {
			strMsg.Format(IDS_ERR_ADDITEM, m_ctList.GetItemCount()+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			delete	pExec;
			pExeList->RemoveAt(pos);
			return;
		}
		pExec->SetImageNo(nImage);
		// ﾘｽﾄｺﾝﾄﾛｰﾙへの登録
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
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( !CheckData() )
		return;

	CString	strMsg;
	CExecList*	pExeList = AfxGetNCVCApp()->GetExecList();

	// ﾃﾞｰﾀ置換 -> 削除ﾌﾗｸﾞを立てて新たに登録
	CExecOption* pExec = reinterpret_cast<CExecOption *>(m_ctList.GetItemData(nIndex));
	pExec->m_bDlgDel = TRUE;
	pos = NULL;
	pExec = NULL;
	try {
		pExec = new CExecOption(m_strFile,	// ﾀﾞｲｱﾛｸﾞ登録のﾏｰｸはｺﾝｽﾄﾗｸﾀにて
						m_strCommand, m_strToolTip, m_bNCType, m_bDXFType, m_bShort);
		pos = pExeList->AddTail(pExec);
		// ﾘｽﾄｺﾝﾄﾛｰﾙ用ｲﾒｰｼﾞﾘｽﾄ
		nImage = m_ilExec.Add(AfxGetNCVCMainWnd()->GetIconHandle(FALSE, m_strFile));
		if ( nImage < 0 ) {
			strMsg.Format(IDS_ERR_ADDITEM, nIndex+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			delete	pExec;
			pExeList->RemoveAt(pos);
			return;
		}
		pExec->SetImageNo(nImage);
		// ﾘｽﾄｺﾝﾄﾛｰﾙの該当ﾃﾞｰﾀを置換
		if ( !m_ctList.SetItemData(nIndex, reinterpret_cast<DWORD_PTR>(pExec)) ) {
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

	// ﾎﾟｲﾝﾀを削除するとｷｬﾝｾﾙできなくなるので配列の削除はﾀﾞｲｱﾛｸﾞOK後
	// ｲﾒｰｼﾞﾘｽﾄも削除するとこれに続くｲﾒｰｼﾞの順序が変わるので削除しない
	reinterpret_cast<CExecOption *>(m_ctList.GetItemData(nIndex))->m_bDlgDel = TRUE;

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
	if ( nIndex<0 || nIndex>=m_ctList.GetItemCount()-1 )
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
	if ( ::NCVC_FileDlgCommon(IDS_OPTION_EXEC, IDS_EXE_FILTER, TRUE, strFile, strPath) == IDOK ) {
		m_strFile = strFile;
		::Path_Name_From_FullPath(m_strFile, strPath, strFile, FALSE);	// 拡張子無視
		m_strToolTip = strFile;
		UpdateData(FALSE);
	}
}

void CExecSetupDlg::OnDropFiles(HDROP hDropInfo) 
{
	if ( ::DragQueryFile(hDropInfo, -1, NULL, 0) > 1 ) {
		// D&Dは1件だけ受け付け
		CString	strPath, strFile;
		UINT nLen = ::DragQueryFile(hDropInfo, 0, NULL, 0);
		::DragQueryFile(hDropInfo, 0, m_strFile.GetBuffer(nLen+1), nLen+1);
		m_strFile.ReleaseBuffer();
		::Path_Name_From_FullPath(m_strFile, strPath, strFile, FALSE);
		m_strToolTip = strFile;
		UpdateData(FALSE);
		m_ctFile.SetFocus();
		m_ctFile.SetSel(0, -1);
	}
	::DragFinish(hDropInfo);
}
