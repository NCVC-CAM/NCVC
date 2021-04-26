// ExtensionDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "ExtensionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CExtensionDlg, CDialog)
	//{{AFX_MSG_MAP(CExtensionDlg)
	ON_BN_CLICKED(IDC_EXT_NCD_ADD, &CExtensionDlg::OnExtAdd)
	ON_BN_CLICKED(IDC_EXT_DXF_ADD, &CExtensionDlg::OnExtAdd)
	ON_BN_CLICKED(IDC_EXT_NCD_DEL, &CExtensionDlg::OnExtDel)
	ON_BN_CLICKED(IDC_EXT_DXF_DEL, &CExtensionDlg::OnExtDel)
	ON_BN_CLICKED(IDC_EXT_NCD_DEF, &CExtensionDlg::OnExtDefault)
	ON_LBN_SELCHANGE(IDC_EXT_NCD_LIST, &CExtensionDlg::OnExtSelchangeList)
	ON_LBN_SELCHANGE(IDC_EXT_DXF_LIST, &CExtensionDlg::OnExtSelchangeList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExtensionDlg �_�C�A���O

CExtensionDlg::CExtensionDlg() : CDialog(CExtensionDlg::IDD, NULL)
{
	m_strExtDefault = AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->m_strDefaultExt;
}

void CExtensionDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExtensionDlg)
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EXT_NCD_TXT, m_strExtTxt[0]);
	DDX_Text(pDX, IDC_EXT_DXF_TXT, m_strExtTxt[1]);
	DDX_Text(pDX, IDC_EXT_NCD_DEFTXT, m_strExtDefault);
	DDX_Control(pDX, IDC_EXT_NCD_TXT, m_ctExtTxt[0]);
	DDX_Control(pDX, IDC_EXT_DXF_TXT, m_ctExtTxt[1]);
	DDX_Control(pDX, IDC_EXT_NCD_LIST, m_ctExtList[0]);
	DDX_Control(pDX, IDC_EXT_DXF_LIST, m_ctExtList[1]);
	DDX_Control(pDX, IDC_EXT_NCD_DEL, m_ctExtDelBtn[0]);
	DDX_Control(pDX, IDC_EXT_DXF_DEL, m_ctExtDelBtn[1]);
	DDX_Control(pDX, IDC_EXT_NCD_DEF, m_ctExtDefBtn);
}

/////////////////////////////////////////////////////////////////////////////
// CExtensionDlg ���b�Z�[�W �n���h��

