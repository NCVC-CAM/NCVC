// DXFdata.h: CDXFdata クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
class	CLayerData;
class	CDXFshape;

// 矢印の角度と長さ
#define	ARRAWANGLE		RAD(15.0f)
#define	ARRAWLENGTH		100

// DXF状態ﾌﾗｸﾞ
#define	DXFFLG_MAKE				0x0001
#define	DXFFLG_SEARCH			0x0002
#define	DXFFLG_EDGE				0x0004
#define	DXFFLG_CLRWORK			0x000f
#define	DXFFLG_POLYCHILD		0x0010
#define	DXFFLG_SELECT			0x0080
#define	DXFFLG_DIRECTIONFIX		0x0100
#define	DXFFLG_OFFSET_EXCLUDE	0x1000
// DXFPOLYLINEﾌﾗｸﾞ
#define	DXFPOLY_SEQ				0x0001	// 進行方向 TRUE:Head->Tail, FALSE:Tail->Head
#define	DXFPOLY_SEQBAK			0x0002	// ↑のﾊﾞｯｸｱｯﾌﾟ
#define	DXFPOLY_CLOSED			0x0100
#define	DXFPOLY_INTERSEC		0x0200
#define	DXFPOLY_ELLIPSE			0x1000

// ｵﾌﾞｼﾞｪｸﾄ生成時の引数
// 各 pLayer は，DataOperation()で strLayer から CLayerDataｵﾌﾞｼﾞｪｸﾄを登録する
struct	DXFBASE
{
	CLayerData*	pLayer;
};
struct	DXFPARGV : public DXFBASE
{
	CPointF		c;			// 穴あけ位置or中心
};
typedef	DXFPARGV*		LPDXFPARGV;
typedef	const DXFPARGV*	LPCDXFPARGV;	

struct	DXFLARGV : public DXFBASE
{
	CPointF		s, e;		// 開始・終了点
};
typedef	DXFLARGV*		LPDXFLARGV;
typedef	const DXFLARGV*	LPCDXFLARGV;

struct	DXFCARGV : public DXFPARGV
{
	float		r;			// 半径
};
typedef	DXFCARGV*		LPDXFCARGV;
typedef	const DXFCARGV*	LPCDXFCARGV;

struct	DXFAARGV : public DXFCARGV
{
	float		sq, eq;		// 始点・終点角度
};
typedef	DXFAARGV*		LPDXFAARGV;
typedef	const DXFAARGV*	LPCDXFAARGV;

struct	DXFEARGV : public DXFPARGV
{
	CPointF		l;			// 長軸(中心からの相対)
	float		s;			// 短軸(倍率)
	float		sq, eq;		// 始点・終点角度
	BOOL		bRound;		// Default==TRUE(反時計回り)
};
typedef	DXFEARGV*		LPDXFEARGV;
typedef	const DXFEARGV*	LPCDXFEARGV;

struct	DXFTARGV : public DXFPARGV
{
	CString		strValue;	// 文字列
};
typedef	DXFTARGV*		LPDXFTARGV;
typedef	const DXFTARGV*	LPCDXFTARGV;

// ﾌﾞﾛｯｸの付加情報
#define	DXFBLFLG_X		0x0001
#define	DXFBLFLG_Y		0x0002
#define	DXFBLFLG_Z		0x0004
#define	DXFBLFLG_R		0x0008
struct	DXFBLOCK
{
	DWORD		dwBlockFlg;
	CPointF		ptOrg;			// 挿入ｵﾌｾｯﾄ
	float		dMagni[NCXYZ];	// 各軸の倍率
	float		dRound;			// 回転角度(度)
};
typedef	DXFBLOCK*		LPDXFBLOCK;
typedef	const DXFBLOCK*	LPCDXFBLOCK;

// CDXFpoint用動的関数呼び出し
class	CDXFpoint;
typedef float (*PFNORGDRILLTUNING)(const CDXFpoint*);

// CDXFcircleEx用ｽﾍﾟｼｬﾙﾀｲﾌﾟ
enum	ENDXFTYPE2
	{DXFORGDATA = 0, DXFSTADATA = 1};

