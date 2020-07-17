// MakeNCDlgEx3.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCMakeOption.h"
#include "MakeNCDlgEx.h"
#include "MakeNCDlgEx11.h"
#include "MakeNCDlgEx21.h"

#include "MagaDbgMac.h"
#include "MakeNCDlgEx3.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

UINT	CMakeNCDlgEx3::m_nParentID = 0;
#define	GetNCMakeParent()	static_cast<CMakeNCDlgEx *>(GetParent())
#define	IsMakeEx1()	( m_nParentID == ID_FILE_DXF2NCD_EX1 )
#define	IsMakeEx2()	( m_nParentID == ID_FILE_DXF2NCD_EX2 )

// ؽĺ��۰�ͯ�ް������
static	LPCTSTR	g_szListHeader[] = {
	"ڲԖ�", "�؍����̧��", "�Ő[Z���W", "�����Ő[Z���W",
	"�ʏo��̧�ٖ�"
};
// �g�pͯ�ް��������ޯ��
static	int		HEADINDEX[][4] = {
	{0, 1, 2, 4},
	{0, 3, 4, 0}	// 4�Ԗڂ���а
};

BEGIN_MESSAGE_MAP(CMakeNCDlgEx3, CPropertyPage)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_MKNCEX_LAYERLIST, OnGetDispInfoLayerList)
	ON_NOTIFY(NM_DBLCLK, IDC_MKNCEX_LAYERLIST, OnDblClkLayerList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MKNCEX_LAYERLIST, OnColumnClickLayerList)
	ON_NOTIFY(LVN_ENDSCROLL, IDC_MKNCEX_LAYERLIST, OnEndScrollLayerList)
	ON_BN_CLICKED(IDC_EXE_UP, OnUp)
	ON_BN_CLICKED(IDC_EXE_DOWN, OnDown)
	ON_MESSAGE(PSM_QUERYSIBLINGS, OnQuerySiblings)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx3 �_�C�A���O

CMakeNCDlgEx3::CMakeNCDlgEx3() : CPropertyPage(CMakeNCDlgEx3::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	m_nSortColumn = 0;
}

CMakeNCDlgEx3::~CMakeNCDlgEx3()
{
}

void CMakeNCDlgEx3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MKNCEX_LAYERLIST, m_ctLayerList);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx3 ���b�Z�[�W �n���h��

BOOL CMakeNCDlgEx3::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ID�擾(����ޯ��֐��̂��߂ɽ�è�����ނɊi�[)
	m_nParentID = GetNCMakeParent()->GetNCMakeID();

	// ��ė�̎擾
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IsMakeEx1() ? IDS_REG_DXF_SORTLAYER1 : IDS_REG_DXF_SORTLAYER2));
	int nSortLayer = (int)(AfxGetApp()->GetProfileInt(strRegKey, strEntry, 1));

	// ؽĺ��۰ق̗�}��
	CRect	rc;
	m_ctLayerList.GetClientRect(rc);
	int nWidth  = m_ctLayerList.GetStringWidth(CString('W',10));
	// ڲԖ�
	m_ctLayerList.InsertColumn(0, g_szListHeader[0], LVCFMT_LEFT, nWidth-2);
	// ����ID�ɂ��񍀖ڂ̓o�^
	if ( IsMakeEx1() ) {
		int	nWidth2 = rc.Width()-nWidth*2-16;
		m_ctLayerList.InsertColumn(1, g_szListHeader[1], LVCFMT_LEFT, nWidth2);
		m_ctLayerList.InsertColumn(2, g_szListHeader[2], LVCFMT_RIGHT, nWidth+2);
		m_ctLayerList.InsertColumn(3, g_szListHeader[4], LVCFMT_LEFT, nWidth2);
	}
	else {
		m_ctLayerList.InsertColumn(1, g_szListHeader[3], LVCFMT_RIGHT, nWidth+4);
		m_ctLayerList.InsertColumn(2, g_szListHeader[4], LVCFMT_LEFT, rc.Width()-nWidth*2-17);
	}
	// ؽĺ��۰ق̊g������
	m_ctLayerList.SetExtendedStyle( m_ctLayerList.GetExtendedStyle() |
		LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES );
	// ڲ�ؽĺ��۰قւ̓o�^
	LV_ITEM		lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	CDXFDoc*	pDoc = GetNCMakeParent()->GetDocument();
	CLayerData*	pLayer;
	int		i, nCnt;

	for ( i=0, nCnt=0; i<pDoc->GetLayerCnt(); i++ ) {
		pLayer = pDoc->GetLayerData(i);
		if ( pLayer->IsCutType() ) {
			lvi.iItem = nCnt;
			lvi.lParam = (LPARAM)pLayer;
			if ( m_ctLayerList.InsertItem(&lvi) < 0 ) {
				CString	strMsg;
				strMsg.Format(IDS_ERR_ADDITEM, nCnt+1);
				AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
				break;
			}
			m_ctLayerList.SetCheck(nCnt++, pLayer->m_bLayerFlg[LAYER_CUTTARGET]);
		}
	}
	// ڲ�ؽĂ̕��בւ�
	m_ctLayerList.SortItems(CompareFunc, nSortLayer);
	// ͯ�ްϰ��}��
	SetHeaderMark(nSortLayer);		// m_nSortColumn�֑��

	UpdateData(FALSE);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CMakeNCDlgEx3::OnSetActive()
{
	// �u�O�ցv�Ɓu�����v���݂�L��
	GetNCMakeParent()->SetWizardButtons(PSWIZB_BACK|PSWIZB_FINISH);
	return TRUE;
}

