// DXFOption.cpp: CDXFOption �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCVCdefine.h"
#include "DXFOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

static	const	int		g_nDxfID[] = {
	IDS_REG_DXF_VIEWER, IDS_REG_DXF_ORGTYPE
};
static	const	int		g_nDxfOldID[] = {
	IDS_REG_DXF_REGEX, IDS_REG_DXF_MATCH, IDS_REG_DXF_ACCEPT
};
static	const	int		g_nDxfDef[] = {
	1, 0
};
extern	LPCTSTR	g_szDefaultLayer[] = {
	"ORIGIN", "CAM",
	"MOVE", "CORRECT"		// DXF�o�͂ɂ�������̫��ڲԖ��ɂ̂ݎg�p
};

/////////////////////////////////////////////////////////////////////////////
// CDXFOption �N���X�̍\�z/����

CDXFOption::CDXFOption()
{
	extern	LPCTSTR	gg_szRegKey;
	int		i;
	CString	strRegKey, strEntry, strTmp, strResult;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	for ( i=0; i<DXFLAYERSIZE; i++ ) {	// DXF[ORG|CAM|STR|MOV|COM]LAYER
		VERIFY(strEntry.LoadString(IDS_REG_DXF_ORGLAYER+i));
		m_strReadLayer[i] = AfxGetApp()->GetProfileString(strRegKey, strEntry,
			i<=DXFCAMLAYER ? g_szDefaultLayer[i] : strTmp );	// ��̫�����Ұ� �́C���_�Ɛ؍�ڲԂ̂�
	}
	m_regCutter = m_strReadLayer[DXFCAMLAYER];

	// ��߼��
	for ( i=0; i<SIZEOF(m_nDXF); i++ ) {
		VERIFY(strEntry.LoadString(g_nDxfID[i]));
		m_nDXF[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, g_nDxfDef[i]);
	}
	// ������߼�݂̍폜
	CRegKey	reg;
	// --- "Software\MNCT-S\NCVC\Settings"
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strRegKey) == ERROR_SUCCESS ) {
		for ( i=0; i<SIZEOF(g_nDxfOldID); i++ ) {
			VERIFY(strEntry.LoadString(g_nDxfOldID[i]));
			reg.DeleteValue(strEntry);
		}
		reg.Close();
	}

	m_enMakeType = NCMAKEMILL;
	try {
		// �؍����̧�ق̗���
		ReadInitHistory(NCMAKEMILL);
		ReadInitHistory(NCMAKELATHE);
		// ڲԖ��Ə���̧�ي֌W�̗���
		VERIFY(strEntry.LoadString(IDS_REG_DXF_LAYERTOINIT));
		for ( i=0; i<DXFMAXINITFILE; i++ ) {
			strTmp.Format(IDS_COMMON_FORMAT, strEntry, i);
			strResult = AfxGetApp()->GetProfileString(strRegKey, strTmp);
			if ( strResult.IsEmpty() )
				break;
			if ( ::IsFileExist(strResult, TRUE, FALSE) )
				m_strLayerToInitList.AddTail(strResult); 
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////
// ���ފ֐�

BOOL CDXFOption::ReadInitHistory(enMAKETYPE enType)
{
	extern	LPTSTR	g_pszExecDir;	// ���s�ިڸ��(NCVC.cpp)
	int		i;
	CStringList*	pList;
	CString	strRegKey, strEntry, strDefExt('.'),
			strTmp, strResult;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	if ( enType == NCMAKEMILL ) {
		pList = &m_strMillList;
		VERIFY(strEntry.LoadString(IDS_REG_DXF_INIT));
		VERIFY(strTmp.LoadString(IDS_NCIM_FILTER));
	}
	else {
		pList = &m_strLatheList;
		VERIFY(strEntry.LoadString(IDS_REG_DXF_LATHEINIT));
		VERIFY(strTmp.LoadString(IDS_NCIL_FILTER));
	}
	strDefExt += strTmp.Left(3);

	for ( i=0; i<DXFMAXINITFILE; i++ ) {
		strTmp.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strTmp);
		if ( strResult.IsEmpty() )
			break;
		if ( ::IsFileExist(strResult, TRUE, FALSE) )
			pList->AddTail(strResult); 
	}
	if ( pList->IsEmpty() ) {
		strResult  = g_pszExecDir;
		strResult += "INIT" + strDefExt;
		pList->AddTail(strResult);
	}

	return TRUE;
}

BOOL CDXFOption::SaveInitHistory(enMAKETYPE enType)
{
	int			i;
	POSITION	pos;
	const CStringList*	pList;
	CString		strRegKey, strEntry, strFmt;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	if ( enType == NCMAKEMILL ) {
		VERIFY(strEntry.LoadString(IDS_REG_DXF_INIT));
		pList = GetMillInitList();
	}
	else {
		VERIFY(strEntry.LoadString(IDS_REG_DXF_LATHEINIT));
		pList = GetLatheInitList();
	}

	for ( i=0; i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		AfxGetApp()->WriteProfileString(strRegKey, strFmt, NULL);
	}
	for ( i=0, pos=pList->GetHeadPosition(); pos && i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, pList->GetNext(pos)) )
			return FALSE;
	}
	m_enMakeType = enType;

	return TRUE;
}

BOOL CDXFOption::SaveLayerHistory(void)
{
	int			i;
	POSITION	pos;
	CString		strRegKey, strEntry, strFmt;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_DXF_LAYERTOINIT));

	for ( i=0; i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		AfxGetApp()->WriteProfileString(strRegKey, strFmt, NULL);
	}
	for ( i=0, pos=m_strLayerToInitList.GetHeadPosition();
				pos && i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, m_strLayerToInitList.GetNext(pos)) )
			return FALSE;
	}

	return TRUE;
}

BOOL CDXFOption::AddListHistory(CStringList& strList, LPCTSTR lpszSearch)
{
	if ( !lpszSearch || lstrlen(lpszSearch) <= 0 )
		return TRUE;
	if ( !strList.IsEmpty() &&
			strList.GetHead().CompareNoCase(lpszSearch) == 0 )
		return TRUE;

	// ������̌���(Find�͑啶������������ʂ���̂Ŏg���Ȃ�)
	try {
		POSITION pos1, pos2;
		for ( pos1=strList.GetHeadPosition(); (pos2=pos1); ) {
			if ( strList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
				// �����񂪂���΁C����������Đ擪��
				strList.RemoveAt(pos2);
				strList.AddHead(lpszSearch);
				return TRUE;
			}
		}
		// �Ȃ���ΐ擪�ɒǉ�
		strList.AddHead(lpszSearch);
		// DXFMAXINITFILE ���z����΍Ō������
		if ( strList.GetCount() > DXFMAXINITFILE )
			strList.RemoveTail();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

void CDXFOption::DelListHistory(CStringList& strList, LPCTSTR lpszSearch)
{
	POSITION pos1, pos2;
	for ( pos1=strList.GetHeadPosition(); (pos2 = pos1); ) {
		if ( strList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
			strList.RemoveAt(pos2);
			break;
		}
	}
}

BOOL CDXFOption::SaveDXFoption(void)
{
	int			i;
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	for ( i=0; i<DXFLAYERSIZE; i++ ) {
		VERIFY(strEntry.LoadString(IDS_REG_DXF_ORGLAYER+i));
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry, m_strReadLayer[i]) )
			return FALSE;
	}

	for ( i=0; i<SIZEOF(m_nDXF); i++ ) {
		VERIFY(strEntry.LoadString(g_nDxfID[i]));
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_nDXF[i]) )
			return FALSE;
	}

	return TRUE;
}