/////////////////////////////////////////////////////////////////////////////
// ＣＡＭデータのヘッダークラス
class CCAMHead : public CObject
{
public:
	CCAMHead() {}
	virtual	void	Serialize(CArchive&);

	DECLARE_SERIAL(CCAMHead)
};

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータのベースクラス
class CDXFdata : public CObject
{
	ENDXFTYPE	m_enMakeType,	// 生成ﾀｲﾌﾟ(円->穴加工など)
				m_enType;		// ﾃﾞｰﾀﾀｲﾌﾟ : NCVCdefine.h
	DWORD		m_dwFlags,		// DXF状態ﾌﾗｸﾞ
				m_nSerialSeq;

protected:
	int			m_nPoint;		// m_ptTun, m_ptMake 確保数
	CPointF*	m_pt;			// 各ｵﾌﾞｼﾞｪｸﾄ固有座標
	CPointF*	m_ptTun;		// 原点調整後の座標
	CPointF*	m_ptMake;		// 小数第３位までの比較用(NC生成ﾃﾞｰﾀ)
	CRect3F		m_rcMax;		// ｵﾌﾞｼﾞｪｸﾄ矩形
	// CDXFpolyline だけは CDXFpolyline から更新する
	CLayerData*	m_pParentLayer;	// ﾃﾞｰﾀの属するﾚｲﾔ情報
	CDXFshape*	m_pParentMap;	// ﾃﾞｰﾀの属するﾏｯﾌﾟ情報
	//
	virtual	void	XRev(void);		// X軸の符号反転
	virtual	void	YRev(void);		// Y軸の符号反転
	virtual	void	SetMaxRect(void) = 0;

	void	OrgTuningBase(void);

	CDXFdata(ENDXFTYPE, CLayerData*, int, DWORD);

public:
	virtual	~CDXFdata();
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	static	BOOL		ms_fXRev;		// X軸反転かどうか
	static	BOOL		ms_fYRev;		// Y軸反転かどうか
	static	CPointF		ms_ptOrg;		// 切削原点
	static	CDXFdata*	ms_pData;		// １つ前の生成ﾃﾞｰﾀ
	static	DWORD		ms_nSerialSeq;	// ｼﾘｱﾗｲｽﾞｶｳﾝﾀ

	ENDXFTYPE	GetType(void) const;
	ENDXFTYPE	GetMakeType(void) const;
	DWORD		GetSerializeSeq(void) const;
	void		ChangeMakeType(ENDXFTYPE);	// 円ﾃﾞｰﾀによる穴加工など
	CLayerData*	GetParentLayer(void) const;
	CDXFshape*	GetParentMap(void) const;
	void		SetParentMap(CDXFshape*);
	DWORD		GetDxfFlg(void) const;
	void		SetDxfFlg(DWORD, BOOL = TRUE);
	BOOL		IsMakeFlg(void) const;
	BOOL		IsSearchFlg(void) const;
	BOOL		IsEdgeFlg(void) const;
	void		SetMakeFlg(void);
	void		SetSearchFlg(void);
	void		ClearMakeFlg(void);
	void		ClearSearchFlg(void);
	//
	CPen*		GetDrawPen(void) const;
	const CRect3F	GetMaxRect(void) const;
	//
	int			GetPointNumber(void) const;
	const CPointF	GetNativePoint(size_t) const;
	virtual	void	SetNativePoint(size_t, const CPointF&);		// virtual -> CDXFarc
	const CPointF	GetTunPoint(size_t) const;
	const CPointF	GetMakePoint(size_t) const;
	BOOL		IsMatchPoint(const CPointF&) const;
	BOOL		IsMakeMatchObject(const CDXFdata*);
	//	
	virtual	void	SwapMakePt(int);	// m_ptTun, m_ptMake の始点終点入れ替え
	virtual	void	SwapNativePt(void);	// 固有座標値の入れ替え
	virtual	void	RoundObjPoint(const CPointF&, float);
	//
	virtual	BOOL	IsMakeTarget(void) const = 0;
	virtual	BOOL	IsMakeMatchPoint(const CPointF&) = 0;
	virtual BOOL	IsStartEqEnd(void) const = 0;	// 始点終点が同じｵﾌﾞｼﾞｪｸﾄならTRUE
	virtual	BOOL	IsDirectionPoint(const CPointF&, const CPointF&);
			float	GetEdgeGap(const CDXFdata*, BOOL = TRUE);
	virtual	float	GetEdgeGap(const CPointF&,  BOOL = TRUE) = 0;
	virtual	const CPointF	GetStartCutterPoint(void) const = 0;// 加工開始位置
	virtual	const CPointF	GetStartMakePoint(void) const = 0;
	virtual	const CPointF	GetEndCutterPoint(void) const = 0;	// 加工終了位置(加工終点)
	virtual	const CPointF	GetEndMakePoint(void) const = 0;
	virtual	size_t	SetVectorPoint(CVPointF&, float = 0.0f) const = 0;	// from CDXFchain::IsPointInPolygon()
	virtual	void	SetVectorPoint(CVPointF&, size_t) const = 0;
	virtual	void	SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const = 0;
	virtual	float	GetLength(void) const = 0;
	//
	virtual	void	DrawTuning(float) = 0;
	virtual	void	Draw(CDC*) const = 0;
	virtual	float	OrgTuning(BOOL = TRUE) = 0;
	//
	virtual	float	GetSelectPointGap(const CPointF&) = 0;
	virtual	BOOL	GetDirectionArraw(const CPointF&, CPointF[][3]) = 0;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointF[], BOOL = TRUE) const = 0;
	virtual	int		GetIntersectionPoint(const CPointF&, const CPointF&, CPointF[], BOOL = TRUE) const = 0;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const = 0;
	virtual	int		CheckIntersectionCircle(const CPointF&, float) const = 0;

	virtual	void	Serialize(CArchive&);
	DECLARE_DYNAMIC(CDXFdata)
};

