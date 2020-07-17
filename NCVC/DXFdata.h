// DXFdata.h: CDXFdata クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
class	CLayerData;
class	CDXFshape;

// 矢印の角度と長さ
#define	ARRAWANGLE		RAD(15.0)
#define	ARRAWLENGTH		100

// DXF状態ﾌﾗｸﾞ
#define	DXFFLG_MAKE				0x0001
#define	DXFFLG_SEARCH			0x0002
#define	DXFFLG_CLRWORK			0x000f
#define	DXFFLG_SELECT			0x0010
#define	DXFFLG_DIRECTIONFIX		0x0100
#define	DXFFLG_OFFSET_EXCLUDE	0x1000
// DXFPOLYLINEﾌﾗｸﾞ
#define	DXFPOLY_SEQ				0x0001
#define	DXFPOLY_SEQBAK			0x0002
#define	DXFPOLY_CLOSED			0x0100
#define	DXFPOLY_INTERSEC		0x0200

// ｵﾌﾞｼﾞｪｸﾄ生成時の引数
// 各 pLayer は，DataOperation()で strLayer から CLayerDataｵﾌﾞｼﾞｪｸﾄを登録する
typedef	struct	tagDXF_POINT {
	CLayerData*	pLayer;
	CPointD		c;			// 穴あけ位置
} DXFPARGV, *LPDXFPARGV;
#define	LPCDXFPARGV		const LPDXFPARGV

typedef	struct	tagDXF_LINE {
	CLayerData*	pLayer;
	CPointD		s, e;		// 開始・終了点
} DXFLARGV, *LPDXFLARGV;
#define	LPCDXFLARGV		const LPDXFLARGV

typedef	struct	tagDXF_CIRCLE {
	CLayerData*	pLayer;
	CPointD		c;			// 中心
	double		r;			// 半径
} DXFCARGV, *LPDXFCARGV;
#define	LPCDXFCARGV		const LPDXFCARGV

typedef	struct	tagDXF_ARC {
	CLayerData*	pLayer;
	CPointD		c;			// 中心
	double		r;			// 半径
	double		sq, eq;		// 始点・終点角度
} DXFAARGV, *LPDXFAARGV;
#define	LPCDXFAARGV		const LPDXFAARGV

typedef	struct	tagDXF_ELLIPSE {
	CLayerData*	pLayer;
	CPointD		c;			// 中心
	CPointD		l;			// 長軸(中心からの相対)
	double		s;			// 短軸(倍率)
	double		sq, eq;		// 始点・終点角度
	BOOL		bRound;		// Default==TRUE(反時計回り)
} DXFEARGV, *LPDXFEARGV;
#define	LPCDXFEARGV		const LPDXFEARGV

typedef	struct	tagDXF_TEXT {
	CLayerData*	pLayer;
	CPointD		c;			// 文字位置
	CString		strValue;	// 文字列
} DXFTARGV, *LPDXFTARGV;
#define	LPCDXFTARGV		const LPDXFTARGV

// ﾌﾞﾛｯｸの付加情報
#define	DXFBLFLG_X		0x0001
#define	DXFBLFLG_Y		0x0002
#define	DXFBLFLG_Z		0x0004
#define	DXFBLFLG_R		0x0008
typedef	struct	tagDXF_BLOCK {
	DWORD		dwBlockFlg;
	CPointD		ptOrg;			// 挿入ｵﾌｾｯﾄ
	double		dMagni[NCXYZ];	// 各軸の倍率
	double		dRound;			// 回転角度(度)
} DXFBLOCK, *LPDXFBLOCK;
#define	LPCDXFBLOCK		const LPDXFBLOCK

