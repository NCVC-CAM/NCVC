// DXFDoc.h : ヘッダー ファイル
//

#pragma once

#include "DocBase.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "NCVCdefine.h"

class	CDXFDoc;
class	CDXFView;

// 原点を示すﾃﾞﾌｫﾙﾄの半径
#define	ORGRADIUS		10.0

// 自動処理ﾃﾞｰﾀ受け渡し構造体
struct	AUTOWORKINGDATA
{
	int		nSelect;		// 機能選択(0:輪郭, 1:ｵﾌｾｯﾄ)
	double	dOffset;		// ｵﾌｾｯﾄ
	BOOL	bAcuteRound;	// 鋭角の外側を丸める
	int		nLoopCnt;		// 繰り返し数
	int		nScanLine;		// 走査線(0:なし, 1:X方向, 2:Y方向)
	BOOL	bCircleScroll;	// 円ﾃﾞｰﾀはｽｸﾛｰﾙ切削
	// 初期化
	AUTOWORKINGDATA() {
		nSelect	= 0;
		dOffset	= 1.0;
		bAcuteRound	= TRUE;
		nLoopCnt	= 1;
		nScanLine	= 0;
		bCircleScroll	= TRUE;
	}
};
typedef	AUTOWORKINGDATA*	LPAUTOWORKINGDATA;
// 自動形状処理ﾀｲﾌﾟ
enum {
	AUTOWORKING = 0,
	AUTORECALCWORKING,
	AUTOSTRICTOFFSET
};

// 結合のための情報
struct CADBINDINFO {
	BOOL		bTarget;
	CDXFDoc*	pDoc;
	CDXFView*	pView;
	CPointD		pt,			// 配置位置(論理座標)
				ptOffset;	// 子の描画原点
	// 初期化
	CADBINDINFO(CDXFDoc* ppDoc, CDXFView* ppView) {
		bTarget = TRUE;
		pDoc = ppDoc;
		pView = ppView;
	}
	CADBINDINFO(BOOL bTgt, CDXFDoc* ppDoc, CDXFView* ppView, const CPointD& ppt, const CPointD& pptOffset) {
		bTarget = bTgt;
		pDoc = ppDoc;
		pView = ppView;
		pt = ppt;
		ptOffset = pptOffset;
	}
	CADBINDINFO(CADBINDINFO* pInfo) {
		bTarget = pInfo->bTarget;
		pDoc = pInfo->pDoc;
		pView = pInfo->pView;
		pt = pInfo->pt;
		ptOffset = pInfo->ptOffset;
	}
};
typedef	CADBINDINFO*	LPCADBINDINFO;

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc ドキュメント

class CDXFDoc : public CDocBase
{
	UINT	m_nShapeProcessID;		// 形状加工指示ID
	AUTOWORKINGDATA	m_AutoWork;		// 自動輪郭処理ﾃﾞｰﾀ
	CDXFcircleEx*	m_pCircle;		// 切削原点
	CDXFline*		m_pLatheLine[2];// 旋盤用原点([0]:外径, [1]:端面)
	CDXFDoc*		m_pParentDoc;	// 親ﾄﾞｷｭﾒﾝﾄ(bind)
	boost::optional<CPointD>	m_ptOrgOrig;	// ﾌｧｲﾙから読み込んだｵﾘｼﾞﾅﾙ原点

	CString		m_strNCFileName;	// NC生成ﾌｧｲﾙ名
	int			m_nDataCnt[5],		// Point,Line,Circle,Arc,Ellipse ﾃﾞｰﾀｶｳﾝﾄ
				m_nLayerDataCnt[DXFLAYERSIZE-1];	// ﾚｲﾔ別のﾃﾞｰﾀｶｳﾝﾄ(ORIGIN除く)
	CLayerArray	m_obLayer;			// ﾚｲﾔ情報配列
	CLayerMap	m_mpLayer;			// ﾚｲﾔ名をｷｰにしたﾏｯﾌﾟ
	CSortArray<CPtrArray, LPCADBINDINFO>	m_bindInfo;	// 結合情報

	// DXFｵﾌﾞｼﾞｪｸﾄ delete
	void	RemoveData(CLayerData*, int);

	// ｵﾌﾞｼﾞｪｸﾄ矩形
	void	SetMaxRect(const CDXFdata* pData) {
		m_rcMax |= pData->GetMaxRect();
	}

	void	MakeDXF(const CString&);

