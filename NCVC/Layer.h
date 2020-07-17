// Layer.h: CLayerData, CLayerMap クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "DXFdata.h"
#include "DXFshape.h"

// CLayerData ﾌﾗｸﾞ
enum LAYERFLG {
	LAYER_VIEW = 0,		// 表示対象
	LAYER_CUT_TARGET,	// 切削対象
	LAYER_DRILL_Z,		// 最深Z座標を穴加工にも適用するか
	LAYER_PART_OUT,		// 個別出力ﾌﾗｸﾞ
	LAYER_PART_ERROR,	// 個別出力でのｴﾗｰ
		LAYER_FLGNUM		// ﾌﾗｸﾞの数[5]
};

// CLayerData::DataOperation() の操作方法
enum	ENDXFOPERATION	{
	DXFADD, DXFINS, DXFMOD
};

class	CNCMakeMillOpt;

//////////////////////////////////////////////////////////////////////
// Layerﾃﾞｰﾀ
//////////////////////////////////////////////////////////////////////
class CLayerData : public CObject
{
friend	class	CLayerDlg;
friend	class	CMakeNCDlgEx2;
friend	class	CMakeNCDlgEx3;
friend	class	CMakeNCDlgEx11;
friend	class	CMakeNCDlgEx21;

	std::bitset<LAYER_FLGNUM>	m_bLayerFlg;	// CLayerDataﾌﾗｸﾞ
	CString	m_strLayer;		// ﾚｲﾔ名
	int		m_nType;		// ﾚｲﾔﾀｲﾌﾟ(DXFORGLAYER～DXFCOMLAYER)
	CDXFarray	m_obDXFArray;		// 切削ﾃﾞｰﾀ(CDXFdata)
	CTypedPtrArrayEx<CObArray, CDXFtext*>
				m_obDXFTextArray;	// 切削ﾃﾞｰﾀ(CDXFtext only)
	// 形状認識
	CShapeArray	m_obShapeArray;	// 連結集団
	CDXFshape*	m_pActiveShape;	// CDXFworkingｼﾘｱﾗｲｽﾞ参照用
	// 以下「複数ﾚｲﾔの一括処理」にて使用
	int		m_nListNo;		// 切削順
	CString	m_strInitFile,	// 切削条件ﾌｧｲﾙ
			m_strNCFile,	// 個別出力ﾌｧｲﾙ名
			m_strLayerComment,	// ﾚｲﾔごとのｺﾒﾝﾄ
			m_strLayerCode;		// ﾚｲﾔごとの特殊ｺｰﾄﾞ
	double	m_dInitZCut,	// 切削条件ﾌｧｲﾙに設定された最深Z座標(CMakeNCDlgEx::表示)
			m_dZCut;		// 最深Z座標(CMakeNCDlgEx2::入力)

protected:
	CLayerData();
public:
	CLayerData(const CString&, int);
	CLayerData(const CLayerData*, BOOL);
	virtual	~CLayerData();

// ｱﾄﾘﾋﾞｭｰﾄ
	BOOL	IsLayerFlag(LAYERFLG n) const {
		return m_bLayerFlg[n];
	}
	INT_PTR	GetDxfSize(void) const {
		return m_obDXFArray.GetSize();
	}
	INT_PTR	GetDxfTextSize(void) const {
		return m_obDXFTextArray.GetSize();
	}
	INT_PTR	GetShapeSize(void) const {
		return m_obShapeArray.GetSize();
	}
	CDXFdata* GetDxfData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetDxfSize());
		return m_obDXFArray[n];
	}
	CDXFtext* GetDxfTextData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetDxfTextSize());
		return m_obDXFTextArray[n];
	}
	CDXFshape*	GetShapeData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetShapeSize());
		return m_obShapeArray[n];
	}
	CDXFshape*	GetActiveShape(void) const {
		return m_pActiveShape;
	}
	void	SetActiveShape(CDXFshape* pShape) {
		m_pActiveShape = pShape;
	}
	//
	const	CString		GetStrLayer(void) const {
		return m_strLayer;
	}
	int		GetLayerType(void) const {
		ASSERT(m_nType>=DXFORGLAYER && m_nType<=DXFCOMLAYER);
		return m_nType;
	}
	BOOL	IsCutType(void) const {
		return m_nType == DXFCAMLAYER;
	}
	BOOL	IsMakeTarget(void) const {
		return IsCutType() && IsLayerFlag(LAYER_CUT_TARGET);
	}
	void	SetCutTargetFlg(BOOL bCut) {
		m_bLayerFlg.set(LAYER_CUT_TARGET, bCut);
	}
	void	SetCutTargetFlg_fromView(void) {
		m_bLayerFlg.set(LAYER_CUT_TARGET, m_bLayerFlg[LAYER_VIEW]);
	}
	int		GetLayerListNo(void) const {
		return m_nListNo;
	}
	void	SetLayerListNo(int n) {
		m_nListNo = n;
	}
	void	SetLayerPartFlag(BOOL bError = FALSE) {
		m_bLayerFlg.set(LAYER_PART_ERROR, bError);
	}
	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
	void	SetInitFile(LPCTSTR);
	CString	GetNCFile(void) const {
		return m_strNCFile;
	}
	void	SetNCFile(CString& strFile) {
		m_strNCFile = strFile;
	}
	double	GetZCut(void) const {
		return m_dZCut;
	}
	CString	GetLayerComment(void) const {
		return m_strLayerComment;
	}
	CString	GetLayerOutputCode(void) const {
		return m_strLayerCode;
	}

// ｵﾍﾟﾚｰｼｮﾝ
	CDXFdata*	DataOperation(CDXFdata*, ENDXFOPERATION, int);
	CDXFdata*	RemoveData(int nIndex) {
		CDXFdata*	pData = m_obDXFArray[nIndex];
		m_obDXFArray.RemoveAt(nIndex);
		return pData;
	}
	CDXFtext*	RemoveDataText(int nIndex) {
		CDXFtext*	pData = m_obDXFTextArray[nIndex];
		m_obDXFTextArray.RemoveAt(nIndex);
		return pData;
	}
	void	AddShape(CDXFshape* pShape) {
		m_obShapeArray.Add(pShape);
	}
	void	RemoveShape(CDXFshape* pShape) {
		for ( int i=0; i<m_obShapeArray.GetSize(); i++ ) {
			if ( pShape == m_obShapeArray[i] ) {
				delete	m_obShapeArray[i];
				m_obShapeArray.RemoveAt(i);
				break;
			}
		}
	}
	void	RemoveAllShape(void) {
		for ( int i=0; i<m_obShapeArray.GetSize(); i++ )
			delete	m_obShapeArray[i];
		m_obShapeArray.RemoveAll();
	}
	void	AscendingShapeSort(void);
	void	DescendingShapeSort(void);
	void	SerializeShapeSort(void);
	//
	void	AllChangeFactor(double) const;
	void	AllShapeClearSideFlg(void) const;
	void	DrawWorking(CDC*);
	// ﾚｲﾔ名と条件ﾌｧｲﾙ，最深Z値の関係情報
	void	SetLayerInfo(const CString&);	// from CDXFDoc::ReadLayerMap()
	CString	FormatLayerInfo(LPCTSTR);		// from CDXFDoc::SaveLayerMap()

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CLayerData)
};

typedef	CSortArray<CObArray, CLayerData*>					CLayerArray;
typedef	CTypedPtrMap<CMapStringToOb, CString, CLayerData*>	CLayerMap;