// CDXFpoint用動的関数呼び出し
class	CDXFpoint;
typedef double (*PFNORGDRILLTUNING)(const CDXFpoint*);

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
	CPointD*	m_pt;			// 各ｵﾌﾞｼﾞｪｸﾄ固有座標
	CPointD*	m_ptTun;		// 原点調整後の座標
	CPointD*	m_ptMake;		// 小数第３位までの比較用(NC生成ﾃﾞｰﾀ)
	CRect3D		m_rcMax;		// ｵﾌﾞｼﾞｪｸﾄ矩形
	// CDXFpolyline だけは CDXFpolyline から更新する
	CLayerData*	m_pParentLayer;	// ﾃﾞｰﾀの属するﾚｲﾔ情報
	CDXFshape*	m_pParentMap;	// ﾃﾞｰﾀの属するﾏｯﾌﾟ情報
	virtual	void	XRev(void);		// X軸の符号反転
	virtual	void	YRev(void);		// Y軸の符号反転

	void	OrgTuningBase(void);

	CDXFdata(ENDXFTYPE, CLayerData*, int);

public:
	virtual	~CDXFdata();
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	static	BOOL		ms_fXRev;		// X軸反転かどうか
	static	BOOL		ms_fYRev;		// Y軸反転かどうか
	static	CPointD		ms_ptOrg;		// 切削原点
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
	void		SetMakeFlg(void);
	void		SetSearchFlg(void);
	void		ClearMakeFlg(void);
	void		ClearSearchFlg(void);
	//
	CPen*		GetDrawPen(void) const;
	const CRect3D	GetMaxRect(void) const;
	//
	int			GetPointNumber(void) const;
	const CPointD	GetNativePoint(size_t) const;
	virtual	void	SetNativePoint(size_t, const CPointD&);		// virtual -> CDXFarc
	const CPointD	GetTunPoint(size_t) const;
	const CPointD	GetMakePoint(size_t) const;
	BOOL		IsMatchPoint(const CPointD&) const;
	BOOL		IsMakeMatchObject(const CDXFdata*);
	//	
	virtual	void	SwapMakePt(int);	// m_ptTun, m_ptMake の始点終点入れ替え
	virtual	void	SwapNativePt(void);	// 固有座標値の入れ替え
	// 各ｵﾌﾞｼﾞｪｸﾄにしか解らない独自の処理 -> 純粋仮想関数
	virtual	BOOL	IsMakeTarget(void) const = 0;
	virtual	BOOL	IsMakeMatchPoint(const CPointD&) = 0;
	virtual BOOL	IsStartEqEnd(void) const = 0;	// 始点終点が同じｵﾌﾞｼﾞｪｸﾄならTRUE
			double	GetEdgeGap(const CDXFdata*, BOOL = TRUE);
	virtual	double	GetEdgeGap(const CPointD&,  BOOL = TRUE) = 0;
	virtual	const CPointD	GetStartCutterPoint(void) const = 0;// 加工開始位置
	virtual	const CPointD	GetStartMakePoint(void) const = 0;
	virtual	const CPointD	GetEndCutterPoint(void) const = 0;	// 加工終了位置(加工終点)
	virtual	const CPointD	GetEndMakePoint(void) const = 0;
	virtual	size_t	SetVectorPoint(VECPOINTD&, double = 0.0) const = 0;	// from CDXFchain::IsPointInPolygon()
	virtual	void	SetVectorPoint(VECPOINTD&, size_t) const = 0;
	virtual	void	SetWireHeteroData(const CDXFdata*, VECPOINTD&, VECPOINTD&, double) const = 0;
	virtual	double	GetLength(void) const = 0;
	//
	virtual	void	DrawTuning(const double) = 0;
	virtual	void	Draw(CDC*) const = 0;
	virtual	double	OrgTuning(BOOL = TRUE) = 0;
	//
	virtual	double	GetSelectPointGap(const CPointD&) = 0;
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const = 0;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const = 0;
	virtual	int		GetIntersectionPoint(const CPointD&, const CPointD&, CPointD[], BOOL = TRUE) const = 0;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const = 0;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const = 0;

	virtual	void	Serialize(CArchive&);
	DECLARE_DYNAMIC(CDXFdata)
};