BOOL CMakeNCDlgEx3::OnWizardFinish()
{
	extern	LPCTSTR	gg_szCat;

	if ( !OnKillActive() )	// ����Ĕ������Ȃ��̂ŋ����Ăяo��
		return FALSE;

	CNCMakeOption*	pMakeOpt = NULL;
	int			i, nResult = -1;
	const int	nLoop = m_ctLayerList.GetItemCount();
	CString		strMiss, strPartOut;
	CLayerData*	pLayer;

	if ( IsMakeEx2() )
		pMakeOpt = new CNCMakeOption(GetNCMakeParent()->m_strInitFileName);

	// �ŏI�ް�����
	for ( i=0; i<nLoop; i++ ) {
		if ( !m_ctLayerList.GetCheck(i) )
			continue;
		pLayer = reinterpret_cast<CLayerData *>(m_ctLayerList.GetItemData(i));
		if ( IsMakeEx1() ) {
			// �؍�������eڲԂɊ��蓖�Ă��Ă��邩
			if ( !::IsFileExist(pLayer->GetInitFile()) ) {
				SetFocusListCtrl(i);
				return FALSE;
			}
		}
		else {
			// R�_�Ƌ���Z���W������
			if ( pMakeOpt->GetDbl(MKNC_DBL_ZG0STOP) < pLayer->m_dZCut ) {
				// �װү���ނ̐���
				if ( !strMiss.IsEmpty() )
					strMiss += gg_szCat;
				strMiss += pLayer->m_strLayer;
				if ( nResult < 0 )
					nResult = i;
			}
		}
	}
	if ( pMakeOpt )		// IsMakeEx2()
		delete	pMakeOpt;

	if ( !strMiss.IsEmpty() ) {
		CString	strMsg;
		strMsg.Format(IDS_ERR_MAKEMULTILAYER_Z, strMiss);
		AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
		SetFocusListCtrl(nResult);
		return FALSE;
	}

	// �㏑���m�F
	for ( i=0; i<nLoop; i++ ) {
		if ( !m_ctLayerList.GetCheck(i) )
			continue;
		pLayer = reinterpret_cast<CLayerData *>(m_ctLayerList.GetItemData(i));
		// �o�͏��̐ݒ�
		pLayer->SetLayerListNo(pLayer->IsLayerFlag(LAYER_PARTOUT) ? -1 : i);
		// �ʏo�̗͂L��
		if ( pLayer->IsLayerFlag(LAYER_PARTOUT) ) {
			strPartOut = pLayer->GetNCFile();	// ��\�ōŏ��̂�
			break;
		}
	}
	if ( strPartOut.IsEmpty() )
		strPartOut = GetNCMakeParent()->m_strNCFileName;
	if ( !::IsFileExist(strPartOut, FALSE) )
		return FALSE;

	// ��ė�̕ۑ�
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IsMakeEx1() ? IDS_REG_DXF_SORTLAYER1 : IDS_REG_DXF_SORTLAYER2));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_nSortColumn);

	return TRUE;
}

BOOL CMakeNCDlgEx3::OnKillActive() 
{
	BOOL	bCutCheck = FALSE;
	CLayerData*	pLayer;

	// �ް����f
	for ( int i=0; i<m_ctLayerList.GetItemCount(); i++ ) {
		pLayer = reinterpret_cast<CLayerData *>(m_ctLayerList.GetItemData(i));
		pLayer->m_bLayerFlg.set(LAYER_CUTTARGET, m_ctLayerList.GetCheck(i));
		if ( pLayer->m_bLayerFlg[LAYER_CUTTARGET] )
			bCutCheck = TRUE;
	}
	// �؍�Ώۂ��P���Ȃ�
	if ( !bCutCheck ) {
		AfxMessageBox(IDS_ERR_MAKEMULTILAYER, MB_OK|MB_ICONEXCLAMATION);
		SetFocusListCtrl(0);
		return FALSE;
	}

	// ��̧�ُd������(�؍�Ώۂ��ύX�����ꍇ������̂ŁA�����ł�����)
	CString	strFile( GetNCMakeParent()->m_strNCFileName ),
			strResult( GetNCMakeParent()->GetDocument()->CheckDuplexFile(strFile) );
	if ( !strResult.IsEmpty() ) {
		LVFINDINFO	infoFind;
		infoFind.flags = LVFI_STRING;
		infoFind.psz = strResult;
		SetFocusListCtrl(m_ctLayerList.FindItem(&infoFind));
		return FALSE;
	}

	return TRUE;
}

