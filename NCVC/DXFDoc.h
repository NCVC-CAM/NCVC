// DXFDoc.h : ヘッダー ファイル
//

#pragma once

#include "DocBase.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "NCVCdefine.h"

class	CDXFBlockData;

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
// 自動形状処理ﾀｲﾌﾟ
#define	AUTOWORKING			0
#define	AUTORECALCWORKING	1
#define	AUTOSTRICTOFFSET	2

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc ドキュメント

class CDXFDoc : public CDocument, public CDocBase
{
	BOOL	m_bReady,		// NC生成可能かどうか(ｴﾗｰﾌﾗｸﾞ)
			m_bThread,		// ｽﾚｯﾄﾞ継続ﾌﾗｸﾞ
			m_bReload,		// 再読込ﾌﾗｸﾞ(from DXFSetup.cpp)
			m_bShape;		// 形状処理を行ったか
	UINT	m_nShapeProcessID;	// 形状加工指示ID
	AUTOWORKINGDATA	m_AutoWork;	// 自動輪郭処理ﾃﾞｰﾀ
	CRect3D		m_rcMax;			// ﾄﾞｷｭﾒﾝﾄのｵﾌﾞｼﾞｪｸﾄ最大矩形
	CDXFcircleEx*	m_pCircle;		// 切削原点
	boost::optional<CPointD>	m_ptOrgOrig;	// ﾌｧｲﾙから読み込んだｵﾘｼﾞﾅﾙ原点

	CString		m_strNCFileName;	// NC生成ﾌｧｲﾙ名
	int			m_nDataCnt[5],		// Point,Line,Circle,Arc,Ellipse ﾃﾞｰﾀｶｳﾝﾄ
				m_nLayerDataCnt[DXFLAYERSIZE-1];	// ﾚｲﾔ別のﾃﾞｰﾀｶｳﾝﾄ(ORIGIN除く)
	CLayerArray	m_obLayer;			// ﾚｲﾔ情報配列
	CLayerMap	m_mpLayer;			// ﾚｲﾔ名をｷｰにしたﾏｯﾌﾟ

	// DXFｵﾌﾞｼﾞｪｸﾄ delete
	void	RemoveData(CLayerData*, int);

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
	UINT	GetShapeProcessID(void) const {
		return m_nShapeProcessID;
	}
	CDXFcircleEx*	GetCircleObject(void) {
		return m_pCircle;
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
	CLayerData*	GetLayerData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetLayerCnt());
		return m_obLayer[n];
	}
	CLayerData*	GetLayerData(LPCTSTR lpszLayer) const {
		CLayerData*	pLayer = NULL;
		return m_mpLayer.Lookup(lpszLayer, pLayer) ? pLayer : NULL;
	}
	CString GetNCFileName(void) const {
		return m_strNCFileName;
	}
	CRect3D	GetMaxRect(void) const {
		return m_rcMax;
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
	void	AllChangeFactor(double) const;	// 拡大率の更新

	boost::optional<CPointD>	GetCutterOrigin(void) {
		boost::optional<CPointD>	ptResult;
		if ( m_pCircle )
			ptResult = m_pCircle->GetCenter();
		return ptResult;
	}
	void	CreateCutterOrigin(const CPointD&, double, BOOL = FALSE);
	double	GetCutterOrgR(void) {
		return m_pCircle ? m_pCircle->GetR() : 0.0;
	}

	// ﾏｳｽｸﾘｯｸの位置と該当ｵﾌﾞｼﾞｪｸﾄの最小距離を返す
	boost::tuple<CDXFshape*, CDXFdata*, double>	GetSelectObject(const CPointD&, const CRectD&);

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
	// 更新ﾏｰｸ付与
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
	// 変更されたﾄﾞｷｭﾒﾝﾄが閉じられる前にﾌﾚｰﾑﾜｰｸが呼び出し
	virtual BOOL SaveModified();
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
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnEditOrigin();
	afx_msg void OnEditShape();
	afx_msg void OnEditAutoShape();
	afx_msg void OnEditStrictOffset();
	afx_msg void OnUpdateEditShape(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditShaping(CCmdUI* pCmdUI);
	//}}AFX_MSG
	// NC生成ﾒﾆｭｰ
	afx_msg void OnUpdateFileDXF2NCD(CCmdUI* pCmdUI);
	afx_msg void OnFileDXF2NCD(UINT);
	// 形状加工処理
	afx_msg void OnUpdateShapePattern(CCmdUI* pCmdUI);
	afx_msg	void OnShapePattern(UINT);

	DECLARE_MESSAGE_MAP()
};

// DXFﾃﾞｰﾀの読み込み from DXFDoc2.cpp
BOOL	ReadDXF(CDXFDoc*, LPCTSTR);