typedef CSortArray<CObArray, CDXFdata*>			CDXFsort;
typedef CTypedPtrListEx<CObList, CDXFdata*>		CDXFlist;
typedef	CTypedPtrArrayEx<CObArray, CDXFdata*>	CDXFarray;

/////////////////////////////////////////////////////////////////////////////
// Point クラス
class CDXFpoint : public CDXFdata  
{
protected:
	CPoint	m_ptDraw;	// 描画調整用(兼CDXFarc, CDXFellipse)
	CRect	m_rcDraw;	// 矩形描画(兼CDXFcircle)
	// CDXFtext からも参照
	virtual	void	SetMaxRect(void);
	//
	CDXFpoint();
	CDXFpoint(ENDXFTYPE, CLayerData*, int, DWORD);
public:
	CDXFpoint(LPCDXFPARGV, DWORD = 0);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFpoint(CLayerData*, const CDXFpoint*, LPCDXFBLOCK, DWORD = 0);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	const	CPoint	GetDrawPoint(void) const;

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointF&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	float	GetEdgeGap(const CPointF&, BOOL = TRUE);
	virtual	const CPointF	GetStartCutterPoint(void) const;
	virtual	const CPointF	GetStartMakePoint(void) const;
	virtual	const CPointF	GetEndCutterPoint(void) const;
	virtual	const CPointF	GetEndMakePoint(void) const;
	virtual	float	GetLength(void) const;
	virtual	size_t	SetVectorPoint(CVPointF&, float = 0.0f) const;
	virtual	void	SetVectorPoint(CVPointF&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const;

	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;
	virtual	float	OrgTuning(BOOL = TRUE);

	// 基準軸による加工
	static	PFNORGDRILLTUNING	ms_pfnOrgDrillTuning;
	static	float	OrgTuning_Seq(const CDXFpoint*);
	static	float	OrgTuning_XY(const CDXFpoint*);

	virtual	float	GetSelectPointGap(const CPointF&);
	virtual	BOOL	GetDirectionArraw(const CPointF&, CPointF[][3]);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointF[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointF&, const CPointF&, CPointF[], BOOL = TRUE) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointF&, float) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFpoint)
};

