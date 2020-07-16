// CustomClass.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "CustomClass.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

//////////////////////////////////////////////////////////////////////
//	CStringKeyIndex

CStringKeyIndex::CStringKeyIndex(int nSize, LPCTSTR pszElement[], BOOL bCaseUpper/*=TRUE*/)
{
	InitHashTable(::GetPrimeNumber(max(17, nSize)));
	CString	strElement;
	for ( int i=0; i<nSize; i++ ) {
		strElement = pszElement[i];
		if ( bCaseUpper )
			strElement.MakeUpper();
		SetAt(strElement, (LPVOID)i);
	}
	m_bCaseUpper = bCaseUpper;
}

int CStringKeyIndex::GetIndex(LPCTSTR pszKey)
{
	LPVOID	pIndex;
	CString	strKey(pszKey);
	if ( m_bCaseUpper )
		strKey.MakeUpper();
	return Lookup(strKey, pIndex) ? (int)pIndex : -1;
}
