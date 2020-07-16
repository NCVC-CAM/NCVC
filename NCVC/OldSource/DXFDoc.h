// DXFDoc.h : ヘッダー ファイル
//

#if !defined(AFX_DXFDOC_H__95D33FAC_974A_11D3_B0D5_004005691B12__INCLUDED_)
#define AFX_DXFDOC_H__95D33FAC_974A_11D3_B0D5_004005691B12__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DocBase.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
class	CDXFBlockData;

// CDXFDoc::DataOperation() の操作方法
enum	ENDXFOPERATION	{
	DXFADD, DXFINS, DXFMOD
};

// m_pCircle 指示
#define	DXFCIRCLEORG	0
#define	DXFCIRCLESTA	1

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc ドキュメント

class CDXFDoc : public CDocument, public CDocBase
{
	BOOL	m_bReady,		// NC生成可能かどうか(ｴﾗｰﾌﾗｸﾞ)
			m_bThread,		// ｽﾚｯﾄﾞ継続ﾌﾗｸﾞ
			m_bReload,		// 再読込ﾌﾗｸﾞ(from DXFSetup.cpp)
			m_bShape;		// 形状処理を行ったか
	UINT	m_nShapePattern;	// 形状処理ﾊﾟﾀｰﾝ

	CRect3D		m_rcMax;			// ﾄﾞｷｭﾒﾝﾄのｵﾌﾞｼﾞｪｸﾄ最大矩形
	CDXFcircleEx*	m_pCircle[2];	// 切削原点と加工開始位置の情報
	CPointD		m_ptOrgOrig;		// ﾌｧｲﾙから読み込んだｵﾘｼﾞﾅﾙ原点

	CString		m_strNCFileName;	// NC生成ﾌｧｲﾙ名
	int			m_nDataCnt[5];		// Point,Line,Circle,Arc,Ellipse ﾃﾞｰﾀｶｳﾝﾄ
	CDXFarray	m_obDXFArray,		// DXF切削ﾃﾞｰﾀ(CDXFdata)
				m_obStartArray,		// DXF加工開始位置指示ﾃﾞｰﾀ(CDXFline/CDXFtext/CDXFpolyline)
				m_obMoveArray;		// DXF移動指示ﾃﾞｰﾀ(〃)
	CTypedPtrArray<CObArray, CDXFtext*>
				m_obDXFTextArray,	// DXF切削ﾃﾞｰﾀ(CDXFtext only)
				m_obStartTextArray,	// DXF加工開始位置指示ﾃﾞｰﾀ(〃)
				m_obMoveTextArray,	// DXF移動指示ﾃﾞｰﾀ(〃)
				m_obCommentArray;	// ｺﾒﾝﾄ文字ﾃﾞｰﾀ(〃)
	CLayerMap	m_mpLayer;			// ﾚｲﾔ名管理ﾏｯﾌﾟ
	CLayerArray	m_obLayer;			// ﾚｲﾔ名管理ﾏｯﾌﾟのｼﾘｱﾗｲｽﾞ用一時格納ｴﾘｱ
	DXFVIEWINFO	m_dxfViewInfo;		// 表示状況(ｼﾘｱﾗｲｽﾞ用)

	// RemoveAt() と DataOperation()::DXFMOD の inlineｻﾌﾞ
	void	RemoveSub(CDXFdata* pData) {
		ENDXFTYPE	enType = pData->GetType();
		const CLayerData* pLayer = pData->GetLayerData();
		if ( pLayer )
			m_mpLayer.DelLayerMap(pLayer->GetStrLayer());
		delete	pData;
		if ( enType>=DXFPOINTDATA || enType<=DXFELLIPSEDATA )
			m_nDataCnt[enType]--;
	}

	// ｵﾌﾞｼﾞｪｸﾄ矩形
	void	SetMaxRect(const CDXFdata* pData) {
		m_rcMax |= pData->GetMaxRect();
	}

