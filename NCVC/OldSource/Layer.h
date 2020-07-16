// Layer.h: CLayerData, CLayerMap クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LAYER_H__696BA943_2D27_4A34_97D7_633F16942430__INCLUDED_)
#define AFX_LAYER_H__696BA943_2D27_4A34_97D7_633F16942430__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NCVCdefine.h"
#include "DXFdata.h"
#include "DXFshape.h"
class	CNCMakeOption;

//////////////////////////////////////////////////////////////////////
// Layerﾃﾞｰﾀ
//////////////////////////////////////////////////////////////////////
class CLayerData : public CObject
{
friend	class	CLayerDlg;
friend	class	CLayerMap;
friend	class	CMakeNCDlgEx1;
friend	class	CMakeNCDlgEx2;
friend	class	CMakeNCDlgEx11;
friend	class	CMakeNCDlgEx21;

	CString	m_strLayer;		// ﾚｲﾔ名
	int		m_nType,		// ﾚｲﾔﾀｲﾌﾟ(DXFORGLAYER～DXFCOMLAYER)
			m_nDataCnt;		// ﾚｲﾔ名ごとのｶｳﾝﾄ
	BOOL	m_bView;		// 表示対象
	// 以下「複数ﾚｲﾔの一括処理」にて使用
	int		m_nListNo;		// 切削順
	DWORD	m_dwFlags;		// 状態ﾌﾗｸﾞ(複数ﾚｲﾔの個別出力処理でのｴﾗｰ判断)
	BOOL	m_bCutTarget,	// 切削対象
			m_bDrillZ,		// 最深Z座標を穴加工にも適用するか
			m_bPartOut;		// 個別出力ﾌﾗｸﾞ
	CString	m_strInitFile,	// 切削条件ﾌｧｲﾙ
			m_strNCFile;	// 個別出力ﾌｧｲﾙ名
	double	m_dInitZCut,	// 切削条件ﾌｧｲﾙに設定された最深Z座標(CMakeNCDlgEx1::表示)
			m_dZCut;		// 最深Z座標(CMakeNCDlgEx2::入力)
	// 以下ｼﾘｱﾗｲｽﾞにて使用
	int		m_nSerial;		// ｼﾘｱﾙ番号
	// 形状認識
	CDXFmap	m_mpDXFdata;	// ﾚｲﾔごとの座標ﾏｯﾌﾟ母体(連結ｵﾌﾞｼﾞｪｸﾄ検索用一時領域)
	CDXFmapArray	m_obChainMap;	// 連結集団
	HANDLE	m_hClearMap;	// 座標ﾏｯﾌﾟ母体消去ｽﾚｯﾄﾞのﾊﾝﾄﾞﾙ

	// ﾚｲﾔ名と条件ﾌｧｲﾙ，最深Z値の関係を保存(CMakeNCDlgEx[1|2])にて使用
	CString	FormatLayerInfo(LPCTSTR);	// from CLayerMap::SaveLayerMap()

