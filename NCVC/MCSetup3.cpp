// MCSetup3.cpp : �C���v�������e�[�V���� �t�@�C��
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
	{"�s", LVCFMT_CENTER},
	{"���@��", LVCFMT_LEFT},
	{"�a�␳�l ", LVCFMT_RIGHT},
	{"���␳�l ", LVCFMT_RIGHT},
	{"�H������ ", LVCFMT_RIGHT}
};
static	LPCTSTR		g_szMillType[] = {
	"�����", "�ް�", "�ʎ��"
};

BEGIN_MESSAGE_MAP(CMCSetup3, CPropertyPage)
	//{{AFX_MSG_MAP(CMCSetup3)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MCST3_LIST, &CMCSetup3::OnItemChangedToolList)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_MCST3_LIST, &CMCSetup3::OnGetDispInfoToolList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MCST3_LIST, &CMCSetup3::OnColumnClickToolList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_MCST3_LIST, &CMCSetup3::OnKeyDownToolList)
	ON_BN_CLICKED(IDC_EXE_ADD, &CMCSetup3::OnAdd)
	ON_BN_CLICKED(IDC_EXE_MOD, &CMCSetup3::OnMod)
	ON_BN_CLICKED(IDC_EXE_DEL, &CMCSetup3::OnDel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup3 �v���p�e�B �y�[�W

CMCSetup3::CMCSetup3() : CPropertyPage(CMCSetup3::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	m_bChange = FALSE;
	m_nSortColumn = 0;

	// ���ݓo�^�ς݂̍H����́C�o�^�E�폜�׸ނ�ر
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	PLIST_FOREACH(CMCTOOLINFO* pToolInfo, &pMCopt->m_ltTool)
		pToolInfo->m_bDlgAdd = pToolInfo->m_bDlgDel = FALSE;
	END_FOREACH
	m_nType = pMCopt->m_nCorrectType;
}

void CMCSetup3::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMCSetup3)
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
// CMCSetup3 ���ފ֐�

void CMCSetup3::SetDetailData(const CMCTOOLINFO* pToolInfo)
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

void CMCSetup3::SetHeaderMark(int nNewColumn)
{
	CHeaderCtrl*	pHeader = m_ctToolList.GetHeaderCtrl();
	HDITEM			hdi;
	int				nPos = abs(m_nSortColumn) - 1;

	hdi.mask = HDI_FORMAT;

	// ϰ���폜
	if ( m_nSortColumn!=0 && m_nSortColumn!=nNewColumn ) {
		pHeader->GetItem(nPos, &hdi);
		hdi.fmt &= ~( HDF_SORTUP | HDF_SORTDOWN );
		pHeader->SetItem(nPos, &hdi);
	}

	// ϰ���}��
	if ( nNewColumn != 0 ) {
		nPos = abs(nNewColumn) - 1;
		pHeader->GetItem(nPos, &hdi);
		hdi.fmt |= ( nNewColumn<0 ? HDF_SORTUP : HDF_SORTDOWN );
		pHeader->SetItem(nPos, &hdi);
	}

	m_nSortColumn = nNewColumn;
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup3 ���b�Z�[�W �n���h��

BOOL CMCSetup3::OnInitDialog() 
{
	__super::OnInitDialog();

	int			i;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// ��ė�̎擾
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCTOOLSORT));
	int nSortLayer = (int)(AfxGetApp()->GetProfileInt(strRegKey, strEntry, 1));

	// ͯ�ް��`
	for ( i=0; i<SIZEOF(g_ToolHeader); i++ ) {
		m_ctToolList.InsertColumn(i,
			g_ToolHeader[i].lpszName, g_ToolHeader[i].nFormat);
	}
	// ��
	CRect		rc;
	m_ctToolList.GetClientRect(rc);
	i = m_ctToolList.GetStringWidth(g_ToolHeader[2].lpszName) + 16;	// "�a�␳�l "
	m_ctToolList.SetColumnWidth(0, i/2);
	m_ctToolList.SetColumnWidth(2, i);
	m_ctToolList.SetColumnWidth(3, i);
	m_ctToolList.SetColumnWidth(4, i);
	m_ctToolList.SetColumnWidth(1, rc.Width() -
		m_ctToolList.GetColumnWidth(0)-
		m_ctToolList.GetColumnWidth(2)-
		m_ctToolList.GetColumnWidth(3)-
		m_ctToolList.GetColumnWidth(4)-32);
	// ؽĺ��۰قւ̓o�^
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
	// �P�s�I��
	DWORD	dwStyle = m_ctToolList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	m_ctToolList.SetExtendedStyle(dwStyle);
	// �ڍו\��
	if ( pMCopt->m_ltTool.IsEmpty() )
		m_nToolNo.SetFocus();
	else {
		m_ctToolList.SortItems(CompareToolListFunc, nSortLayer);
		m_ctToolList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		// ͯ�ްϰ��}��
		SetHeaderMark(nSortLayer);		// m_nSortColumn�֑��
	}

	return FALSE;
}

BOOL CMCSetup3::OnApply() 
{
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// �޲�۸ނō폜���ꂽ�H������폜
	pMCopt->ReductionTools(FALSE);
	pMCopt->m_nCorrectType = m_nType;

	// ��ė�̕ۑ�
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCTOOLSORT));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_nSortColumn);

	// �ēǍ�����
	if ( m_bChange )
		static_cast<CMCSetup *>(GetParentSheet())->m_bReload = TRUE;

	return TRUE;
}

