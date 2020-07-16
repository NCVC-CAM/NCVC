// Layer.cpp: CLayerData, CLayerMap クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeOption.h"
#include "DXFdata.h"
#include "Layer.h"
#include "Sstring.h"

#include <math.h>
#include <stdlib.h>

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

static	LPCTSTR	g_szLayerToInitComment[] = {
	"##\n",
	"## NCVC:ﾚｲﾔ名と切削条件ﾌｧｲﾙの関係情報ﾌｧｲﾙ\n",
	"## ﾚｲﾔ名，切削対象ﾌﾗｸﾞ(1:対象 0:除外)，切削条件ﾌｧｲﾙ,\n",
	"##\t強制最深Z, 強制最深Zを穴加工にも適用(1:する 0:しない),\n",
	"##\t個別出力(1:する 0:しない), 個別出力ﾌｧｲﾙ名\n"
};
#define	LAYERTOINITORDER	7

static	int		LayerCompareFunc(CLayerData*, CLayerData*);		// ﾚｲﾔ名の並べ替え

IMPLEMENT_SERIAL(CLayerData, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

//////////////////////////////////////////////////////////////////////
// CLayerData クラス
//////////////////////////////////////////////////////////////////////

void CLayerData::SetShapeSwitch_AllChainMap(BOOL bSelect)
{
	for ( int i=0; i<m_obChainMap.GetSize(); i++ )
		m_obChainMap[i]->SetShapeSwitch(bSelect);
}

void CLayerData::RemoveMasterMap(void)
{
	if ( !m_hClearMap ) {
		CWinThread*	pThread = AfxBeginThread(RemoveMasterMapThread, this,
				THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		m_hClearMap = ::NC_DuplicateHandle(pThread->m_hThread);
		pThread->ResumeThread();
	}
}

UINT CLayerData::RemoveMasterMapThread(LPVOID pParam)
{
	CLayerData*	pLayer = (CLayerData *)pParam;
	pLayer->m_mpDXFdata.RemoveAll();
	pLayer->m_hClearMap = NULL;
	return 0;
}

void CLayerData::SetInitFile
	(LPCTSTR lpszInitFile, CNCMakeOption* pNCMake/*=NULL*/, CLayerData* pLayer/*=NULL*/)
{
	m_dInitZCut = 0.0;
	if ( !lpszInitFile || lstrlen(lpszInitFile)<=0 ) {
		m_strInitFile.Empty();
		return;
	}
	m_strInitFile = lpszInitFile;
	// 条件ﾌｧｲﾙを読み込む必要がない
	if ( !pNCMake )
		return;
	// 同一ﾌｧｲﾙ名なら読み込む必要なし
	if ( pLayer && m_strInitFile.CompareNoCase(pLayer->m_strInitFile)==0 ) {
		m_dInitZCut = pLayer->m_dInitZCut;
		return;
	}
	// NC生成ｵﾌﾟｼｮﾝの読み込み
	pNCMake->ReadMakeOption(m_strInitFile);
	m_dInitZCut = pNCMake->GetFlag(MKNC_FLG_DEEP) ? 
		pNCMake->GetDbl(MKNC_DBL_DEEP) : pNCMake->GetDbl(MKNC_DBL_ZCUT);
}

CString CLayerData::FormatLayerInfo(LPCTSTR lpszBase)
{
	CString	strBuf, strInitFile, strNCFile;
	TCHAR	szFile[_MAX_PATH];

	// 同じﾙｰﾄﾊﾟｽか?
	if ( ::PathIsSameRoot(lpszBase, m_strInitFile) )	// Shlwapi.h
		strInitFile = ::PathRelativePathTo(szFile, lpszBase, FILE_ATTRIBUTE_NORMAL,
						m_strInitFile, FILE_ATTRIBUTE_NORMAL) ? szFile : m_strInitFile;
	else
		strInitFile = m_strInitFile;
	if ( ::PathIsSameRoot(lpszBase, m_strNCFile) )
		strNCFile = ::PathRelativePathTo(szFile, lpszBase, FILE_ATTRIBUTE_NORMAL,
						m_strNCFile, FILE_ATTRIBUTE_NORMAL) ? szFile : m_strNCFile;
	else
		strNCFile = m_strNCFile;

	strBuf.Format("%s, %d, %s, %.3f, %d, %d, %s\n", m_strLayer,
		m_bCutTarget ? 1 : 0, strInitFile,
		m_dZCut, m_bDrillZ ? 1 : 0, 
		m_bPartOut ? 1 : 0, strNCFile);
	return strBuf;
}

void CLayerData::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {
		ar << m_strLayer << m_nType << m_nDataCnt <<
			m_nListNo << m_bView << m_bCutTarget;
	}
	else {
		ar >> m_strLayer >> m_nType >> m_nDataCnt >>
			m_nListNo >> m_bView >> m_bCutTarget;
	}
}

//////////////////////////////////////////////////////////////////////
// CLayerMap クラス
//////////////////////////////////////////////////////////////////////

CLayerMap::~CLayerMap()
{
	CLayerData*	pLayer;
	CString		strLayer;
	for ( POSITION pos = GetStartPosition(); pos; ) {
		GetNextAssoc(pos, strLayer, pLayer);
		delete	pLayer;
	}
	RemoveAll();
}

void CLayerMap::InitialCutFlg(BOOL bRead)
{
	CLayerData* pLayer;
	CString	strLayer;
	for ( POSITION pos = GetStartPosition(); pos; ) {
		GetNextAssoc(pos, strLayer, pLayer);
		// 現在の表示状況で対象ﾌﾗｸﾞを置換
		if ( pLayer->IsCutType() ) {
			if ( !bRead )	// ﾚｲﾔ情報が読み込まれていたら置換しない
				pLayer->m_bCutTarget = pLayer->m_bView;
		}
		else
			pLayer->m_bCutTarget = pLayer->m_bView;
	}
}

CString CLayerMap::CheckDuplexFile(const CString& strOrgFile)
{
	POSITION	pos1, pos2;
	CLayerData*	pLayer;
	CLayerData*	pDataCmp;
	CString		strLayer, strFile;

	// ｵﾘｼﾞﾅﾙﾌｧｲﾙとの重複ﾁｪｯｸと個別出力同士の重複ﾁｪｯｸ
	for ( pos1 = GetStartPosition(); pos1; ) {
		GetNextAssoc(pos1, strLayer, pLayer);
		if ( !pLayer->m_bCutTarget || !pLayer->m_bPartOut )
			continue;
		// ｵﾘｼﾞﾅﾙﾌｧｲﾙとの重複ﾁｪｯｸ
		strFile = pLayer->m_strNCFile;
		if ( strOrgFile.CompareNoCase(strFile) == 0 ) {
			AfxMessageBox(IDS_ERR_OVERLAPPINGFILE, MB_OK|MB_ICONEXCLAMATION);
			return strLayer;
		}
		// 個別出力同士の重複ﾁｪｯｸ
		for ( pos2 = pos1; pos2; ) {
			GetNextAssoc(pos2, strLayer, pDataCmp);
			if ( !pDataCmp->m_bCutTarget || !pDataCmp->m_bPartOut )
				continue;
			if ( strFile.CompareNoCase(pDataCmp->m_strNCFile) == 0 ) {
				AfxMessageBox(IDS_ERR_OVERLAPPINGFILE, MB_OK|MB_ICONEXCLAMATION);
				return strLayer;
			}
		}
	}

	return CString();
}

BOOL CLayerMap::ReadLayerMap(LPCTSTR lpszFile, CNCMakeOption* pNCMake/*=NULL*/)
{
	extern	LPCTSTR		gg_szComma;		// StdAfx.cpp
	// 命令を分割
	static	STRING_CUTTER_EX	ctOrder(gg_szComma);

	CString	strBuf, strPiece, strBuf_Upper;
	TCHAR	szCurrent[_MAX_PATH], szFile[_MAX_PATH];
	int		i;
	BOOL	bResult = TRUE;

	CLayerStringMap	strInitMap;	// 条件ﾌｧｲﾙとﾚｲﾔ名の関係ﾏｯﾌﾟ
	CLayerData*	pLayer;
	CLayerData*	pInitMap;

	// ｶﾚﾝﾄﾃﾞｨﾚｸﾄﾘを lpszFile に -> PathSearchAndQualify()
	::GetCurrentDirectory(_MAX_PATH, szCurrent);
	::Path_Name_From_FullPath(lpszFile, strBuf/*strPath*/, strPiece/*strName*/);
	::SetCurrentDirectory(strBuf);

	try {
		CStdioFile	fp(lpszFile,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		// 読み込みﾙｰﾌﾟ
		while ( fp.ReadString(strBuf) ) {
			strBuf.TrimLeft();
			if ( ::NC_IsNullLine(strBuf) )
				continue;
			ctOrder.Set( strBuf );
			// 命令解析ﾙｰﾌﾟ
			for ( i=0; i<LAYERTOINITORDER && ctOrder.GiveAPieceEx(strPiece); i++ ) {
				strPiece.TrimLeft();	strPiece.TrimRight();
				if ( i > 0 ) {
					strBuf = strPiece.Mid(1);
					strBuf.TrimLeft();	strBuf.TrimRight();
				}
				switch ( i ) {
				case 0:		// ﾚｲﾔ名
					// CLayerMap に登録されている切削ﾚｲﾔ名だけを対象とする
					// pLayer にも CLayerData* 代入を兼ねる
					if ( strPiece.GetLength() <= 0 || !Lookup(strPiece, pLayer) || !pLayer->IsCutType() )
						i = LAYERTOINITORDER;	// for文break
					break;
				case 1:		// 切削対象ﾌﾗｸﾞ
					pLayer->m_bCutTarget = atoi(strBuf) ? TRUE : FALSE;
					break;
				case 2:		// 切削条件ﾌｧｲﾙ
					if ( !strBuf.IsEmpty() ) {
						strBuf_Upper = strBuf;
						strBuf_Upper.MakeUpper();
						// 相対ﾊﾟｽなら絶対ﾊﾟｽにに変換
						if ( ::PathIsRelative(strBuf) &&	// Shlwapi.h
									::PathSearchAndQualify(strBuf, szFile, _MAX_PATH) ) {
							strBuf_Upper = strBuf = szFile;
							strBuf_Upper.MakeUpper();
						}
						// 条件を読むか読まないか
						if ( strInitMap.Lookup(strBuf_Upper, pInitMap) )
							pLayer->SetInitFile(strBuf, pNCMake, pInitMap);
						else {
							pLayer->SetInitFile(strBuf, pNCMake);
							strInitMap.SetAt(strBuf_Upper, pLayer);
						}
					}
					else
						pLayer->m_strInitFile.Empty();
					break;
				case 3:		// 強制最深Z
					pLayer->m_dZCut = atof(strBuf);
					break;
				case 4:		// 強制最深Zを穴加工にも適用
					pLayer->m_bDrillZ = atoi(strBuf) ? TRUE : FALSE;
					break;
				case 5:		// 個別出力
					pLayer->m_bPartOut = atoi(strBuf) ? TRUE : FALSE;
					break;
				case 6:		// 個別出力ﾌｧｲﾙ名
					if ( !strBuf.IsEmpty() ) {
						// 相対ﾊﾟｽなら絶対ﾊﾟｽに
						if ( ::PathIsRelative(strBuf) &&
								::PathSearchAndQualify(strBuf, szFile, _MAX_PATH) )
							strBuf = szFile;
					}
					pLayer->m_strNCFile = strBuf;
					break;
				}
			}	// End of for()
#ifdef _DEBUG
			g_dbg.printf("Layer=%s Check=%d InitFile=%s",
				pLayer->m_strLayer, pLayer->m_bCutTarget, pLayer->m_strInitFile );
			g_dbg.printf("--- Z=%f Drill=%d",
				pLayer->m_dZCut, pLayer->m_bDrillZ );
			g_dbg.printf("--- PartOut=%d NCFile=%s",
				pLayer->m_bPartOut, pLayer->m_strNCFile);
#endif
		}	// End of while(ReadString)
	}
	catch (CFileException* e) {
		strBuf.Format(IDS_ERR_DXF2NCDINIT_EX, lpszFile);
		AfxMessageBox(strBuf, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}
	catch (CMemoryException* e) {	// CMapStringToOb::SetAt()
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}

	// ｶﾚﾝﾄﾃﾞｨﾚｸﾄﾘを元に戻す
	::SetCurrentDirectory(szCurrent);

	return bResult;
}

BOOL CLayerMap::SaveLayerMap(LPCTSTR lpszFile) const
{
	CSortArray<CObArray, CLayerData*>	obLayer;	// ﾚｲﾔ名で並べ替えて保存
	CLayerData*	pLayer;
	CString		strLayer;
	int			i;

	try {
		// 切削ﾚｲﾔのみ CSortArray にﾃﾞｰﾀ格納して並べ替え
		obLayer.SetSize(0, 64);
		for ( POSITION pos = GetStartPosition(); pos; ) {
			GetNextAssoc(pos, strLayer, pLayer);
			if ( pLayer->IsCutType() )
				obLayer.Add(pLayer);
		}
		obLayer.Sort(LayerCompareFunc);
		//
		CStdioFile	fp(lpszFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
		// ｺﾒﾝﾄ出力
		for ( i=0; i<SIZEOF(g_szLayerToInitComment); i++ )
			fp.WriteString(g_szLayerToInitComment[i]);
		fp.WriteString(g_szLayerToInitComment[0]);
		// ﾃﾞｰﾀ出力
		for ( i=0; i<obLayer.GetSize(); i++ )
			fp.WriteString( obLayer[i]->FormatLayerInfo(lpszFile) );
		fp.Close();
	}
	catch ( CMemoryException* e ) {
		CString	strMsg;
		strMsg.Format(IDS_ERR_DXF2NCDINIT_EX, lpszFile);
		AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}
	catch (	CFileException* e) {
		CString	strMsg;
		strMsg.Format(IDS_ERR_DXF2NCDINIT_EX, lpszFile);
		AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

int LayerCompareFunc(CLayerData* pFirst, CLayerData* pSecond)
{
	return pFirst->GetStrLayer().Compare( pSecond->GetStrLayer() );
}