/////////////////////////////////////////////////////////////////////////////
// Line クラス
//		下位のCircle/Arcｸﾗｽ用にCDXFpointからの派生とする
//		CDXFarcｸﾗｽでは始点, 終点情報が必要
class CDXFline : public CDXFpoint
{
protected:
	CPoint	m_ptDrawS, m_ptDrawE;	// 描画用始点，終点
	virtual	void	SetMaxRect(void);
	//
	CDXFline();
	CDXFline(ENDXFTYPE, CLayerData*, int, DWORD);
public:
	CDXFline(LPCDXFLARGV, DWORD = 0);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFline(CLayerData*, const CDXFline*, LPCDXFBLOCK, DWORD = 0);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointF&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	float	GetEdgeGap(const CPointF&, BOOL = TRUE);
	virtual	const CPointF	GetStartCutterPoint(void) const;
	virtual	const CPointF	GetStartMakePoint(void) const;
	virtual	const CPointF	GetEndCutterPoint(void) const;
	virtual	const CPointF	GetEndMakePoint(void) const;
	virtual	float	GetLength(void) const;
	virtual	size_t	SetVectorPoint(CVPointF&, float = 0.0f) const;
	virtual	void	SetVectorPoint(CVPointF&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const;

	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;
	virtual	float	OrgTuning(BOOL = TRUE);

	virtual	float	GetSelectPointGap(const CPointF&);
	virtual	BOOL	GetDirectionArraw(const CPointF&, CPointF[][3]);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointF[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointF&, const CPointF&, CPointF[], BOOL = TRUE) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointF&, float) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFline)
};

/////////////////////////////////////////////////////////////////////////////
// Circle クラス
//		Lineｸﾗｽの派生とする必要はないが，
//		下位のArcｸﾗｽで始点終点をLineｸﾗｽと見立てるため．
//
//		座標ﾃﾞｰﾀの格納は
//			m_pt[]		-> 0, 90, 180, 270
//			m_ptTun[]	-> 0, 180, 90, 270
//		この方が都合がよい
//
//		m_bRound -> 本来はArc用．方向指示に対応するためCircleｸﾗｽのﾒﾝﾊﾞにした
//
class CDXFcircle : public CDXFline
{
	BOOL	m_bRoundFixed;	// 方向指示(回転方向の固定)

protected:
	int		m_nArrayExt;	// m_ptTun, m_ptMake の配列を指示(0,1:±X軸 or 2,3:±Y軸)
	float	m_r, m_rMake;	// 半径, 小数第３位までの半径 -> NC生成で使用
	CPointF	m_ct, m_ctTun;	// 中心座標
	BOOL	m_bRound;		// G2(FALSE)/G3(TRUE) DXFの基本は G3(反時計回り)

	virtual	void	SetCirclePoint(void);	// m_pt への代入
	virtual	void	XRev(void);
	virtual	void	YRev(void);

	// CDXFcircleEx からも参照
	virtual	void	SetMaxRect(void);
	// CDXFellipse からも参照
	void	GetQuarterPoint(const CPointF&, CPointF[]) const;

	// 円，円弧，共通処理
	void	SetEllipseArgv_Circle(LPCDXFBLOCK, LPDXFEARGV, float, float, BOOL);
	float	GetSelectPointGap_Circle(const CPointF&, float, float) const;
	BOOL	GetDirectionArraw_Circle(const float[], const CPointF[], CPointF[][3]);
	size_t	SetVectorPointSub(BOOL, float, float, float, const CPointF&, CVPointF&) const;

	CDXFcircle();
	CDXFcircle(ENDXFTYPE, CLayerData*, const CPointF&, float, BOOL, int, DWORD);
public:
	CDXFcircle(LPCDXFCARGV, DWORD = 0);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFcircle(CLayerData*, const CDXFcircle*, LPCDXFBLOCK, DWORD = 0);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	// ﾌﾞﾛｯｸｺﾋﾟｰ時の尺度で円が楕円になる
	void	SetEllipseArgv(LPCDXFBLOCK, LPDXFEARGV);