	// 座標ﾏｯﾌﾟ母体消去ｽﾚｯﾄﾞ
	static	UINT	RemoveMasterMapThread(LPVOID);

protected:
	// m_mpDXFdata 初期状態で DXFMAPFLG_LAYER ﾌﾗｸﾞを立てておく
	CLayerData() : m_mpDXFdata(DXFMAPFLG_LAYER) {
		m_nType		= -1;
		m_nDataCnt	= 0;
		m_nListNo	= -1;
		m_dwFlags	= 0;
		m_bView = m_bCutTarget = m_bDrillZ = m_bPartOut = FALSE;
		m_dInitZCut	= m_dZCut = 0.0;
		m_hClearMap	= NULL;
	};
public:
	CLayerData(const CString& strLayer, int nType) : m_mpDXFdata(DXFMAPFLG_LAYER) {	// 標準ｺﾝｽﾄﾗｸﾀ
		m_strLayer	= strLayer;
		m_nType		= nType;
		m_nDataCnt	= 1;
		m_bView = m_bCutTarget = TRUE;
		m_nListNo	= -1;
		m_dwFlags	= 0;
		m_bDrillZ	= TRUE;
		m_bPartOut	= FALSE;
		m_dInitZCut	= m_dZCut = 0.0;
		m_hClearMap	= NULL;
	}
	CLayerData(const CLayerData* pLayer, BOOL bCut) {	// CMakeNCDlgEx[1|2][1]用ｺﾋﾟｰｺﾝｽﾄﾗｸﾀ
		// 必要なﾃﾞｰﾀだけｺﾋﾟｰ
		m_strLayer		= pLayer->m_strLayer;
		m_nType			= pLayer->m_nType;
		m_bCutTarget	= bCut;
		m_bDrillZ		= pLayer->m_bDrillZ;
		m_bPartOut		= pLayer->m_bPartOut;
		m_strInitFile	= pLayer->m_strInitFile;
		m_strNCFile		= pLayer->m_strNCFile;
		m_dInitZCut		= pLayer->m_dInitZCut;
		m_dZCut			= pLayer->m_dZCut;
		m_hClearMap	= NULL;
	}
	virtual	~CLayerData() {
		for ( int i=0; i<m_obChainMap.GetSize(); i++ )
			delete	m_obChainMap[i];
		if ( m_hClearMap ) {
			::WaitForSingleObject(m_hClearMap, INFINITE);
			::CloseHandle(m_hClearMap);
		}
	}

// ｱﾄﾘﾋﾞｭｰﾄ
	int		GetCount(void) const {
		return m_nDataCnt;
	}
	const	CString		GetStrLayer(void) const {
		return m_strLayer;
	}
	int		GetLayerType(void) const {
		return m_nType;
	}
	BOOL	IsCutType(void) const {
		return m_nType == DXFCAMLAYER;
	}
	BOOL	IsViewLayer(void) const {
		return m_bView;
	}
	BOOL	IsCutTarget(void) const {
		return m_bCutTarget;
	}
	BOOL	IsDrillZ(void) const {
		return m_bDrillZ;
	}
	BOOL	IsPartOut(void) const {
		return m_bPartOut;
	}

// ｵﾍﾟﾚｰｼｮﾝ
	int		Increment(void) {
		m_nDataCnt++;
		return GetCount();
	}
	int		Decrement(void) {
		m_nDataCnt--;
		return GetCount();
	}
	//
	int		GetListNo(void) const {
		return m_nListNo;
	}
	void	SetListNo(int a) {
		m_nListNo = a;
	}
	DWORD	GetLayerFlags(void) const {
		return m_dwFlags;
	}
	void	SetLayerFlags(DWORD dwFlags = 0) {
		m_dwFlags = dwFlags;
	}
	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
	void	SetInitFile(LPCTSTR, CNCMakeOption* = NULL, CLayerData* = NULL);
	CString	GetNCFile(void) const {
		return m_strNCFile;
	}
	void	SetNCFile(LPCTSTR lpNCFile) {
		m_strNCFile = lpNCFile;
	}
	double	GetZCut(void) const {
		return m_dZCut;
	}
	void	SetCutTargetFlg(BOOL bCut) {
		m_bCutTarget = bCut;
	}
	//
	void	SetSerial(int a) {
		m_nSerial = a;
	}
	int		GetSerial(void) const {
		return m_nSerial;
	}
	//
	CDXFmap*	GetMasterMap(void) {
		return &m_mpDXFdata;
	}
	void	SetPointMasterMap(CDXFdata* pData) {
		m_mpDXFdata.SetPointMap(pData);
	}
	CDXFmapArray*	GetChainMap(void) {
		return &m_obChainMap;
	}
	void	SetShapeSwitch_AllChainMap(BOOL);
	void	RemoveMasterMap(void);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CLayerData)
};

typedef	CTypedPtrArray<CObArray, CLayerData*>	CLayerArray;

//////////////////////////////////////////////////////////////////////
// LayerMap(Layerﾃﾞｰﾀ管理用)
//////////////////////////////////////////////////////////////////////
typedef	CTypedPtrMap<CMapStringToOb, CString, CLayerData*>	CLayerStringMap;
class CLayerMap : public CLayerStringMap
{
public:
	virtual ~CLayerMap();

// ｵﾍﾟﾚｰｼｮﾝ
	CLayerData*	AddLayerMap(const CString& strLayer, int nType = DXFCAMLAYER) {
		ASSERT( !strLayer.IsEmpty() );
		CLayerData* pLayer;
		if ( Lookup(strLayer, pLayer) )
			pLayer->Increment();
		else {
			pLayer = new CLayerData(strLayer, nType);
			SetAt(strLayer, pLayer);
		}
		ASSERT( pLayer );
		return pLayer;
	}
	void	DelLayerMap(const CString& strLayer) {
		CLayerData* pLayer;
		// 速度優先のためｶｳﾝﾄがｾﾞﾛになってもﾃﾞｰﾀは消さない．さほど変動のあるﾃﾞｰﾀではないﾊｽﾞ
		if ( Lookup(strLayer, pLayer) )
			pLayer->Decrement();
	}

	void	InitialCutFlg(BOOL);
	CString	CheckDuplexFile(const CString&);

	BOOL	ReadLayerMap(LPCTSTR, CNCMakeOption* = NULL);
	BOOL	SaveLayerMap(LPCTSTR) const;
};

#endif // !defined(AFX_LAYER_H__696BA943_2D27_4A34_97D7_633F16942430__INCLUDED_)