void CMakeNCDlgEx3::OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	CString	strBuf;

	if ( pDispInfo->item.mask & LVIF_TEXT ) {
		CLayerData* pLayer = reinterpret_cast<CLayerData *>(pDispInfo->item.lParam);
		if ( IsMakeEx1() ) {
			switch ( pDispInfo->item.iSubItem ) {
			case 0:		// ڲԖ�
				lstrcpy(pDispInfo->item.pszText, pLayer->m_strLayer);
				break;
			case 1:		// �؍����̧��
				lstrcpy(pDispInfo->item.pszText, pLayer->m_strInitFile);
				break;
			case 2:		// �Ő[Z���W
				strBuf.Format(IDS_MAKENCD_FORMAT, pLayer->m_dInitZCut);
				lstrcpy(pDispInfo->item.pszText, strBuf);
				break;
			case 3:		// �ʏo��̧��
				if ( pLayer->m_bLayerFlg[LAYER_PARTOUT] )
					lstrcpy(pDispInfo->item.pszText, pLayer->m_strNCFile);
				else
					pDispInfo->item.pszText[0] = '\0';
				break;
			}
		}
		else {
			switch ( pDispInfo->item.iSubItem ) {
			case 0:		// ڲԖ�
				lstrcpy(pDispInfo->item.pszText, pLayer->m_strLayer);
				break;
			case 1:		// �����Ő[Z���W
				strBuf.Format(IDS_MAKENCD_FORMAT, pLayer->m_dZCut);
				lstrcpy(pDispInfo->item.pszText, strBuf);
				break;
			case 2:		// �ʏo��̧��
				if ( pLayer->m_bLayerFlg[LAYER_PARTOUT] )
					lstrcpy(pDispInfo->item.pszText, pLayer->m_strNCFile);
				else
					pDispInfo->item.pszText[0] = '\0';
				break;
			}
		}
	}

	*pResult = 0;
}

void CMakeNCDlgEx3::OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// ����ٸد����ꂽ���ڂ𒲂ׁC�ڍאݒ��޲�۸ނ�
	DWORD	dwPos = ::GetMessagePos();
	CPoint	pt((int)LOWORD(dwPos), (int)HIWORD(dwPos));
	m_ctLayerList.ScreenToClient(&pt);
	int		nIndex = m_ctLayerList.HitTest(pt);

	if ( IsMakeEx1() ) {
		CMakeNCDlgEx11	dlg(GetNCMakeParent(), nIndex);
		if ( dlg.DoModal() == IDOK )
			OnQuerySiblings(0, (LPARAM)(&(dlg.m_obLayer)));
	}
	else {
		CMakeNCDlgEx21	dlg(GetNCMakeParent(), nIndex);
		if ( dlg.DoModal() == IDOK )
			OnQuerySiblings(0, (LPARAM)(&(dlg.m_obLayer)));
	}

	*pResult = 0;
}

void CMakeNCDlgEx3::OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int	nNewColumn = pNMListView->iSubItem + 1;
	if ( abs(m_nSortColumn) == nNewColumn )
		nNewColumn = -m_nSortColumn;	// �����E�~���̐؂�ւ�
	// ���בւ�
	m_ctLayerList.SortItems(CompareFunc, nNewColumn);
	// ͯ�ްϰ��}��
	SetHeaderMark(nNewColumn);

	*pResult = 0;
}

void CMakeNCDlgEx3::OnEndScrollLayerList(NMHDR *, LRESULT *)
{
	// �����ĕ`�悳���邱�ƂŌr���������邱�Ƃ�h��
	m_ctLayerList.Invalidate();
}

