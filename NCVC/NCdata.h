// NCdata.h: CNCdata クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "MainFrm.h"
#include "NCVCdefine.h"

// 拡張dwValFlags 上位16ﾋﾞｯﾄを使用
#define	NCD_CORRECT_L		0x00010000
#define	NCD_CORRECT_R		0x00020000
#define	NCD_CORRECT			(NCD_CORRECT_L | NCD_CORRECT_R)

// 始点終点指示
enum	ENPOINTORDER
	{STARTPOINT, ENDPOINT};

/////////////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀ，描画ｵﾌﾞｼﾞｪｸﾄ以外のﾍﾞｰｽｸﾗｽ
// 描画ｵﾌﾞｼﾞｪｸﾄ以外でも生成することで，ﾄﾚｰｽの重要ﾎﾟｲﾝﾄで停止させる
class CNCdata
{
	ENNCDTYPE	m_enType;	// ﾃﾞｰﾀﾀｲﾌﾟ : NCVCdefine.h
	void		Constracter(LPNCARGV);

protected:
	NCDATA		m_nc;			// NC基礎ﾃﾞｰﾀ -> NCVCaddin.h
	CPointD		m_pt2D;			// ２次元変換後の座標計算結果(終点)
	double		m_dFeed;		// このｵﾌﾞｼﾞｪｸﾄの切削送り速度
	double		m_dMove[NCXYZ];	// 各軸ごとの移動距離(早送りの時間計算用)
	CTypedPtrArrayEx<CPtrArray, CNCdata*>
				m_obCdata;		// 補正用ｵﾌﾞｼﾞｪｸﾄ
	//	固定ｻｲｸﾙでは，指定された座標が最終座標ではないので
	//	m_nc.dValue[] ではない最終座標の保持が必要
	CPoint3D	m_ptValS, m_ptValE;		// 開始,終了座標
	CRect3D		m_rcMax;		// 空間占有矩形

	// CPoint3Dから平面の2D座標を抽出
	CPointD	GetPlaneValue(const CPoint3D&) const;
	// CPoint3Dから平面の2D座標を抽出して原点補正
	CPointD	GetPlaneValueOrg(const CPoint3D&, const CPoint3D&) const;

	// 派生ｸﾗｽ用ｺﾝｽﾄﾗｸﾀ
	CNCdata(ENNCDTYPE, const CNCdata*, LPNCARGV);
	CNCdata(const CNCdata*);

public:
	CNCdata(LPNCARGV);				// 初回の初期化ｺﾝｽﾄﾗｸﾀ
	CNCdata(const CNCdata*, LPNCARGV);	// 本ﾃﾞｰﾀ
	virtual	~CNCdata();

public:
	ENNCDTYPE	GetType(void) const;
	UINT	GetNCObjErrorCode(void) const;
	int		GetStrLine(void) const;
	int		GetGtype(void) const;
	int		GetGcode(void) const;
	ENPLANE	GetPlane(void) const;
	DWORD	GetValFlags(void) const;
	double	GetValue(size_t a) const;
	const CPoint3D	GetStartPoint(void) const;
	const CPoint3D	GetEndPoint(void) const;
	double	GetEndValue(size_t a) const;
	const CPoint3D	GetEndCorrectPoint(void) const;
	CNCdata*	GetEndCorrectObject(void);
	const CPointD	Get2DPoint(void) const;
	double	GetCutLength(void) const;
	double	GetFeed(void) const;
	double	GetMove(size_t a) const;
	const CRect3D	GetMaxRect(void) const;
	CNCdata*	CopyObject(void);
	void		AddCorrectObject(CNCdata*);

	virtual	void	DrawTuning(double);
	virtual	void	DrawTuningXY(double);
	virtual	void	DrawTuningXZ(double);
	virtual	void	DrawTuningYZ(double);
	virtual	void	Draw(CDC*, BOOL = FALSE) const;
	virtual	void	DrawXY(CDC*, BOOL = FALSE) const;
	virtual	void	DrawXZ(CDC*, BOOL = FALSE) const;
	virtual	void	DrawYZ(CDC*, BOOL = FALSE) const;