	BOOL	IsRoundFixed(void) const;
	float	GetR(void) const;
	float	GetMakeR(void) const;
	BOOL	GetRound(void) const;
	int		GetG(void) const;
	int		GetBaseAxis(void) const;
	float	GetIJK(int nType) const;
	const CPointF	GetCenter(void) const;
	const CPointF	GetMakeCenter(void) const;

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointF&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	BOOL	IsDirectionPoint(const CPointF&, const CPointF&);
			BOOL	IsUnderRadius(float) const;
	virtual	float	GetEdgeGap(const CPointF&, BOOL = TRUE);
	virtual	const CPointF	GetStartCutterPoint(void) const;
	virtual	const CPointF	GetStartMakePoint(void) const;
	virtual	const CPointF	GetEndCutterPoint(void) const;
	virtual	const CPointF	GetEndMakePoint(void) const;
	virtual	float	GetLength(void) const;
	virtual	size_t	SetVectorPoint(CVPointF&, float = 0.0f) const;
	virtual	void	SetVectorPoint(CVPointF&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const;

	virtual	void	SwapMakePt(int);
	virtual	void	SwapNativePt(void);
	virtual	void	RoundObjPoint(const CPointF&, float);
	virtual	BOOL	IsRangeAngle(const CPointF&) const;

	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;
	virtual	float	OrgTuning(BOOL = TRUE);

	virtual	float	GetSelectPointGap(const CPointF&);
	virtual	BOOL	GetDirectionArraw(const CPointF&, CPointF[][3]);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointF[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointF&, const CPointF&, CPointF[], BOOL = TRUE) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointF&, float) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFcircle)
};

/////////////////////////////////////////////////////////////////////////////
// CircleEx クラス
//		原点指示，加工開始位置の指示ﾃﾞｰﾀに使用
class CDXFcircleEx : public CDXFcircle
{
	ENDXFTYPE2	m_enType2;

protected:
	CPointF		m_ctMake;			// 小数第３位までの中心座標 -> NC生成で使用

	virtual	void	SetCirclePoint(void);
	virtual	void	XRev(void);
	virtual	void	YRev(void);

protected:
	CDXFcircleEx();
public:
	CDXFcircleEx(ENDXFTYPE2, CLayerData*, const CPointF&, float);

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointF&);
	virtual	float	GetEdgeGap(const CPointF&, BOOL = TRUE);
	virtual	const CPointF	GetStartCutterPoint(void) const;
	virtual	const CPointF	GetStartMakePoint(void) const;
	virtual	const CPointF	GetEndCutterPoint(void) const;
	virtual	const CPointF	GetEndMakePoint(void) const;

	virtual	void	Draw(CDC*) const;
	virtual	float	OrgTuning(BOOL = TRUE);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFcircleEx)
};

/////////////////////////////////////////////////////////////////////////////
// Arc クラス
//		本来なら
//		class CDXFarc : public CDXFline, public CDXFcircle
//		とすべきだが，多重継承による不具合のため，
//		CDXFcircle を CDXFline の派生にして回避した
class CDXFarc : public CDXFcircle
{
	void	SetRsign(void);

protected:
	BOOL	m_bRoundOrig;	// 生成中に向きが変わる可能性があるのでﾊﾞｯｸｱｯﾌﾟ
	float	m_sq, m_eq,		// 開始・終了角度(ﾗｼﾞｱﾝ単位)
			m_sqDraw, m_eqDraw,	m_rDraw;	// 描画用(Swapしない)

	void	AngleTuning(void);
	void	SwapRound(void);
	void	SwapAngle(void);

	virtual	void	XRev(void);
	virtual	void	YRev(void);
	virtual	void	SetMaxRect(void);

protected:
	CDXFarc();
	CDXFarc(ENDXFTYPE, CLayerData*, const CPointF&, float, float, float, BOOL, int, DWORD);
public:
	CDXFarc(LPCDXFAARGV, DWORD = 0);
	CDXFarc(LPCDXFAARGV, BOOL, const CPointF&, const CPointF&, DWORD = 0);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFarc(CLayerData*, const CDXFarc*, LPCDXFBLOCK, DWORD = 0);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	// ﾌﾞﾛｯｸｺﾋﾟｰ時の尺度で円弧が楕円弧になる
	void	SetEllipseArgv(LPCDXFBLOCK, LPDXFEARGV);