	// 原点補正の文字列を数値に変換
	BOOL	GetEditOrgPoint(LPCTSTR, CPointD&);

/*
	ｽﾚｯﾄﾞﾊﾝﾄﾞﾙによるWaitForSingleObject()の代わりに
	ｸﾘﾃｨｶﾙｾｸｼｮﾝを用いることで，ｽﾚｯﾄﾞﾊﾝﾄﾞﾙに対する終了通知(初期化)が必要なくなる
*/
	// CDXFCircleの穴加工対象ﾃﾞｰﾀを元に戻す
	CCriticalSection	m_csRestoreCircleType;	// ｽﾚｯﾄﾞﾊﾝﾄﾞﾙの代わり
	static	UINT	RestoreCircleTypeThread(LPVOID);

protected: // シリアライズ機能のみから作成します。
	CDXFDoc();
	DECLARE_DYNCREATE(CDXFDoc)

// アトリビュート
public:
	BOOL	IsReload(void) const {
		return m_bReload;
	}
	BOOL	IsShape(void) const {
		return m_bShape;
	}
	UINT	GetShapePattern(void) const {
		return m_nShapePattern;
	}
	void	SetShapePattern(UINT nShape) {
		m_nShapePattern = nShape;
	}
	CDXFcircleEx*	GetCircleObject(size_t a) {
		ASSERT(a>=0 && a<SIZEOF(m_pCircle));
		return m_pCircle[a];
	}
	int	GetDxfSize(void) const {
		return m_obDXFArray.GetSize();
	}
	int	GetDxfTextSize(void) const {
		return m_obDXFTextArray.GetSize();
	}
	int	GetDxfStartSize(void) const {
		return m_obStartArray.GetSize();
	}
	int	GetDxfStartTextSize(void) const {
		return m_obStartTextArray.GetSize();
	}
	int	GetDxfMoveSize(void) const {
		return m_obMoveArray.GetSize();
	}
	int	GetDxfMoveTextSize(void) const {
		return m_obMoveTextArray.GetSize();
	}
	int	GetDxfCommentSize(void) const {
		return m_obCommentArray.GetSize();
	}
	CDXFdata* GetDxfData(int n) {
		ASSERT(n>=0 && n<GetDxfSize());
		return m_obDXFArray[n];
	}
	CDXFtext* GetDxfTextData(int n) {
		ASSERT(n>=0 && n<GetDxfTextSize());
		return m_obDXFTextArray[n];
	}
	CDXFdata* GetDxfStartData(int n) {
		ASSERT(n>=0 && n<GetDxfStartSize());
		return m_obStartArray[n];
	}
	CDXFtext* GetDxfStartTextData(int n) {
		ASSERT(n>=0 && n<GetDxfStartTextSize());
		return m_obStartTextArray[n];
	}
	CDXFdata* GetDxfMoveData(int n) {
		ASSERT(n>=0 && n<GetDxfMoveSize());
		return m_obMoveArray[n];
	}
	CDXFtext* GetDxfMoveTextData(int n) {
		ASSERT(n>=0 && n<GetDxfMoveTextSize());
		return m_obMoveTextArray[n];
	}
	CDXFtext* GetDxfCommentData(int n) {
		ASSERT(n>=0 && n<GetDxfCommentSize());
		return m_obCommentArray[n];
	}
	int	GetDxfDataCnt(ENDXFTYPE enType) const {
		ASSERT(enType>=DXFPOINTDATA && enType<=DXFELLIPSEDATA);
		return m_nDataCnt[enType];
	}
	CString GetNCFileName(void) const {
		return m_strNCFileName;
	}
	CLayerMap*	GetLayerMap(void) {
		return &m_mpLayer;
	}
	CLayerData*	GetLayerData_FromArray(int a) {
		return a>=0 && a<m_obLayer.GetSize() ? m_obLayer[a] : NULL;
	}
	CRect3D	GetMaxRect(void) const {
		return m_rcMax;
	}
	const	LPDXFVIEWINFO	GetDxfViewInfo(void) const {
		return (const LPDXFVIEWINFO)&m_dxfViewInfo;
	}

// オペレーション
public:
	// ｶｽﾀﾑｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
	BOOL	RouteCmdToAllViews(CView*, UINT, int, void*, AFX_CMDHANDLERINFO*);
	void	SetReadyFlg(BOOL bReady) {
		m_bReady = bReady;
	}
	void	SetReload(BOOL bReload) {
		m_bReload = bReload;
	}
	// DXFｵﾌﾞｼﾞｪｸﾄの操作 -> m_obDXFArray
	void	DataOperation(LPDXFPARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFLARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFCARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFAARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFEARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFTARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFpoint*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFline*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFcircle*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFarc*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFellipse*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFtext*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFpolyline*, int = -1, ENDXFOPERATION = DXFADD);
	void	RemoveAt(int, int);
	void	RemoveAtText(int, int);
	// DXFｵﾌﾞｼﾞｪｸﾄの操作 -> m_obStartArray
	void	DataOperation_STR(LPDXFLARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_STR(LPDXFTARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_STR(CDXFline*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_STR(CDXFtext*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_STR(CDXFpolyline*, int = -1, ENDXFOPERATION = DXFADD);
	void	RemoveAt_STR(int, int);
	void	RemoveAtText_STR(int, int);
	// DXFｵﾌﾞｼﾞｪｸﾄの操作 -> m_obMoveArray
	void	DataOperation_MOV(LPDXFLARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_MOV(LPDXFTARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_MOV(CDXFline*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_MOV(CDXFtext*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_MOV(CDXFpolyline*, int = -1, ENDXFOPERATION = DXFADD);
	void	RemoveAt_MOV(int, int);
	void	RemoveAtText_MOV(int, int);
	// DXFｵﾌﾞｼﾞｪｸﾄの操作 -> m_obCommentArray
	void	DataOperation_COM(LPDXFTARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_COM(CDXFtext*, int = -1, ENDXFOPERATION = DXFADD);
	void	RemoveAt_COM(int, int);

	void	AllChangeFactor(double);	// 拡大率の更新

	CPointD	GetCutterOrigin(void) {
		if ( m_pCircle[DXFCIRCLEORG] )
			return m_pCircle[DXFCIRCLEORG]->GetCenter();
		else
			return CPointD(HUGE_VAL);
	}
	void	SetCutterOrigin(const CPointD&, double, BOOL = FALSE);
	double	GetCutterOrgR(void) {
		if ( m_pCircle[DXFCIRCLEORG] )
			return m_pCircle[DXFCIRCLEORG]->GetR();
		else
			return 0.0;
	}
	CPointD	GetStartOrigin(void) {
		if ( m_pCircle[DXFCIRCLESTA] )
			return m_pCircle[DXFCIRCLESTA]->GetCenter();
		else
			return CPointD(HUGE_VAL);
	}
	void	SetStartOrigin(const CPointD&, double, BOOL = FALSE);
	double	GetStartOrgR(void) {
		if ( m_pCircle[DXFCIRCLESTA] )
			return m_pCircle[DXFCIRCLESTA]->GetR();
		else
			return 0.0;
	}

	// ﾏｳｽｸﾘｯｸの位置と該当ｵﾌﾞｼﾞｪｸﾄの最小距離を返す
	double	GetSelectViewPointGap(const CPointD&, const CRectD&, CDXFdata**);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDXFDoc)
	public:
	virtual void Serialize(CArchive& ar);   // ドキュメント I/O に対してオーバーライドされます。
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
	//}}AFX_VIRTUAL
	// ﾌﾚｰﾑが２つあるのでｳｨﾝﾄﾞｳﾀｲﾄﾙの「:1」を防ぐ
	virtual void UpdateFrameCounts();

// インプリメンテーション
public:
	virtual ~CDXFDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
	// Serialize()後のﾃﾞｰﾀ詳細
	void	SerializeInfo(void);
#endif

	// メッセージ マップ関数の生成
protected:
	//{{AFX_MSG(CDXFDoc)
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
	afx_msg void OnEditOrigin();
	afx_msg void OnEditShape();
	afx_msg void OnEditAutoShape();
	afx_msg void OnUpdateEditShape(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditShaping(CCmdUI* pCmdUI);
	//}}AFX_MSG
	// NC生成ﾒﾆｭｰ
	afx_msg void OnUpdateFileDXF2NCD(CCmdUI* pCmdUI);
	afx_msg void OnFileDXF2NCD(UINT);

	DECLARE_MESSAGE_MAP()
};

// DXFﾃﾞｰﾀの読み込み from DXFDoc2.cpp
BOOL	ReadDXF(CDXFDoc*, LPCTSTR);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DXFDOC_H__95D33FAC_974A_11D3_B0D5_004005691B12__INCLUDED_)