	// 移動長，切削長の計算
	virtual	double	SetCalcLength(void);
	// 丸め(ｺｰﾅｰR)の座標計算
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	// 面取り位置の座標設定
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	// ２線がなす角度を求める
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	// ｵﾌｾｯﾄ値の符号計算
	virtual	int		CalcOffsetSign(void) const;
	// [始点|終点]を垂直にｵﾌｾｯﾄ(90°回転)した座標計算
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	// ｵﾌｾｯﾄ分移動させた交点
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, int, int) const;
	// ｵﾌｾｯﾄ分移動させた交点(円弧は接線)
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, int, int) const;
	// 補正座標の設定
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);

#ifdef _DEBUG
	void	DbgDump(void);
#endif
};

typedef	CTypedPtrArrayEx<CPtrArray, CNCdata*>	CNCarray;

/////////////////////////////////////////////////////////////////////////////
// G0,G1 直線補間ｸﾗｽ
class CNCline : public CNCdata
{
	EN_NCPEN	GetPenType(void) const;
	void	DrawLine(CDC*, size_t, BOOL) const;
	
protected:
	CPointD		m_pt2Ds;				// 2次元変換後の始点(XYZ平面用)
	CPoint		m_ptDrawS[1+NCXYZ],		// 拡大係数込みの描画始点終点
				m_ptDrawE[1+NCXYZ];
	void	SetMaxRect(void);

	CNCline(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv) :	// 派生ｸﾗｽ用
		CNCdata(enType, pData, lpArgv) {}
public:
	CNCline(const CNCdata*, LPNCARGV);
	CNCline(const CNCdata*);

public:
	virtual	void	DrawTuning(double);
	virtual	void	DrawTuningXY(double);
	virtual	void	DrawTuningXZ(double);
	virtual	void	DrawTuningYZ(double);
	virtual	void	Draw(CDC*, BOOL = FALSE) const;
	virtual	void	DrawXY(CDC*, BOOL = FALSE) const;
	virtual	void	DrawXZ(CDC*, BOOL = FALSE) const;
	virtual	void	DrawYZ(CDC*, BOOL = FALSE) const;

	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, int, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, int, int) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// G8x 固定ｻｲｸﾙｸﾗｽ
struct PTCYCLE
{
	CPointD		ptI, ptR, ptC;	// ｲﾆｼｬﾙ点、R点、切り込み点
	CPoint		ptDrawI, ptDrawR, ptDrawC;
	CRect		rcDraw;			// 同一平面の丸印描画用
	void	DrawTuning(double f);
};

class CNCcycle : public CNCline
{
	int			m_nDrawCnt;		// ｵﾌﾞｼﾞｪｸﾄ座標値生成数==描画用繰り返し数
	PTCYCLE*	m_Cycle[1+NCXYZ];	// XYZとXY,XZ,YZ

	CPoint3D	m_ptValI,		// 前回位置と１点目の穴加工座標
				m_ptValR;
	CPoint		m_ptDrawI[1+NCXYZ],		// 拡大係数
				m_ptDrawR[1+NCXYZ];

	double		m_dInitial,		// ｲﾆｼｬﾙ点の記憶
				m_dCycleMove,	// 移動距離(切削距離はm_nc.dLength)
				m_dDwell;		// ﾄﾞｳｪﾙ時間

	void	DrawCyclePlane(CDC*, size_t) const;
	void	DrawCycle(CDC*, size_t) const;

public:
	CNCcycle(const CNCdata*, LPNCARGV);
	virtual ~CNCcycle();

	int		GetDrawCnt(void) const;
	const	PTCYCLE*	GetCycleInside(size_t) const;
	const	CPoint3D	GetIPoint(void) const;
	const	CPoint3D	GetRPoint(void) const;
	double	GetInitialValue(void) const;
	double	GetCycleMove(void) const;
	double	GetDwell(void) const;

	virtual	void	DrawTuning(double);
	virtual	void	DrawTuningXY(double);
	virtual	void	DrawTuningXZ(double);
	virtual	void	DrawTuningYZ(double);
	virtual	void	Draw(CDC*, BOOL = FALSE) const;
	virtual	void	DrawXY(CDC*, BOOL = FALSE) const;
	virtual	void	DrawXZ(CDC*, BOOL = FALSE) const;
	virtual	void	DrawYZ(CDC*, BOOL = FALSE) const;

	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, int, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, int, int) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// G2,G3 円弧補間ｸﾗｽ