	BOOL	GetRoundOrig(void) const;
	float	GetStartAngle(void) const;
	float	GetEndAngle(void) const;

	virtual	void	SetNativePoint(size_t, const CPointF&);		// 角度の更新を含む

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointF&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	float	GetEdgeGap(const CPointF&, BOOL = TRUE);
	virtual	const CPointF	GetStartCutterPoint(void) const;
	virtual	const CPointF	GetStartMakePoint(void) const;
	virtual	const CPointF	GetEndCutterPoint(void) const;
	virtual	const CPointF	GetEndMakePoint(void) const;
	virtual	float	GetLength(void) const;
	virtual	size_t	SetVectorPoint(CVPointF&, float = 0.0f) const;
	virtual	void	SetVectorPoint(CVPointF&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const;

	virtual	void	SwapMakePt(int);
	virtual	void	SwapNativePt(void);
	virtual	void	RoundObjPoint(const CPointF&, float);
	virtual	BOOL	IsRangeAngle(const CPointF&) const;

	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;
	virtual	float	OrgTuning(BOOL = TRUE);

	virtual	float	GetSelectPointGap(const CPointF&);
	virtual	BOOL	GetDirectionArraw(const CPointF&, CPointF[][3]);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointF[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointF&, const CPointF&, CPointF[], BOOL = TRUE) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointF&, float) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFarc)
};

/////////////////////////////////////////////////////////////////////////////
// Ellipse クラス
//		楕円の矩形計算は，計算が煩雑なので
//		長軸長さを半径と見なし CDXFarc と同一にする
class CDXFellipse : public CDXFarc
{
	CPointF	m_ptLong;	// 長軸(中心からの相対)
	float	m_dShort,	// 短軸(倍率)
			m_dLongLength, m_dDrawLongLength,
			m_lq, m_lqMake,		// 長軸角度(==楕円の傾き)
			m_lqMakeCos, m_lqMakeSin,	// 傾きの cos(), sin() 
			m_lqDrawCos, m_lqDrawSin;	// 描画や生成で使用頻度が高いので計算結果を保存
	BOOL	m_bArc;		// TRUE:楕円弧，FALSE:楕円

	void	Construct(void);
	void	EllipseCalc(void);
	void	SetEllipseTunPoint(void);
	void	XYRev(const CPointF&, const CPointF&);

protected:
	virtual	void	XRev(void);
	virtual	void	YRev(void);
	virtual	void	SetMaxRect(void);

	CDXFellipse();
public:
	CDXFellipse(LPCDXFEARGV, DWORD = 0);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFellipse(CLayerData*, const CDXFellipse*, LPCDXFBLOCK, DWORD = 0);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	const CPointF	GetLongPoint(void) const;
	float	GetShortMagni(void) const;
	float	GetLongLength(void) const;
	float	GetShortLength(void) const;
	float	GetLean(void) const;
	float	GetMakeLean(void) const;
	float	GetMakeLeanCos(void) const;
	float	GetMakeLeanSin(void) const;
	BOOL	IsArc(void) const;
			void	SetRoundFixed(BOOL);

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointF&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	BOOL	IsDirectionPoint(const CPointF&, const CPointF&);
			BOOL	IsLongEqShort(void) const;
	virtual	float	GetEdgeGap(const CPointF&, BOOL = TRUE);
	virtual	const CPointF	GetStartCutterPoint(void) const;
	virtual	const CPointF	GetStartMakePoint(void) const;
	virtual	const CPointF	GetEndCutterPoint(void) const;
	virtual	const CPointF	GetEndMakePoint(void) const;
	virtual	size_t	SetVectorPoint(CVPointF&, float = 0.0f) const;
	virtual	void	SetVectorPoint(CVPointF&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const;

	virtual	void	SwapMakePt(int);
	virtual	void	SwapNativePt(void);
	virtual	void	RoundObjPoint(const CPointF&, float);

	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;
	virtual	float	OrgTuning(BOOL = TRUE);

	virtual	float	GetSelectPointGap(const CPointF&);
	virtual	BOOL	GetDirectionArraw(const CPointF&, CPointF[][3]);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointF[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointF&, const CPointF&, CPointF[], BOOL = TRUE) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointF&, float) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFellipse)
};

