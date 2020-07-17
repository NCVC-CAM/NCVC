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

// UAV_DXFADDSHAPE 引数
class	CLayerData;
class	CDXFshape;
typedef	struct	tagDXFADDSHAPE {
	CLayerData*	pLayer;
	CDXFshape*	pShape;
} DXFADDSHAPE, *LPDXFADDSHAPE;

// 現在の表示状況
typedef	struct	tagDXFVIEWINFO {
	CPoint		ptOrg;		// 論理座標原点
	double		dFactor;	// 拡大率
} DXFVIEWINFO, *LPDXFVIEWINFO;

// ﾌｧｲﾙ変更通知の監視ｽﾚｯﾄﾞ
typedef	struct	tagFNCNGTHREADPARAM {
	LPCTSTR		lpstrFileName;	// ﾌﾙﾊﾟｽﾌｧｲﾙ名
	HWND		hWndFrame;		// 変更通知の送信先
	HANDLE		hFinish;		// 終了通知ｲﾍﾞﾝﾄﾊﾝﾄﾞﾙ
} FNCNGTHREADPARAM, *LPFNCNGTHREADPARAM;

//	ﾌｧｲﾙ変更通知の監視ｽﾚｯﾄﾞ
UINT	FileChangeNotificationThread(LPVOID pParam);

/////////////////////////////////////////////////////////////////////////////
// CDocBase

template<size_t T> class CDocBase
{
	// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ
	CWinThread*	m_pFileChangeThread;
	HANDLE		m_hAddinThread;			// ｽﾚｯﾄﾞﾛｯｸﾊﾝﾄﾞﾙ
	CEvent		m_evFinish;				// 終了通知ｲﾍﾞﾝﾄ

protected:
	// 派生ｸﾗｽ用ﾄﾞｷｭﾒﾝﾄﾌﾗｸﾞ
	std::bitset<T>	m_bDocFlg;

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

protected:
	BOOL	OnOpenDocumentSP(LPCTSTR lpstrFileName, CFrameWnd* pWnd);
	void	OnCloseDocumentSP(void);
	BOOL	UpdateModifiedTitle(BOOL bModified, CString& strTitle);

public:
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
};

#include "DocBase.inl"
