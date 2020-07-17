// NCDoc.h : CNCDoc クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "DocBase.h"
#include "NCdata.h"
#include "DXFMakeOption.h"
#include "MCOption.h"

enum NCCOMMENT {		// g_szNCcomment[]
	ENDMILL = 0, DRILL, TAP, REAMER, 
	WORKRECT, WORKCYLINDER,
	LATHEVIEW, WIREVIEW,
	TOOLPOS
};
#define	ENDMILL_S		g_szNCcomment[ENDMILL]
#define	DRILL_S			g_szNCcomment[DRILL]
#define	TAP_S			g_szNCcomment[TAP]
#define	REAMER_S		g_szNCcomment[REAMER]
#define	WORKRECT_S		g_szNCcomment[WORKRECT]
#define	WORKCYLINDER_S	g_szNCcomment[WORKCYLINDER]
#define	LATHEVIEW_S		g_szNCcomment[LATHEVIEW]
#define	WIREVIEW_S		g_szNCcomment[WIREVIEW]
#define	TOOLPOS_S		g_szNCcomment[TOOLPOS]

// CNCDoc::DataOperation() の操作方法
enum ENNCOPERATION {
	NCADD, NCINS, NCMOD
};

class CNCDoc : public CDocBase
{
	CWinThread*	m_pCutcalcThread;	// 切削時間計算ｽﾚｯﾄﾞのﾊﾝﾄﾞﾙ
	CString		m_strDXFFileName,	// DXF出力ﾌｧｲﾙ名
				m_strCurrentFile;	// 現在処理中のNCﾌｧｲﾙ名(FileInsert etc.)
	CRecentViewInfo*	m_pRecentViewInfo;		// ﾌｧｲﾙごとの描画情報
	//
	int			m_nWorkOrg;						// 使用中のﾜｰｸ座標
	CPoint3F	m_ptNcWorkOrg[WORKOFFSET+1],	// ﾜｰｸ座標系(G54～G59)とG92原点
				m_ptNcLocalOrg;					// ﾛｰｶﾙ座標系(G52)原点
	CNCblockArray	m_obBlock;		// ﾌｧｲﾙｲﾒｰｼﾞﾌﾞﾛｯｸﾃﾞｰﾀ
	CNCarray		m_obGdata;		// Gｺｰﾄﾞ描画ｵﾌﾞｼﾞｪｸﾄ
	CStringArray	m_obMacroFile;	// ﾏｸﾛ展開一時ﾌｧｲﾙ
	float		m_dMove[2],		// 移動距離, 切削移動距離
				m_dCutTime;		// 切削時間
	CRect3F		m_rcWork,		// ﾜｰｸ矩形(最大切削矩形兼OpenGLﾜｰｸ矩形用)
				m_rcWorkCo;		// ｺﾒﾝﾄ指示
	//
	void	SetMaxRect(const CNCdata* pData) {
		// 最大ｵﾌﾞｼﾞｪｸﾄ矩形ﾃﾞｰﾀｾｯﾄ
		m_rcMax  |= pData->GetMaxRect();
		m_rcWork |= pData->GetMaxCutRect();
	}

	// ﾄﾚｰｽ
	CCriticalSection	m_csTraceDraw;
	UINT	m_nTrace;		// ﾄﾚｰｽ実行状態
	INT_PTR	m_nTraceDraw;	// 次の描画ﾎﾟｲﾝﾄ
	INT_PTR	m_nTraceStart;	// 描画開始ﾎﾟｲﾝﾄ(ｶｰｿﾙ位置からﾄﾚｰｽ実行)

	void	MakeDXF(const CDXFMakeOption*);

	// 移動・切削長，時間計算ｽﾚｯﾄﾞ
	static	UINT CuttimeCalc_Thread(LPVOID);

	void	SerializeBlock(CArchive&, CNCblockArray&, DWORD);
	BOOL	SerializeAfterCheck(void);
	BOOL	ValidBlockCheck(void);
	BOOL	ValidDataCheck(void);

	void	ClearBlockData(void);
	void	DeleteMacroFile(void);

protected: // シリアライズ機能のみから作成します。
	CNCDoc();
	DECLARE_DYNCREATE(CNCDoc)

// アトリビュート
public:
	BOOL	IsDocMill(void) const {
		return !(IsDocFlag(NCDOC_WIRE)||IsDocFlag(NCDOC_LATHE));
	}
	CString	GetDXFFileName(void) const {
		return m_strDXFFileName;
	}
	CString	GetCurrentFileName(void) const {
		return m_strCurrentFile;
	}
	CRecentViewInfo*	GetRecentViewInfo(void) const {
		return m_pRecentViewInfo;
	}
	INT_PTR		GetNCBlockSize(void) const {
		return m_obBlock.GetSize();
	}
	CNCblock*	GetNCblock(INT_PTR n) {
		ASSERT(0<=n && n<GetNCBlockSize());
		return m_obBlock[n];
	}
	INT_PTR		GetNCsize(void) const {
		return m_obGdata.GetSize();
	}
	CNCdata*	GetNCdata(INT_PTR n) const {
		ASSERT(0<=n && n<GetNCsize());
		return m_obGdata[n];
	}
	float	GetMoveData(size_t a) const {
		ASSERT(0<=a && a<SIZEOF(m_dMove));
		return m_dMove[a];
	}
	CPoint3F	GetOffsetOrig(void) const {
		ASSERT(0<=m_nWorkOrg && m_nWorkOrg<SIZEOF(m_ptNcWorkOrg));
		return m_ptNcWorkOrg[m_nWorkOrg] + m_ptNcLocalOrg;
	}
	float	GetCutTime(void) const {
		return m_dCutTime;
	}

	void	GetWorkRectPP(int a, float []);	// from NCInfoView.cpp