int CALLBACK CMakeNCDlgEx3::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lSubItem)
{
	CLayerData*	pLayer1 = reinterpret_cast<CLayerData *>(lParam1);
	CLayerData*	pLayer2 = reinterpret_cast<CLayerData *>(lParam2);
	int	nResult = 0, nSort = (int)lSubItem;

	switch ( abs(nSort) ) {
	case 0:	// ���ݽ
		nResult = pLayer1->GetLayerListNo() - pLayer2->GetLayerListNo();
		break;
	case 1:	// ڲԖ�
		nResult = pLayer1->m_strLayer.CompareNoCase(pLayer2->m_strLayer);
		break;
	case 2:	// �؍����̧�� or �����Ő[Z���W
		if ( IsMakeEx1() )
			nResult = pLayer1->m_strInitFile.CompareNoCase(pLayer2->m_strInitFile);
		else
			nResult = (int)(pLayer1->m_dZCut * 1000.0 - pLayer2->m_dZCut * 1000.0);
		break;
	case 3:	// �Ő[Z���W IsMakeEx1()�̂�
		nResult = (int)(pLayer1->m_dInitZCut * 1000.0 - pLayer2->m_dInitZCut * 1000.0);
		break;
	}

	return nSort != 0 ? (nResult * nSort) : nResult;
}

void CMakeNCDlgEx3::OnUp() 
{
	int			nIndex;
	POSITION	pos;
	if ( !(pos=m_ctLayerList.GetFirstSelectedItemPosition()) ||
			(nIndex=m_ctLayerList.GetNextSelectedItem(pos)) <= 0 )
		return;
	SwapObject(nIndex-1, nIndex);
	SetHeaderMark(0);	// ��ĂȂ�
}

void CMakeNCDlgEx3::OnDown() 
{
	int			nIndex;
	POSITION	pos;
	if ( !(pos=m_ctLayerList.GetFirstSelectedItemPosition()) )
		return;
	nIndex = m_ctLayerList.GetNextSelectedItem(pos);
	if ( nIndex<0 || nIndex>=m_ctLayerList.GetItemCount()-1 )
		return;
	SwapObject(nIndex+1, nIndex);
	SetHeaderMark(0);
}

void CMakeNCDlgEx3::SwapObject(int nList1, int nList2)
{
	// ��޼ު�Ă̓���ւ�
	CLayerData* pLayer1 = reinterpret_cast<CLayerData *>(m_ctLayerList.GetItemData(nList1));
	CLayerData* pLayer2 = reinterpret_cast<CLayerData *>(m_ctLayerList.GetItemData(nList2));
	m_ctLayerList.SetItemData(nList1, reinterpret_cast<DWORD_PTR>(pLayer2));
	m_ctLayerList.SetItemData(nList2, reinterpret_cast<DWORD_PTR>(pLayer1));

	// �ĕ`��w��
	m_ctLayerList.RedrawItems(min(nList1, nList2), max(nList1, nList2));
	m_ctLayerList.SetItemState(nList1, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	m_ctLayerList.EnsureVisible(nList1, FALSE);
}

LRESULT CMakeNCDlgEx3::OnQuerySiblings(WPARAM wParam, LPARAM lParam)
{
	CLayerArray*	pArray = reinterpret_cast<CLayerArray *>(lParam);
	CLayerData*		pLayer1;
	CLayerData*		pLayer2;
	// ���ʂ̔��f
	for ( int i=0; i<m_ctLayerList.GetItemCount(); i++ ) {
		pLayer1 = reinterpret_cast<CLayerData *>(m_ctLayerList.GetItemData(i));
		pLayer2 = pArray->GetAt(i);
		pLayer1->m_bLayerFlg.set(LAYER_CUTTARGET, pLayer2->m_bLayerFlg[LAYER_CUTTARGET]);
		if ( IsMakeEx1() )
			pLayer1->SetInitFile(pLayer2->m_strInitFile);
		else {
			pLayer1->m_dZCut		= pLayer2->m_dZCut;
			pLayer1->m_bLayerFlg.set(LAYER_DRILLZ, pLayer2->m_bLayerFlg[LAYER_DRILLZ]);
		}
		pLayer1->m_bLayerFlg.set(LAYER_PARTOUT, pLayer2->m_bLayerFlg[LAYER_PARTOUT]);
		pLayer1->m_strNCFile		= pLayer2->m_strNCFile;
		pLayer1->m_strLayerComment	= pLayer2->m_strLayerComment;
		pLayer1->m_strLayerCode		= pLayer2->m_strLayerCode;
		m_ctLayerList.SetCheck(i, pLayer1->m_bLayerFlg[LAYER_CUTTARGET]);
	}
	m_ctLayerList.Invalidate();

	return 0;
}

void CMakeNCDlgEx3::SetFocusListCtrl(int nIndex)
{
	if ( nIndex < 0 )
		nIndex = 0;
	m_ctLayerList.SetFocus();
	m_ctLayerList.SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	m_ctLayerList.EnsureVisible(nIndex, FALSE);
}

void CMakeNCDlgEx3::SetHeaderMark(int nNewColumn)
{
	CHeaderCtrl*	pHeader = m_ctLayerList.GetHeaderCtrl();
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