typedef CSortArray<CObArray, CDXFdata*>			CDXFsort;
typedef CTypedPtrList<CObList, CDXFdata*>		CDXFlist;
typedef	CTypedPtrArrayEx<CObArray, CDXFdata*>	CDXFarray;

/////////////////////////////////////////////////////////////////////////////
// Point クラス
class CDXFpoint : public CDXFdata  
{
protected:
	CPoint	m_ptDraw;	// 描画調整用(兼CDXFarc, CDXFellipse)
	CRect	m_rcDraw;	// 矩形描画(兼CDXFcircle)

	// CDXFtext からも参照
	void	SetMaxRect(void);

protected:
	CDXFpoint();
	CDXFpoint(ENDXFTYPE, CLayerData*, int);
public:
	CDXFpoint(LPCDXFPARGV);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFpoint(CLayerData*, const CDXFpoint*, LPCDXFBLOCK);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	const	CPoint	GetDrawPoint(void) const;

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	double	GetLength(void) const;
	virtual	size_t	SetVectorPoint(VECPOINTD&, double = 0.0) const;
	virtual	void	SetVectorPoint(VECPOINTD&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, VECPOINTD&, VECPOINTD&, double) const;

	virtual	void	DrawTuning(const double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	// 基準軸による加工
	static	PFNORGDRILLTUNING	ms_pfnOrgDrillTuning;
	static	double	OrgTuning_Seq(const CDXFpoint*);
	static	double	OrgTuning_XY(const CDXFpoint*);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointD&, const CPointD&, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFpoint)
};

/////////////////////////////////////////////////////////////////////////////
// Line クラス
//		下位のCircle/Arcｸﾗｽ用にCDXFpointからの派生とする
//		CDXFarcｸﾗｽでは始点, 終点情報が必要
class CDXFline : public CDXFpoint
{
	CPoint	m_ptDrawS, m_ptDrawE;	// 描画用始点，終点
	void	SetMaxRect(void);

protected:
	CDXFline();
	CDXFline(ENDXFTYPE, CLayerData*, int);
public:
	CDXFline(LPCDXFLARGV);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFline(CLayerData*, const CDXFline*, LPCDXFBLOCK);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	double	GetLength(void) const;
	virtual	size_t	SetVectorPoint(VECPOINTD&, double = 0.0) const;
	virtual	void	SetVectorPoint(VECPOINTD&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, VECPOINTD&, VECPOINTD&, double) const;