	// 原点補正の文字列を数値に変換
	BOOL	GetEditOrgPoint(LPCTSTR, CPointD&);

//	---
//	ｽﾚｯﾄﾞﾊﾝﾄﾞﾙによるWaitForSingleObject()の代わりに
//	ｸﾘﾃｨｶﾙｾｸｼｮﾝを用いることで，ｽﾚｯﾄﾞﾊﾝﾄﾞﾙに対する終了通知(初期化)が必要なくなる
//	---
	// CDXFCircleの穴加工対象ﾃﾞｰﾀを元に戻す
	CCriticalSection	m_csRestoreCircleType;	// ｽﾚｯﾄﾞﾊﾝﾄﾞﾙの代わり
	static	UINT	RestoreCircleTypeThread(LPVOID);

protected: // シリアライズ機能のみから作成します。
	CDXFDoc();
	DECLARE_DYNCREATE(CDXFDoc)

// アトリビュート
public:
	UINT	GetShapeProcessID(void) const {
		return m_nShapeProcessID;
	}
	CDXFcircleEx*	GetCircleObject(void) const {
		return m_pCircle;
	}
	boost::optional<CPointD>	GetCutterOrigin(void) const {
		boost::optional<CPointD>	ptResult;
		if ( m_pCircle )
			ptResult = m_pCircle->GetCenter();
		return ptResult;
	}
	double	GetCutterOrgR(void) const {
		return m_pCircle ? m_pCircle->GetR() : 0.0;
	}
	CDXFline*		GetLatheLine(size_t n) const {
		ASSERT(0<=n && n<SIZEOF(m_pLatheLine));
		return m_pLatheLine[n];
	}
	int	GetDxfDataCnt(ENDXFTYPE enType) const {
		ASSERT(enType>=DXFPOINTDATA && enType<=DXFELLIPSEDATA);
		return m_nDataCnt[enType];
	}
	int	GetDxfLayerDataCnt(size_t n) const {
		ASSERT(n>=DXFCAMLAYER && n<=DXFCOMLAYER);
		return m_nLayerDataCnt[n-1];
	}
	INT_PTR	GetLayerCnt(void) const {
		return m_obLayer.GetSize();
	}
	int	GetCutLayerCnt(void) const {
		int	i = 0, nCnt = 0;
		for ( ; i<m_obLayer.GetSize(); i++ ) {
			if ( m_obLayer[i]->IsCutType() )
				nCnt++;
		}
		return nCnt;
	}
	CLayerData*	GetLayerData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetLayerCnt());
		return m_obLayer[n];
	}
	CLayerData*	GetLayerData(LPCTSTR lpszLayer) const {
		CLayerData*	pLayer = NULL;
		return m_mpLayer.Lookup(lpszLayer, pLayer) ? pLayer : NULL;
	}
	INT_PTR	GetBindInfoCnt(void) const {
		return m_bindInfo.GetSize();
	}
	LPCADBINDINFO GetBindInfoData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetBindInfoCnt());
		return m_bindInfo[n];
	}
	int		GetBindInfo_fromView(const CDXFView* pView) {
		for ( int i=0; i<m_bindInfo.GetSize(); i++ ) {
			if ( m_bindInfo[i]->pView == pView )
				return i;
		}
		return -1;
	}
	void	RemoveBindData(INT_PTR n);
	void	SetBindParentDoc(CDXFDoc* pDoc) {
		m_pParentDoc = pDoc;
	}
	CDXFDoc*	GetBindParentDoc(void) const {
		return m_pParentDoc;
	}
	CString GetNCFileName(void) const {
		return m_strNCFileName;
	}

// オペレーション
public:
	// ｶｽﾀﾑｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
	BOOL	RouteCmdToAllViews(CView*, UINT, int, void*, AFX_CMDHANDLERINFO*);
	// DXFｵﾌﾞｼﾞｪｸﾄの操作
	void	DataOperation(CDXFdata*, ENDXFOPERATION = DXFADD, int = -1);
	void	RemoveAt(LPCTSTR, int, int);
	void	RemoveAtText(LPCTSTR, int, int);
	// ﾚｲﾔ情報の操作
	CLayerData*	AddLayerMap(const CString&, int = DXFCAMLAYER);
	void	DelLayerMap(CLayerData*);
	CString	CheckDuplexFile(const CString&, const CLayerArray* = NULL);
	BOOL	ReadLayerMap(LPCTSTR);
	BOOL	SaveLayerMap(LPCTSTR, const CLayerArray* = NULL);
	void	UpdateLayerSequence(void);
	//
	void	AllChangeFactor(double);	// 拡大率の更新
	void	AllSetDxfFlg(DWORD, BOOL = TRUE);
	void	AllRoundObjPoint(double);
	//
	void	CreateCutterOrigin(const CPointD&, double = ORGRADIUS, BOOL = FALSE);
	void	CreateLatheLine(const CPointD&, const CPointD&);
	void	CreateLatheLine(const CDXFline*, LPCDXFBLOCK);
	// ﾏｳｽｸﾘｯｸの位置と該当ｵﾌﾞｼﾞｪｸﾄの最小距離を返す
	boost::tuple<CDXFshape*, CDXFdata*, double>	GetSelectObject(const CPointD&, const CRectD&);
	//
	void	AddBindInfo(LPCADBINDINFO);
	void	SortBindInfo(void);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDXFDoc)
	public:
	virtual void Serialize(CArchive& ar);   // ドキュメント I/O に対してオーバーライドされます。
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL
	virtual void UpdateFrameCounts();
	// 変更されたﾄﾞｷｭﾒﾝﾄが閉じられる前にﾌﾚｰﾑﾜｰｸが呼び出し
	virtual BOOL SaveModified();

// インプリメンテーション
public:
	virtual ~CDXFDoc();
#ifdef _DEBUG
	// Serialize()後のﾃﾞｰﾀ詳細
	void	DbgSerializeInfo(void);
#endif

	// メッセージ マップ関数の生成
protected:
	//{{AFX_MSG(CDXFDoc)
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnEditOrigin();
	afx_msg void OnEditShape();
	afx_msg void OnEditAutoShape();
	afx_msg void OnEditStrictOffset();
	afx_msg void OnUpdateEditShape(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditShaping(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnFileDXF2DXF();
	// NC生成ﾒﾆｭｰ
	afx_msg void OnUpdateFileDXF2NCD(CCmdUI* pCmdUI);
	afx_msg void OnFileDXF2NCD(UINT);
	// 形状加工処理
	afx_msg void OnUpdateShapePattern(CCmdUI* pCmdUI);
	afx_msg	void OnShapePattern(UINT);

	DECLARE_MESSAGE_MAP()
};

// DXFﾃﾞｰﾀの読み込み to ReadDXF.cpp
BOOL	ReadDXF(CDXFDoc*, LPCTSTR);