void CMCSetup3::OnCancel() 
{
	// �޲�۸ނœo�^���ꂽ�H������폜
	AfxGetNCVCApp()->GetMCOption()->ReductionTools(TRUE);
	__super::OnCancel();
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

	CString		strFmt;
	switch ( plvdi->item.iSubItem ) {
	case MCTOOL_T:		// �s�ԍ�
		lstrcpy(plvdi->item.pszText, boost::lexical_cast<std::string>(pToolInfo->m_nTool).c_str());
		break;
	case MCTOOL_NAME:	// �H�
		lstrcpy(plvdi->item.pszText, pToolInfo->m_strName);
		break;
	case MCTOOL_D:		// �a�␳
		strFmt.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolD);
		lstrcpy(plvdi->item.pszText, strFmt);
		break;
	case MCTOOL_H:		// ���␳
		strFmt.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolH);
		lstrcpy(plvdi->item.pszText, strFmt);
		break;
	case MCTOOL_TYPE:	// �H������
		if ( pToolInfo->m_nType>=0 && pToolInfo->m_nType<SIZEOF(g_szMillType) )
			lstrcpy(plvdi->item.pszText, g_szMillType[pToolInfo->m_nType]);
		break;
	}
}

void CMCSetup3::OnColumnClickToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int	nNewColumn = pNMListView->iSubItem + 1;
	if ( abs(m_nSortColumn) == nNewColumn )
		nNewColumn = -m_nSortColumn;	// �����E�~���̐؂�ւ�
	// ���בւ�
	m_ctToolList.SortItems(CompareToolListFunc, nNewColumn);
	// ͯ�ްϰ��}��
	SetHeaderMark(nNewColumn);

	*pResult = 0;
}

int	CALLBACK CMCSetup3::CompareToolListFunc
	(LPARAM lParam1, LPARAM lParam2, LPARAM lSubItem)
{
	CMCTOOLINFO*	pToolInfo1 = (CMCTOOLINFO *)lParam1;
	CMCTOOLINFO*	pToolInfo2 = (CMCTOOLINFO *)lParam2;
	int		nResult = 0, nSort = (int)lSubItem;

	switch ( abs(nSort)-1 ) {
	case MCTOOL_T:		// �s�ԍ�
		nResult = pToolInfo1->m_nTool - pToolInfo2->m_nTool;
		break;
	case MCTOOL_NAME:	// �H�
		nResult = pToolInfo1->m_strName.Compare(pToolInfo2->m_strName);
		break;
	case MCTOOL_D:		// �a�␳
		nResult = (int)( (pToolInfo1->m_dToolD - pToolInfo2->m_dToolD) * 1000 );
		break;
	case MCTOOL_H:		// ���␳
		nResult = (int)( (pToolInfo1->m_dToolH - pToolInfo2->m_dToolH) * 1000 );
		break;
	case MCTOOL_TYPE:	// �H������
		nResult = pToolInfo1->m_nType - pToolInfo2->m_nType;
		break;
	}

	return nSort != 0 ? (nResult * nSort) : nResult;
}

void CMCSetup3::OnKeyDownToolList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	if ( pLVKeyDown->wVKey == VK_DELETE )
		OnDel();	// �폜���ݲ����
	*pResult = 0;
}

void CMCSetup3::OnAdd() 
{
	// �ް�����
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

	// �H����̓o�^
	try {
		pToolInfo = new CMCTOOLINFO(m_nToolNo, m_strToolName, m_dToolD, m_dToolH, m_nMillType, TRUE);
		pos = pMCopt->m_ltTool.AddTail(pToolInfo);
		// ؽĺ��۰قւ̓o�^
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
	// �ް�����
	if ( !CheckData() )
		return;

	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// �ް��u�� -> �폜�׸ނ𗧂ĂĐV���ɓo�^
	CMCTOOLINFO*	pToolInfo = reinterpret_cast<CMCTOOLINFO *>(m_ctToolList.GetItemData(nIndex));
	pToolInfo->m_bDlgDel = TRUE;
	pos = NULL;
	pToolInfo = NULL;
	try {
		pToolInfo = new CMCTOOLINFO(m_nToolNo, m_strToolName, m_dToolD, m_dToolH, m_nMillType, TRUE);
		pos = pMCopt->m_ltTool.AddTail(pToolInfo);
		// ؽĺ��۰ق̊Y���ް���u��
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

void CMCSetup3::OnDel() 
{
	int			nIndex;
	POSITION	pos;
	if ( !(pos=m_ctToolList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctToolList.GetNextSelectedItem(pos)) < 0 )
		return;

	// �߲�����폜����Ʒ�ݾقł��Ȃ��Ȃ�̂Ŕz��̍폜���޲�۸�OK��
	reinterpret_cast<CMCTOOLINFO *>(m_ctToolList.GetItemData(nIndex))->m_bDlgDel = TRUE;

	m_ctToolList.DeleteItem(nIndex);
	m_nToolNo.SetFocus();
	m_nToolNo.SetSel(0, -1);
	m_bChange = TRUE;
}