	virtual	void	DrawTuning(const double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointD&, const CPointD&, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;

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
	void	SetCirclePoint(void);	// m_pt への代入

protected:
	int		m_nArrayExt;	// m_ptTun, m_ptMake の配列を指示(0,1:±X軸 or 2,3:±Y軸)
	double	m_r, m_rMake;	// 半径, 小数第３位までの半径 -> NC生成で使用
	CPointD	m_ct, m_ctTun;	// 中心座標
	BOOL	m_bRound;		// G2(FALSE)/G3(TRUE) DXFの基本は G3(反時計回り)

	virtual	void	XRev(void);
	virtual	void	YRev(void);

	// CDXFcircleEx からも参照
	void	SetMaxRect(void);
	// CDXFellipse からも参照
	void	GetQuarterPoint(const CPointD&, CPointD[]) const;

	// 円，円弧，共通処理
	void	SetEllipseArgv_Circle(LPCDXFBLOCK, LPCDXFEARGV, double, double, BOOL);
	double	GetSelectPointGap_Circle(const CPointD&, double, double) const;
	BOOL	GetDirectionArraw_Circle(const double[], const CPointD[], CPointD[][3]) const;
	size_t	SetVectorPointSub(BOOL, double, double, double, const CPointD&, VECPOINTD&) const;

	CDXFcircle();
	CDXFcircle(ENDXFTYPE, CLayerData*, const CPointD&, double, BOOL, int);
public:
	CDXFcircle(LPCDXFCARGV);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFcircle(CLayerData*, const CDXFcircle*, LPCDXFBLOCK);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	// ﾌﾞﾛｯｸｺﾋﾟｰ時の尺度で円が楕円になる
	void	SetEllipseArgv(LPCDXFBLOCK, LPCDXFEARGV);

	BOOL	IsRoundFixed(void) const;
	double	GetR(void) const;
	double	GetMakeR(void) const;
	BOOL	GetRound(void) const;
	virtual	void	SetRoundFixed(const CPointD&, const CPointD&);	// CDXFellipse
	int		GetG(void) const;
	int		GetBaseAxis(void) const;
	double	GetIJK(int nType) const;
	const CPointD	GetCenter(void) const;
	const CPointD	GetMakeCenter(void) const;

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
			BOOL	IsUnderRadius(double) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	double	GetLength(void) const;
	virtual	size_t	SetVectorPoint(VECPOINTD&, double = 0.0) const;
	virtual	void	SetVectorPoint(VECPOINTD&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, VECPOINTD&, VECPOINTD&, double) const;

	virtual	void	SwapMakePt(int);
	virtual	void	SwapNativePt(void);
	virtual	BOOL	IsRangeAngle(const CPointD&) const;

	virtual	void	DrawTuning(const double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointD&, const CPointD&, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;

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
	CPointD		m_ctMake;			// 小数第３位までの中心座標 -> NC生成で使用

	virtual	void	XRev(void);
	virtual	void	YRev(void);

protected:
	CDXFcircleEx();
public:
	CDXFcircleEx(ENDXFTYPE2, CLayerData*, const CPointD&, double);

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointD&);
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;

	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

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
	void	SetMaxRect(void);

protected:
	BOOL	m_bRoundOrig;	// 生成中に向きが変わる可能性があるのでﾊﾞｯｸｱｯﾌﾟ
	double	m_sq, m_eq,		// 開始・終了角度(ﾗｼﾞｱﾝ単位)
			m_sqDraw, m_eqDraw,	m_rDraw;	// 描画用(Swapしない)

	void	AngleTuning(void);
	void	SwapRound(void);
	void	SwapAngle(void);

	virtual	void	XRev(void);
	virtual	void	YRev(void);

protected:
	CDXFarc();
	CDXFarc(ENDXFTYPE, CLayerData*, const CPointD&, double, double, double, BOOL, int);
public:
	CDXFarc(LPCDXFAARGV);
	// from CDXFpolyline::SetVertex()
	CDXFarc(LPCDXFAARGV, BOOL, const CPointD&, const CPointD&);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFarc(CLayerData*, const CDXFarc*, LPCDXFBLOCK);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	// ﾌﾞﾛｯｸｺﾋﾟｰ時の尺度で円弧が楕円弧になる
	void	SetEllipseArgv(LPCDXFBLOCK, LPCDXFEARGV);

	BOOL	GetRoundOrig(void) const;
	double	GetStartAngle(void) const;
	double	GetEndAngle(void) const;

	virtual	void	SetNativePoint(size_t, const CPointD&);		// 角度の更新を含む

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	double	GetLength(void) const;
	virtual	size_t	SetVectorPoint(VECPOINTD&, double = 0.0) const;
	virtual	void	SetVectorPoint(VECPOINTD&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, VECPOINTD&, VECPOINTD&, double) const;

	virtual	void	SwapMakePt(int);
	virtual	void	SwapNativePt(void);
	virtual	BOOL	IsRangeAngle(const CPointD&) const;

	virtual	void	DrawTuning(const double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointD&, const CPointD&, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFarc)
};

/////////////////////////////////////////////////////////////////////////////
// Ellipse クラス
//		楕円の矩形計算は，計算が煩雑なので
//		長軸長さを半径と見なし CDXFarc と同一にする
class CDXFellipse : public CDXFarc
{
	CPointD	m_ptLong;	// 長軸(中心からの相対)
	double	m_dShort,	// 短軸(倍率)
			m_dLongLength, m_dDrawLongLength;
	double	m_lq, m_lqMake,		// 長軸角度(==楕円の傾き)
			m_lqMakeCos, m_lqMakeSin,	// 傾きの cos(), sin() 
			m_lqDrawCos, m_lqDrawSin;	// 描画や生成で使用頻度が高いので計算結果を保存
	BOOL	m_bArc;		// TRUE:楕円弧，FALSE:楕円