	INT_PTR	SearchBlockRegex(boost::regex&, INT_PTR = 0, BOOL = FALSE);
	void	CheckBreakPoint(INT_PTR a) {	// ﾌﾞﾚｰｸﾎﾟｲﾝﾄの設定
		CNCblock*	pBlock = GetNCblock(a);
		pBlock->SetBlockFlag(pBlock->GetBlockFlag() ^ NCF_BREAK, FALSE);
	}
	BOOL	IsBreakPoint(INT_PTR a) {		// ﾌﾞﾚｰｸﾎﾟｲﾝﾄの状態
		return GetNCblock(a)->GetBlockFlag() & NCF_BREAK;
	}
	void	ClearBreakPoint(void);		// ﾌﾞﾚｰｸﾎﾟｲﾝﾄ全解除
	INT_PTR	GetTraceDraw(void) {
		m_csTraceDraw.Lock();
		INT_PTR	nTraceDraw = m_nTraceDraw;
		m_csTraceDraw.Unlock();
		return nTraceDraw;
	}
	UINT	GetTraceMode(void) const {
		return m_nTrace;
	}
	INT_PTR	GetTraceStart(void) const {
		return m_nTraceStart;
	}

	CRect3F	GetWorkRect(void) const {
		return m_rcWork;
	}
	CRect3F	GetWorkRectOrg(void) const {
		return m_rcWorkCo;
	}

// オペレーション
public:
	// ｶｽﾀﾑｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
	BOOL	RouteCmdToAllViews(CView*, UINT, int, void*, AFX_CMDHANDLERINFO*);

	void	SelectWorkOffset(int nWork) {
		ASSERT(nWork>=0 && nWork<WORKOFFSET);
		m_nWorkOrg = nWork;
	}
	CNCdata*	DataOperation(const CNCdata*, LPNCARGV, INT_PTR = -1, ENNCOPERATION = NCADD);
	void	StrOperation(LPCTSTR, INT_PTR = -1, ENNCOPERATION = NCADD);
	void	RemoveAt(INT_PTR, INT_PTR);
	void	RemoveStr(INT_PTR, INT_PTR);

	void	AllChangeFactor(ENNCDRAWVIEW, float) const;	// 拡大率の更新

	void	CreateCutcalcThread(void);		// 切削時間計算ｽﾚｯﾄﾞの生成
	void	WaitCalcThread(BOOL = FALSE);	// ｽﾚｯﾄﾞの終了

	// from TH_NCRead.cpp
	BOOL	SerializeInsertBlock(LPCTSTR, INT_PTR, DWORD = 0);	// ｻﾌﾞﾌﾟﾛ，ﾏｸﾛの挿入
	void	AddMacroFile(const CString&);	// ﾄﾞｷｭﾒﾝﾄ破棄後に消去する一時ﾌｧｲﾙ
	void	SetWorkRectComment(const CRect3F& rc, BOOL bUpdate = TRUE) {
		m_rcWorkCo = rc;	// ｺﾒﾝﾄで指定されたﾜｰｸ矩形
		if ( bUpdate ) {
			m_rcWorkCo.NormalizeRect();
			m_bDocFlg.set(NCDOC_COMMENTWORK);
		}
	}
	void	SetWorkCylinderComment(float d, float h, const CPoint3F& ptOffset) {
		m_bDocFlg.set(NCDOC_CYLINDER);
		// 外接四角形 -> m_rcWorkCo
		d /= 2.0;
		CRect3F	rc(-d, -d, d, d, h, 0);
		rc.OffsetRect(ptOffset);
		SetWorkRectComment(rc);
	}
	void	SetWorkLatheR(float r) {
		m_rcWorkCo.high = r;
		m_rcWorkCo.low  = 0;
		m_bDocFlg.set(NCDOC_COMMENTWORK_R);
	}
	void	SetWorkLatheZ(float z1, float z2) {
		m_rcWorkCo.left  = z1;
		m_rcWorkCo.right = z2;
		m_rcWorkCo.NormalizeRect();
		m_bDocFlg.set(NCDOC_COMMENTWORK_Z);
	}
	void	SetLatheViewMode(void);

	// from NCWorkDlg.cpp
	void	SetWorkRect(BOOL, const CRect3F&);
	void	SetWorkCylinder(BOOL, float, float, const CPoint3F&);
	void	SetCommentStr(const CString&);

	// from NCViewTab.cpp
	void	SetTraceMode(UINT id) {
		m_nTrace = id;
	}
	BOOL	IncrementTrace(INT_PTR&);	// ﾄﾚｰｽ実行の次のｺｰﾄﾞ検査
	BOOL	SetLineToTrace(BOOL, int);	// 行番号からﾄﾚｰｽ行を設定
	void	StartTrace(void) {
		m_csTraceDraw.Lock();
		m_nTraceDraw = 0;
		m_csTraceDraw.Unlock();
	}
	void	StopTrace(void) {
		m_csTraceDraw.Lock();
		m_nTraceDraw = GetNCsize();
		m_csTraceDraw.Unlock();
	}
	void	ResetTraceStart(void) {
		m_nTraceStart = 0;
	}

	// from NCListView.cpp
	void	InsertBlock(int, const CString&);

	// from ThumbnailDlg.cpp
	void	ReadThumbnail(LPCTSTR);

//オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CNCDoc)
	public:
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
	virtual void OnChangedViewList();

// インプリメンテーション
public:
	virtual ~CNCDoc();

// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCDoc)
	afx_msg void OnFileNCD2DXF();
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWorkRect(CCmdUI* pCmdUI);
	afx_msg void OnWorkRect();
	afx_msg void OnUpdateMaxRect(CCmdUI* pCmdUI);
	afx_msg void OnMaxRect();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