/////////////////////////////////////////////////////////////////////////////
// PolyLine クラス
//		始点，終点の管理のため
//		CDXFline からの派生
class CDXFpolyline : public CDXFline
{
	int			m_nObjCnt[3];	// ﾎﾟﾘﾗｲﾝ要素に含まれる線[0]と円弧[1]，楕円[2]の数
	DWORD		m_dwPolyFlags;	// ﾎﾟﾘﾗｲﾝ描画ﾌﾗｸﾞ
	CDXFlist	m_ltVertex;		// 各頂点(CDXFpoint or CDXFarc or CDXFellipse 格納)
	POSITION	m_posSel;		// GetSelectPointGap() で一番近かったｵﾌﾞｼﾞｪｸﾄﾎﾟｼﾞｼｮﾝ
	CDXFline*	m_pArrawLine;	// 方向指示判定用

	void		CheckPolylineIntersection_SubLoop(const CPointF&, const CPointF&, POSITION);
	void		CheckPolylineIntersection_SubLoop(const CDXFarc*, POSITION);
	void		CheckPolylineIntersection_SubLoop(const CDXFellipse*, POSITION);

protected:
	virtual	void	XRev(void);
	virtual	void	YRev(void);
	virtual	void	SetMaxRect(void);

public:
	CDXFpolyline();		// 生成直後はﾚｲﾔ情報無し
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFpolyline(CLayerData*, const CDXFpolyline*, LPCDXFBLOCK, DWORD = 0);
	virtual ~CDXFpolyline();
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	void	SetPolyFlag(DWORD);
	DWORD	GetPolyFlag(void) const;
	INT_PTR	GetVertexCount(void) const;
	int		GetObjectCount(int) const;
	POSITION	GetFirstVertex(void) const;
	CDXFdata*	GetNextVertex(POSITION&) const;
	const	CPointF		GetFirstPoint(void) const;
	CDXFdata*	GetFirstObject(void) const;
	CDXFdata*	GetTailObject(void) const;
	BOOL	IsIntersection(void) const;

	BOOL	SetVertex(LPCDXFPARGV);
	BOOL	SetVertex(LPCDXFPARGV, float, const CPointF&);
	void	EndSeq(void);
	void	CheckPolylineIntersection(void);

	virtual	size_t	SetVectorPoint(CVPointF&, float = 0.0f) const;
	virtual	void	SetVectorPoint(CVPointF&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const;

	virtual	void	SwapMakePt(int);
	virtual	void	SwapNativePt(void);
	virtual	void	RoundObjPoint(const CPointF&, float);

	virtual	BOOL	IsMakeTarget(void) const;
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	BOOL	IsDirectionPoint(const CPointF&, const CPointF&);
	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;
	virtual	float	OrgTuning(BOOL = TRUE);

	virtual	float	GetSelectPointGap(const CPointF&);
	virtual	BOOL	GetDirectionArraw(const CPointF&, CPointF[][3]);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointF[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointF&, const CPointF&, CPointF[], BOOL = TRUE) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointF&, float) const;

	// Polylineの特殊処理(from ReadDXF.cpp)
	void	SetParentLayer(CLayerData*);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFpolyline)
};

/////////////////////////////////////////////////////////////////////////////
// コメント文字 クラス
class CDXFtext : public CDXFpoint
{
protected:
	CString	m_strValue;

	CDXFtext();
public:
	CDXFtext(LPCDXFTARGV lpText, DWORD = 0);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFtext(CLayerData*, const CDXFtext*, LPCDXFBLOCK, DWORD = 0);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	CString		GetStrValue(void) const;

	virtual	float	GetEdgeGap(const CPointF&, BOOL = TRUE);
	virtual	void	Draw(CDC*) const;
	virtual	float	OrgTuning(BOOL = TRUE);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFtext)
};

#include "DXFdata.inl"