BOOL CExtensionDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	CString	strResult;
	typedef	std::pair<CString, LPVOID>	PAIR;

	// ؽĺ��۰قւ̓o�^
	for ( int i=0; i<SIZEOF(m_ctExtList); i++ ) {
		strResult = AfxGetNCVCApp()->GetDocExtString((DOCTYPE)i).Right(3);	// ncd or cam
		m_ctExtList[i].SetItemData(m_ctExtList[i].AddString(strResult), 0);	// �폜�s�\ϰ�
		for ( int j=0; j<2/*SIZEOF(m_mpExt)*/; j++ ) {
			BOOST_FOREACH(PAIR p, AfxGetNCVCApp()->GetDocTemplate((DOCTYPE)i)->m_mpExt[j]) {
				m_ctExtList[i].SetItemData(m_ctExtList[i].AddString(p.first), j);
			}
		}
	}

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CExtensionDlg::OnOK() 
{
	CMapStringToPtr*	pMapExt;
	int		i, j, nCnt;
	CString	strList;
	typedef	std::pair<CString, LPVOID>	PAIR;

	try {
		for ( i=0; i<SIZEOF(m_ctExtList); i++ ) {
			nCnt = m_ctExtList[i].GetCount();
			// ϯ�߂ɂ�����ؽ��ޯ���ɂȂ����̂��폜
			pMapExt = &(AfxGetNCVCApp()->GetDocTemplate((DOCTYPE)i)->m_mpExt[EXT_DLG]);
			BOOST_FOREACH(PAIR p, *pMapExt) {
				for ( j=0; j<nCnt; j++ ) {	// FindString() �ł͕�����v���Ă��܂�
					m_ctExtList[i].GetText(j, strList);
					if ( p.first == strList )
						break;	// �����g���q�������
				}
				if ( j >= nCnt )
					pMapExt->RemoveKey(p.first);
			}
			// ؽ��ޯ���ɂ�����ϯ�߂ɂȂ����̂�o�^
			for ( j=0; j<nCnt; j++ ) {
				if ( m_ctExtList[i].GetItemData(j) > 0 ) {
					m_ctExtList[i].GetText(j, strList);
					pMapExt->SetAt(strList, NULL);	// �T���Ė�����Γo�^
				}
			}
		}
		// �f�t�H���g�g���q
		AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->m_strDefaultExt = m_strExtDefault;
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	EndDialog(IDOK);
}

void CExtensionDlg::OnExtAdd() 
{
	UpdateData();
	int	nID = GetFocus()->GetDlgCtrlID() - IDC_EXT_NCD_ADD;
	ASSERT( nID>=0 && nID<SIZEOF(m_strExtTxt) );
	CString	strTmp(m_strExtTxt[nID]);
	m_strExtTxt[nID] = strTmp.Trim();
	if ( m_strExtTxt[nID].IsEmpty() ) {
		::MessageBeep(MB_ICONASTERISK);
		return;
	}
	int		i, j, nCnt;
	for ( i=0; i<SIZEOF(m_ctExtList); i++ ) {
		nCnt = m_ctExtList[i].GetCount();
		for ( j=0; j<nCnt; j++ ) {
			m_ctExtList[i].GetText(j, strTmp);
			if ( m_strExtTxt[nID] == strTmp ) {
				::MessageBeep(MB_ICONASTERISK);
				return;
			}
		}
	}
	m_ctExtList[nID].SetItemData(m_ctExtList[nID].AddString(m_strExtTxt[nID]), 1);
	m_strExtTxt[nID].Empty();
	UpdateData(FALSE);
	m_ctExtTxt[nID].SetFocus();
}

void CExtensionDlg::OnExtDel() 
{
	int	nID = GetFocus()->GetDlgCtrlID() - IDC_EXT_NCD_DEL;
	ASSERT( nID>=0 && nID<SIZEOF(m_ctExtList) );
	int	nIndex = m_ctExtList[nID].GetCurSel();
	// �޲�۸ޓo�^�������폜�Ώ�
	if ( nIndex != LB_ERR && m_ctExtList[nID].GetItemData(nIndex) > 0 ) {
		if ( nID == 0 ) {
			// �f�t�H���g�g���q�ƃ}�b�`������f�t�H���g�g���q���폜
			CString	strBuf;
			m_ctExtList[0].GetText(nIndex, strBuf);
			if ( m_strExtDefault.CompareNoCase(strBuf) == 0 ) {
				m_strExtDefault.Empty();
				UpdateData(FALSE);
			}
		}
		m_ctExtList[nID].DeleteString( nIndex );
		m_ctExtList[nID].SetFocus();
		int n = m_ctExtList[nID].GetCount() - 1;
		m_ctExtList[nID].SetCurSel( min(nIndex, n) );
		OnExtSelchangeList();
	}
}

void CExtensionDlg::OnExtDefault()
{
	int	nIndex = m_ctExtList[0].GetCurSel();
	if ( nIndex != LB_ERR ) {
		CString	strBuf;
		m_ctExtList[0].GetText(nIndex, strBuf);
		if ( strBuf != "*" ) {	// 2�d�`�F�b�N
			m_strExtDefault = strBuf;
			UpdateData(FALSE);
		}
	}
}

void CExtensionDlg::OnExtSelchangeList() 
{
	int	nID = GetFocus()->GetDlgCtrlID() - IDC_EXT_NCD_LIST;
	ASSERT( nID>=0 && nID<SIZEOF(m_ctExtDelBtn) );
	int	nIndex = m_ctExtList[nID].GetCurSel();
	m_ctExtDelBtn[nID].EnableWindow( nIndex!=LB_ERR && m_ctExtList[nID].GetItemData(nIndex)!=0 ?
		TRUE : FALSE);

	if ( nID == 0 ) {
		BOOL	bDefEnable = TRUE;
		if ( nIndex != LB_ERR ) {
			CString	strBuf;
			m_ctExtList[0].GetText(nIndex, strBuf);
			if ( strBuf == "*" )
				bDefEnable = FALSE;
		}
		else
			bDefEnable = FALSE;
		m_ctExtDefBtn.EnableWindow( bDefEnable );
	}
}
