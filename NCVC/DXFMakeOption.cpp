// DXFMakeOption.cpp: CDXFMakeOption クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFMakeOption.h"
#include "DXFOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// int型命令
static	LPCTSTR	g_szNOrder[] = {
	"LTypeO", "LTypeC", "LTypeM", "LColorO", "LColorC", "LColorM",
	"Plane", "Cycle"
};
static	const	int		g_dfNOrder[] = {
	1, 0, 2, 4, 6, 2,	// 破線, 実線, 点線, 青, 白, 緑
	0, 0				// XY平面, 固定ｻｲｸﾙ円出力
};

// double型命令
static	LPCTSTR	g_szDOrder[] = {
	"OrgLength", "CycleR"
};
static	const	double	g_dfDOrder[] = {
	10.0, 10.0
};

// BOOL型命令
static	LPCTSTR	g_szBOrder[] = {
	"OrgCircle", "OrgCross"
};
static	const	BOOL	g_dfBOrder[] = {
	TRUE, TRUE
};

// CString型命令
static	LPCTSTR	g_szSOrder[] = {
	"LayerO", "LayerC", "LayerM"
};
extern	LPCTSTR	g_szDefaultLayer[];		// from DXFOption.cpp
//	"ORIGIN", "CAM", "MOVE"

/////////////////////////////////////////////////////////////////////////////
// CDXFMakeOption クラスの構築/消滅

CDXFMakeOption::CDXFMakeOption()
{
	ASSERT( SIZEOF(g_szNOrder) == SIZEOF(g_dfNOrder) );
	ASSERT( SIZEOF(g_szNOrder) == SIZEOF(m_unNums) );
	ASSERT( SIZEOF(g_szDOrder) == SIZEOF(g_dfDOrder) );
	ASSERT( SIZEOF(g_szDOrder) == SIZEOF(m_udNums) );
	ASSERT( SIZEOF(g_szBOrder) == SIZEOF(g_dfBOrder) );
	ASSERT( SIZEOF(g_szBOrder) == SIZEOF(m_ubFlags) );
	ASSERT( SIZEOF(g_szSOrder) == SIZEOF(m_strOption) );

	// ﾚｼﾞｽﾄﾘから情報読み込み
	int		i;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_MAKEDXF)), strResult;

	// int型命令読み込み
	for ( i=0; i<SIZEOF(g_szNOrder); i++ )
		m_unNums[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szNOrder[i], g_dfNOrder[i]);
	// double型命令読み込み
	for ( i=0; i<SIZEOF(g_szDOrder); i++ ) {
		strResult = AfxGetApp()->GetProfileString(strRegKey, g_szDOrder[i]);
		m_udNums[i] = strResult.IsEmpty() ? g_dfDOrder[i] : atof(strResult);
	}
	// BOOL型命令読み込み
	for ( i=0; i<SIZEOF(g_szBOrder); i++ )
		m_ubFlags[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szBOrder[i], g_dfBOrder[i]) ?
				TRUE : FALSE;
	// CString型命令読み込み(DXFOptionからﾃﾞﾌｫﾙﾄ情報を初期化)
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	CString	strLayer[SIZEOF(g_szSOrder)];
	strLayer[MKDX_STR_ORIGIN]	= pOpt->GetReadLayer(DXFORGLAYER);
	strLayer[MKDX_STR_CAMLINE]	= pOpt->GetReadLayer(DXFCAMLAYER);
	if ( strLayer[MKDX_STR_CAMLINE].FindOneOf(".|*?+(){}[]^$-\\") >= 0 ) {
		// 正規表現のための特殊な記号が含まれるなら
		strLayer[MKDX_STR_CAMLINE].Empty();
	}
	strLayer[MKDX_STR_MOVE]		= pOpt->GetReadLayer(DXFMOVLAYER);
	for ( i=0; i<SIZEOF(g_szSOrder); i++ ) {
		if ( strLayer[i].IsEmpty() )
			strLayer[i] = g_szDefaultLayer[i];
		m_strOption[i] = AfxGetApp()->GetProfileString(strRegKey, g_szSOrder[i], strLayer[i]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

BOOL CDXFMakeOption::SaveDXFMakeOption(void)
{
	// ﾚｼﾞｽﾄﾘへの保存
	int		i;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_MAKEDXF)), strResult;

	// int型命令の保存
	for ( i=0; i<SIZEOF(g_szNOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, g_szNOrder[i], m_unNums[i]) )
			return FALSE;
	}
	// double型命令の保存
	for ( i=0; i<SIZEOF(g_szDOrder); i++ ) {
		strResult.Format(IDS_MAKENCD_FORMAT, m_udNums[i]);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, g_szDOrder[i], strResult) )
			return FALSE;
	}
	// BOOL型命令の保存
	for ( i=0; i<SIZEOF(g_szBOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, g_szBOrder[i], m_ubFlags[i]) )
			return FALSE;
	}
	// CString型命令の保存
	for ( i=0; i<SIZEOF(g_szSOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileString(strRegKey, g_szSOrder[i], m_strOption[i]) )
			return FALSE;
	}

	return TRUE;
}