class	CNCcircle;
enum	EN_NCCIRCLEDRAW
	{NCCIRCLEDRAW_XYZ, NCCIRCLEDRAW_XY, NCCIRCLEDRAW_XZ, NCCIRCLEDRAW_YZ};
typedef void (CNCcircle::*PFNCIRCLEDRAW)(EN_NCCIRCLEDRAW, CDC*) const;

class CNCcircle : public CNCdata  
{
	void		Constracter(void);

	int			m_nG23;			// G02:0 G03:1
	CPoint3D	m_ptOrg;		// 中心座標
	double		m_r,			// 半径
				m_sq, m_eq,		// 開始・終了角度(ﾗｼﾞｱﾝ単位)
				m_dHelicalStep;	// ﾍﾘｶﾙ動作の移動量
	// 描画用
	double		m_dFactor,		// 現在の拡大率(計算しながら描画)
				m_dFactorXY,
				m_dFactorXZ,
				m_dFactorYZ;
	PFNCIRCLEDRAW	m_pfnCircleDraw;	// 平面別の描画関数
	void	Draw_G17(EN_NCCIRCLEDRAW, CDC*) const;
	void	Draw_G18(EN_NCCIRCLEDRAW, CDC*) const;
	void	Draw_G19(EN_NCCIRCLEDRAW, CDC*) const;

	// IJK指定なしの時，円の方程式から中心の算出
	BOOL	CalcCenter(const CPointD&, const CPointD&);
	void	SetCenter(const CPointD&);
	// 平面座標からの角度計算と調整
	void	AngleTuning(const CPointD&, const CPointD&);

protected:
	// 外接する四角形
	void	SetMaxRect(void);

public:
	CNCcircle(const CNCdata*, LPNCARGV);
	CNCcircle(const CNCdata*);

	int		GetG23(void) const;
	const	CPoint3D	GetOrg(void) const;
	double	GetR(void) const;
	double	GetStartAngle(void) const;
	double	GetEndAngle(void) const;

	virtual	void	DrawTuning(double);
	virtual	void	DrawTuningXY(double);
	virtual	void	DrawTuningXZ(double);
	virtual	void	DrawTuningYZ(double);
	virtual	void	Draw(CDC*, BOOL = FALSE) const;
	virtual	void	DrawXY(CDC*, BOOL = FALSE) const;
	virtual	void	DrawXZ(CDC*, BOOL = FALSE) const;
	virtual	void	DrawYZ(CDC*, BOOL = FALSE) const;

	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, int, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, int, int) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀのﾌﾞﾛｯｸﾃﾞｰﾀｸﾗｽ

// ﾘｿｰｽ内のｴﾗｰｺｰﾄﾞ
#define	NCERROR_RES_MIN	25000
#define	NCERROR_RES_MAX	25999

class CNCblock
{
	CString		m_strLine,	// 行番号
				m_strGcode;	// Gｺｰﾄﾞ文字列
	DWORD		m_dwFlags;	// ﾌﾞﾛｯｸにﾘﾝｸしたﾌﾗｸﾞ
	UINT		m_nError;	// ｴﾗｰｺｰﾄﾞ
	CNCdata*	m_pData;	// ﾌﾞﾛｯｸに対応した最後のCNCdataｵﾌﾞｼﾞｪｸﾄ
	int			m_nArray;	// CNCDoc::m_obGdata 内の番号

public:
	CNCblock(const CString&, const CString&, DWORD = 0);

	CString	GetStrLine(void) const;
	CString	GetStrGcode(void) const;
	CString	GetStrBlock(void) const;
	DWORD	GetBlockFlag(void) const;
	void	SetBlockFlag(DWORD, BOOL = TRUE);
	UINT	GetNCBlkErrorCode(void) const;
	void	SetNCBlkErrorCode(UINT);
	CNCdata*	GetBlockToNCdata(void) const;
	int		GetBlockToNCdataArrayNo(void) const;
	void	SetBlockToNCdata(CNCdata*, int);
};

typedef	CTypedPtrArrayEx<CPtrArray, CNCblock*>	CNCblockArray;

#include "NCdata.inl"