	void	Construct(void);
	void	EllipseCalc(void);
	void	SetMaxRect(void);
	void	SetEllipseTunPoint(void);
	void	XYRev(const CPointD&, const CPointD&);

protected:
	virtual	void	XRev(void);
	virtual	void	YRev(void);

	CDXFellipse();
public:
	CDXFellipse(LPCDXFEARGV);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFellipse(CLayerData*, const CDXFellipse*, LPCDXFBLOCK);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	const CPointD	GetLongPoint(void) const;
	double	GetShortMagni(void) const;
	double	GetLongLength(void) const;
	double	GetShortLength(void) const;
	double	GetLean(void) const;
	double	GetMakeLean(void) const;
	double	GetMakeLeanCos(void) const;
	double	GetMakeLeanSin(void) const;
	BOOL	IsArc(void) const;
	virtual	void	SetRoundFixed(const CPointD&, const CPointD&);
			void	SetRoundFixed(BOOL);

	virtual	BOOL	IsMakeTarget(void) const;
	virtual	BOOL	IsMakeMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
			BOOL	IsLongEqShort(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	size_t	SetVectorPoint(VECPOINTD&, double = 0.0) const;
	virtual	void	SetVectorPoint(VECPOINTD&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, VECPOINTD&, VECPOINTD&, double) const;

	virtual	void	SwapMakePt(int);
	virtual	void	SwapNativePt(void);

	virtual	void	DrawTuning(const double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointD&, const CPointD&, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;

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

	void		CheckPolylineIntersection_SubLoop(const CPointD&, const CPointD&, POSITION);
	void		CheckPolylineIntersection_SubLoop(const CDXFarc*, POSITION);
	void		CheckPolylineIntersection_SubLoop(const CDXFellipse*, POSITION);

protected:
	virtual	void	XRev(void);
	virtual	void	YRev(void);

public:
	CDXFpolyline();		// 生成直後はﾚｲﾔ情報無し
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFpolyline(CLayerData*, const CDXFpolyline*, LPCDXFBLOCK);
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
	const	CPointD		GetFirstPoint(void) const;
	CDXFdata*	GetFirstObject(void) const;
	CDXFdata*	GetTailObject(void) const;
	BOOL	IsIntersection(void) const;

	BOOL	SetVertex(LPCDXFPARGV);
	BOOL	SetVertex(LPCDXFPARGV, double, const CPointD&);
	void	EndSeq(void);
	void	CheckPolylineIntersection(void);

	virtual	size_t	SetVectorPoint(VECPOINTD&, double = 0.0) const;
	virtual	void	SetVectorPoint(VECPOINTD&, size_t) const;
	virtual	void	SetWireHeteroData(const CDXFdata*, VECPOINTD&, VECPOINTD&, double) const;

	virtual	void	SwapMakePt(int);
	virtual	void	SwapNativePt(void);

	virtual	BOOL	IsMakeTarget(void) const;
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	void	DrawTuning(const double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	int		GetIntersectionPoint(const CPointD&, const CPointD&, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;

	// Polylineの特殊処理(from DXFDoc.cpp2)
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
	CDXFtext(LPCDXFTARGV lpText);
	// BLOCKﾃﾞｰﾀからのｺﾋﾟｰ用
	CDXFtext(CLayerData*, const CDXFtext*, LPCDXFBLOCK);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	CString		GetStrValue(void) const;

	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFtext)
};

#include "DXFdata.inl"
