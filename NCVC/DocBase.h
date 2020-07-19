// DocBase.h : ヘッダー ファイル
//

#pragma once

// UpdateAllViews() LPARAM のﾀｲﾌﾟ
#define	UAV_STARTSTOPTRACE		-1
#define	UAV_TRACECURSOR			-2
#define	UAV_FILEINSERT			-3
#define	UAV_FILESAVE			-4
#define	UAV_DRAWWORKRECT		-5
#define	UAV_DRAWMAXRECT			-6
#define	UAV_DXFORGUPDATE		-7
#define	UAV_DXFSHAPEID			-8
#define	UAV_DXFSHAPEUPDATE		-9
#define	UAV_DXFAUTOWORKING		-10
#define	UAV_DXFAUTODELWORKING	-11
#define	UAV_DXFADDWORKING		-12
#define	UAV_DXFADDSHAPE			-13
#define	UAV_CHANGEFONT			-90
#define	UAV_ADDINREDRAW			-99

// CDXFDoc ﾌﾗｸﾞ
enum DXFDOCFLG {
	DXFDOC_READY = 0,	// NC生成可能かどうか(ｴﾗｰﾌﾗｸﾞ)
	DXFDOC_RELOAD,		// 再読込ﾌﾗｸﾞ(from DXFSetup.cpp)
	DXFDOC_THREAD,		// ｽﾚｯﾄﾞ継続ﾌﾗｸﾞ
	DXFDOC_BINDPARENT,	// 統合ﾓｰﾄﾞ
	DXFDOC_BIND,		// 統合ﾓｰﾄﾞ(子)
	DXFDOC_SHAPE,		// 形状処理を行ったか
	DXFDOC_LATHE,		// 旋盤用の原点(ﾜｰｸ径と端面)を読み込んだか
	DXFDOC_WIRE,		// ﾜｲﾔ加工機用の生成が可能かどうか
		DXFDOC_FLGNUM		// ﾌﾗｸﾞの数[8]
};

// CNCDoc ﾌﾗｸﾞ
enum NCDOCFLG {
	NCDOC_ERROR = 0,	// ﾄﾞｷｭﾒﾝﾄ読み込みｴﾗｰ
	NCDOC_CUTCALC,		// 切削時間計算ｽﾚｯﾄﾞ継続ﾌﾗｸﾞ
	NCDOC_REVISEING,	// 補正計算行うかどうか
	NCDOC_COMMENTWORK,	// ｺﾒﾝﾄでﾜｰｸ矩形が指示された
		NCDOC_COMMENTWORK_R,
		NCDOC_COMMENTWORK_Z,
		NCDOC_CYLINDER,		// ﾐﾙ加工の円柱ﾓｰﾄﾞ
		NCDOC_WORKFILE,		// 外部ﾌｧｲﾙの読み込み
	NCDOC_MAXRECT,		// 最大移動矩形の描画
	NCDOC_WRKRECT,		// ﾜｰｸ矩形の描画
	NCDOC_THUMBNAIL,	// ｻﾑﾈｲﾙ表示ﾓｰﾄﾞ
	NCDOC_LATHE,		// NC旋盤ﾓｰﾄﾞ
		NCDOC_LATHE_INSIDE,	// 中ぐり加工アリ
		NCDOC_LATHE_HOLE,	// ｺﾒﾝﾄ中空
	NCDOC_WIRE,			// ﾜｲﾔ加工ﾓｰﾄﾞ
	NCDOC_MC_CHANGE,	// ｺﾒﾝﾄで機械情報の変更
		NCDOC_FLGNUM		// ﾌﾗｸﾞの数[16]
};
#define	DOCFLG_NUM	NCDOC_FLGNUM	// 大きい方

// UAV_DXFADDSHAPE 引数
class	CLayerData;
class	CDXFshape;
struct DXFADDSHAPE
{
	CLayerData*	pLayer;
	CDXFshape*	pShape;
};
typedef	DXFADDSHAPE*	LPDXFADDSHAPE;

// 現在の表示状況
struct DXFVIEWINFO
{
	CPoint		ptOrg;		// 論理座標原点
	float		dFactor;	// 拡大率
};

// ﾌｧｲﾙ変更通知の監視ｽﾚｯﾄﾞ
struct FNCNGTHREADPARAM
{
	LPCTSTR		lpstrFileName;	// ﾌﾙﾊﾟｽﾌｧｲﾙ名
	HWND		hWndFrame;		// 変更通知の送信先
	HANDLE		hFinish;		// 終了通知ｲﾍﾞﾝﾄﾊﾝﾄﾞﾙ
};
typedef	FNCNGTHREADPARAM*	LPFNCNGTHREADPARAM;

//	ﾌｧｲﾙ変更通知の監視ｽﾚｯﾄﾞ
UINT	FileChangeNotificationThread(LPVOID pParam);

/////////////////////////////////////////////////////////////////////////////
// CDocBase

class CDocBase : public CDocument
{
	// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ
	CWinThread*	m_pFileChangeThread;
	HANDLE		m_hAddinThread;			// ｽﾚｯﾄﾞﾛｯｸﾊﾝﾄﾞﾙ
	CEvent		m_evFinish;				// 終了通知ｲﾍﾞﾝﾄ

protected:
	std::bitset<DOCFLG_NUM>	m_bDocFlg;	// 派生ｸﾗｽ用ﾄﾞｷｭﾒﾝﾄﾌﾗｸﾞ
	CRect3F			m_rcMax;	// ﾄﾞｷｭﾒﾝﾄのｵﾌﾞｼﾞｪｸﾄ最大矩形

	// ｱﾄﾞｲﾝｼﾘｱﾙ関数の保持
	PFNNCVCSERIALIZEFUNC	m_pfnSerialFunc;
	// ｱﾄﾞｲﾝ向けﾛｯｸﾊﾝﾄﾞﾙ
	BOOL	IsLockThread(void);

protected:
	CDocBase() {
		UnlockDocument();
		m_pFileChangeThread = NULL;
		m_pfnSerialFunc = NULL;
		m_bDocFlg.reset();
	}
#ifdef _DEBUG
	virtual void AssertValid() const {
		__super::AssertValid();
	}
	virtual void Dump(CDumpContext& dc) const {
		__super::Dump(dc);
	}
#endif

protected:
	BOOL	OnOpenDocumentSP(LPCTSTR lpstrFileName, CFrameWnd* pWnd);
	void	OnCloseDocumentSP(void);
	BOOL	UpdateModifiedTitle(BOOL bModified, CString& strTitle);

public:
	CRect3F	GetMaxRect(void) const {
		return m_rcMax;
	}
	BOOL	IsDocFlag(size_t n) const {
		ASSERT( n < m_bDocFlg.size() );
		return m_bDocFlg[n];
	}
	void	SetDocFlag(size_t n, BOOL val = TRUE) {
		ASSERT( n < m_bDocFlg.size() );
		m_bDocFlg.set(n, val);
	}
	void	LockDocument(HANDLE hThread) {
		m_hAddinThread = hThread;
	}
	void	UnlockDocument(void) {
		m_hAddinThread = NULL;
	}
	PFNNCVCSERIALIZEFUNC	GetSerializeFunc(void) {
		return m_pfnSerialFunc;
	}
	HANDLE	GetLockHandle(void) {
		return m_hAddinThread;
	}
	//
	virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
	// 更新ﾏｰｸ付与
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
};
