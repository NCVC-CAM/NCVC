// NCdata.h: CNCdata クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "MainFrm.h"
#include "DXFOption.h"		// enMAKETYPE
#include "NCVCdefine.h"

#ifdef _DEBUG
//#define	_DEBUG_DUMP
#endif

// 始点終点指示
enum	ENPOINTORDER
	{STARTPOINT, ENDPOINT};
// 描画ﾋﾞｭｰ指定
enum	EN_NCDRAWVIEW
	{NCDRAWVIEW_XYZ=0, NCDRAWVIEW_XY=1, NCDRAWVIEW_XZ=2, NCDRAWVIEW_YZ=3};

// 円弧補間ﾊﾟｽ座標
typedef	std::vector<CPoint3D>	CVCircle;

/////////////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀを読み込む時にだけ必要なﾃﾞｰﾀ
// 読み込み終了後に消去することで、ﾒﾓﾘ使用効率を上げる
class CNCread
{
public:
	CPoint3D	m_ptValOrg,		// 回転・ｽｹｰﾙ等無視の純粋な加工終点
				m_ptOffset;		// ﾜｰｸ座標系のｵﾌｾｯﾄ
	G68ROUND	m_g68;			// G68座標回転情報
	TAPER		m_taper;		// ﾃｰﾊﾟ加工情報
};

/////////////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀ，描画ｵﾌﾞｼﾞｪｸﾄ以外のﾍﾞｰｽｸﾗｽ
// 描画ｵﾌﾞｼﾞｪｸﾄ以外でも生成することで，ﾄﾚｰｽの重要ﾎﾟｲﾝﾄで停止させる
class CNCdata
{
	ENNCDTYPE	m_enType;	// ﾃﾞｰﾀﾀｲﾌﾟ : NCVCdefine.h
	void		Constracter(LPNCARGV);

protected:
	NCDATA		m_nc;			// NC基礎ﾃﾞｰﾀ -> NCVCdefine.h
	CPointD		m_pt2D;			// ２次元変換後の座標計算結果(終点)
	int			m_nSpindle;		// 主軸回転数（主に旋盤ﾓｰﾄﾞで使用）
	double		m_dFeed,		// このｵﾌﾞｼﾞｪｸﾄの切削送り速度
				m_dMove[NCXYZ],	// 各軸ごとの移動距離(早送りの時間計算用)
				m_dEndmill;		// ｴﾝﾄﾞﾐﾙ径
	int			m_nEndmillType;	// ｴﾝﾄﾞﾐﾙﾀｲﾌﾟ
	BOOL		m_bG98;			// G98,G98（主に旋盤ﾓｰﾄﾞで使用）
	CTypedPtrArrayEx<CPtrArray, CNCdata*>
				m_obCdata;		// 補正用ｵﾌﾞｼﾞｪｸﾄ
	CNCdata*	m_pWireObj;		// ﾜｲﾔ加工用UV軸ｵﾌﾞｼﾞｪｸﾄ
	//	固定ｻｲｸﾙでは，指定された座標が最終座標ではないので
	//	m_nc.dValue[] ではない最終座標の保持が必要
	// 他、座標回転(G68)でもｵﾘｼﾞﾅﾙ座標と計算座標を別に保管
	CPoint3D	m_ptValS, m_ptValE;	// 開始,終了座標
	CNCread*	m_pRead;		// 読み込み終了後に消去するﾃﾞｰﾀ群

	// G68座標回転
	void	CalcG68Round(LPG68ROUND, CPoint3D&) const;

	// 派生ｸﾗｽ用ｺﾝｽﾄﾗｸﾀ
	CNCdata(ENNCDTYPE, const CNCdata*, LPNCARGV, const CPoint3D&);
	CNCdata(const CNCdata*);

public:
	CNCdata(LPNCARGV);				// 初回の初期化ｺﾝｽﾄﾗｸﾀ
	CNCdata(const CNCdata*, LPNCARGV, const CPoint3D&);	// 本ﾃﾞｰﾀ
	virtual	~CNCdata();

public:
	ENNCDTYPE	GetType(void) const;
	UINT	GetNCObjErrorCode(void) const;
	int		GetBlockLineNo(void) const;
	int		GetGtype(void) const;
	int		GetGcode(void) const;
	BOOL	IsCutCode(void) const;
	ENPLANE	GetPlane(void) const;
	DWORD	GetValFlags(void) const;
	double	GetValue(size_t) const;
	// CPoint3Dから平面の2D座標を抽出
	CPointD	GetPlaneValue(const CPoint3D&) const;
	// CPointDからCPoint3Dへ座標設定
	void	SetPlaneValue(const CPointD&, CPoint3D&) const;
	// CPoint3Dから平面の2D座標を抽出して原点補正
	CPointD	GetPlaneValueOrg(const CPoint3D&, const CPoint3D&) const;
	//
	const CPoint3D	GetStartPoint(void) const;
	const CPoint3D	GetEndPoint(void) const;
	const CPoint3D	GetOriginalEndPoint(void) const;
	const CPoint3D	GetOffsetPoint(void) const;
	double	GetEndValue(size_t) const;
	double	GetOriginalEndValue(size_t) const;
	const CPoint3D	GetEndCorrectPoint(void) const;
	CNCdata*	GetEndCorrectObject(void);
	const CPointD	Get2DPoint(void) const;
	double	GetCutLength(void) const;
	int		GetSpindle(void) const;
	double	GetFeed(void) const;
	double	GetMove(size_t) const;
	double	SetMove(size_t, double);
	double	GetEndmill(void) const;
	int		GetEndmillType(void) const;
	BOOL	GetG98(void) const;
	CNCdata*	NC_CopyObject(void);			// from TH_Correct.cpp
	void		AddCorrectObject(CNCdata*);
	CTypedPtrArrayEx<CPtrArray, CNCdata*>*
				GetCorrectArray(void);			// DXF出力用
	void		SetWireObj(CNCdata*);			// from TH_UVWire.cpp
	CNCdata*	GetWireObj(void) const;
	const CNCread*	GetReadData(void) const;
	void		DeleteReadData(void);

	virtual	void	DrawTuning(double);
	virtual	void	DrawTuningXY(double);
	virtual	void	DrawTuningXZ(double);
	virtual	void	DrawTuningYZ(double);
	virtual	void	Draw(CDC*, BOOL) const;
	virtual	void	DrawXY(CDC*, BOOL) const;
	virtual	void	DrawXZ(CDC*, BOOL) const;
	virtual	void	DrawYZ(CDC*, BOOL) const;
	virtual	void	DrawWire(CDC*, BOOL) const;
	virtual	void	DrawWireXY(CDC*, BOOL) const;
	virtual	void	DrawWireXZ(CDC*, BOOL) const;
	virtual	void	DrawWireYZ(CDC*, BOOL) const;
	virtual	void	DrawGLMillWire(void) const;
	virtual	void	DrawGLWireWire(void) const;
	virtual	void	DrawGLLatheFace(void) const;
	virtual	void	DrawGLBottomFace(void) const;
			int		AddGLWireFirstVertex(std::vector<GLfloat>&, std::vector<GLfloat>&) const;
	virtual	int		AddGLWireVertex(std::vector<GLfloat>&, std::vector<GLfloat>&) const;
	virtual	int		AddGLWireTexture(int, double&, double, GLfloat*) const;
	//
	virtual	CPoint	GetDrawStartPoint(size_t) const;
	virtual	CPoint	GetDrawEndPoint(size_t) const;
	virtual	void	DrawWireLine(EN_NCDRAWVIEW, CDC*, BOOL) const;

	// ｵﾌﾞｼﾞｪｸﾄ占有矩形(都度ｾｯﾄ)
	virtual	CRect3D	GetMaxRect(void) const;
	// 切削占有矩形
	virtual	CRect3D	GetMaxCutRect(void) const;
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
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, double, BOOL) const;
	// ｵﾌｾｯﾄ分移動させた交点(円弧は接線)
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const;
	// 補正座標の設定
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);

#ifdef _DEBUG_DUMP
	void	DbgDump(void);
#endif
};

typedef	CTypedPtrArrayEx<CPtrArray, CNCdata*>	CNCarray;
typedef void (CNCdata::*PFNNCDRAWPROC)(CDC*, BOOL) const;
typedef void (CNCdata::*PFNGLDRAWPROC)(void) const;

/////////////////////////////////////////////////////////////////////////////
// G0,G1 直線補間ｸﾗｽ
class CNCline : public CNCdata
{
	EN_NCPEN	GetPenType(void) const;
	int			GetLineType(void) const;
	void	DrawLine(EN_NCDRAWVIEW, CDC*, BOOL) const;
	void	SetEndmillPath(CPointD*, CPointD*, CPointD*) const;

protected:
	CPointD		m_pt2Ds;				// 2次元変換後の始点(XYZ平面用)
	CPoint		m_ptDrawS[1+NCXYZ],		// 拡大係数込みの描画始点終点
				m_ptDrawE[1+NCXYZ];

	CNCline(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset) :	// 派生ｸﾗｽ用
		CNCdata(enType, pData, lpArgv, ptOffset) {}
public:
	CNCline(const CNCdata*, LPNCARGV, const CPoint3D&);
	CNCline(const CNCdata*);

public:
	virtual	void	DrawTuning(double);
	virtual	void	DrawTuningXY(double);
	virtual	void	DrawTuningXZ(double);
	virtual	void	DrawTuningYZ(double);
	virtual	void	Draw(CDC*, BOOL) const;
	virtual	void	DrawXY(CDC*, BOOL) const;
	virtual	void	DrawXZ(CDC*, BOOL) const;
	virtual	void	DrawYZ(CDC*, BOOL) const;
	virtual	void	DrawWire(CDC*, BOOL) const;
	virtual	void	DrawWireXY(CDC*, BOOL) const;
	virtual	void	DrawWireXZ(CDC*, BOOL) const;
	virtual	void	DrawWireYZ(CDC*, BOOL) const;
	virtual	void	DrawGLMillWire(void) const;
	virtual	void	DrawGLWireWire(void) const;
	virtual	void	DrawGLLatheFace(void) const;
	virtual	void	DrawGLBottomFace(void) const;
	virtual	int		AddGLWireVertex(std::vector<GLfloat>&, std::vector<GLfloat>&) const;
	virtual	int		AddGLWireTexture(int, double&, double, GLfloat*) const;
	//
	virtual	CPoint	GetDrawStartPoint(size_t) const;
	virtual	CPoint	GetDrawEndPoint(size_t) const;
	virtual	void	DrawWireLine(EN_NCDRAWVIEW, CDC*, BOOL) const;

	virtual	CRect3D	GetMaxRect(void) const;
	virtual	CRect3D	GetMaxCutRect(void) const;
	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, double, BOOL) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const;
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
struct PTCYCLE3D
{
	CPoint3D	ptI, ptR, ptC;
};

class CNCcycle : public CNCline
{
	int			m_nDrawCnt;			// ｵﾌﾞｼﾞｪｸﾄ座標値生成数==描画用繰り返し数
	PTCYCLE*	m_Cycle[1+NCXYZ];	// XYZとXY,XZ,YZ
	PTCYCLE3D*	m_Cycle3D;			// 実際の3D座標(兼OpenGL)

	CPoint3D	m_ptValI,			// 前回位置と１点目の穴加工座標
				m_ptValR;
	CPoint		m_ptDrawI[1+NCXYZ],	// 拡大係数
				m_ptDrawR[1+NCXYZ];

	double		m_dInitial,			// ｲﾆｼｬﾙ点の記憶
				m_dCycleMove,		// 移動距離(切削距離はm_nc.dLength)
				m_dDwell;			// ﾄﾞｳｪﾙ時間

	void	DrawCyclePlane(EN_NCDRAWVIEW, CDC*, BOOL) const;
	void	DrawCycle(EN_NCDRAWVIEW, CDC*, BOOL) const;

public:
	CNCcycle(const CNCdata*, LPNCARGV, const CPoint3D&, BOOL);
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
	virtual	void	Draw(CDC*, BOOL) const;
	virtual	void	DrawXY(CDC*, BOOL) const;
	virtual	void	DrawXZ(CDC*, BOOL) const;
	virtual	void	DrawYZ(CDC*, BOOL) const;
	virtual	void	DrawWire(CDC*, BOOL) const;
	virtual	void	DrawWireXY(CDC*, BOOL) const;
	virtual	void	DrawWireXZ(CDC*, BOOL) const;
	virtual	void	DrawWireYZ(CDC*, BOOL) const;
	virtual	void	DrawGLMillWire(void) const;
	virtual	void	DrawGLWireWire(void) const;
	virtual	void	DrawGLLatheFace(void) const;
	virtual	void	DrawGLBottomFace(void) const;
	virtual	int		AddGLWireVertex(std::vector<GLfloat>&, std::vector<GLfloat>&) const;
	virtual	int		AddGLWireTexture(int, double&, double, GLfloat*) const;

	virtual	CRect3D	GetMaxRect(void) const;
	virtual	CRect3D	GetMaxCutRect(void) const;
	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, double, BOOL) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// G2,G3 円弧補間ｸﾗｽ
class	CNCcircle;
typedef void (CNCcircle::*PFNCIRCLEDRAW)(EN_NCDRAWVIEW, CDC*) const;

class CNCcircle : public CNCline  
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
	void	DrawEndmillXYPath(void) const;
	void	DrawEndmillPipe(void) const;
	void	DrawEndmillBall(void) const;

	// IJK指定なしの時，円の方程式から中心の算出
	BOOL	CalcCenter(const CPointD&, const CPointD&);
	void	SetCenter(const CPointD&);
	// 平面座標からの角度計算と調整
	void	AngleTuning(const CPointD&, const CPointD&);

public:
	CNCcircle(const CNCdata*, LPNCARGV, const CPoint3D&, enMAKETYPE enType = NCMAKEMILL);
	CNCcircle(const CNCdata*);

	int		GetG23(void) const;
	const	CPoint3D	GetOrg(void) const;
	double	GetR(void) const;
	double	GetStartAngle(void) const;
	double	GetEndAngle(void) const;
	boost::tuple<double, double>	GetSqEq(void) const;

	void	Draw_G17(EN_NCDRAWVIEW, CDC*) const;
	void	Draw_G18(EN_NCDRAWVIEW, CDC*) const;
	void	Draw_G19(EN_NCDRAWVIEW, CDC*) const;
	void	DrawGLWire(void) const;

	virtual	void	DrawTuning(double);
	virtual	void	DrawTuningXY(double);
	virtual	void	DrawTuningXZ(double);
	virtual	void	DrawTuningYZ(double);
	virtual	void	Draw(CDC*, BOOL) const;
	virtual	void	DrawXY(CDC*, BOOL) const;
	virtual	void	DrawXZ(CDC*, BOOL) const;
	virtual	void	DrawYZ(CDC*, BOOL) const;
	virtual	void	DrawWire(CDC*, BOOL) const;
	virtual	void	DrawWireXY(CDC*, BOOL) const;
	virtual	void	DrawWireXZ(CDC*, BOOL) const;
	virtual	void	DrawWireYZ(CDC*, BOOL) const;
	virtual	void	DrawGLMillWire(void) const;
	virtual	void	DrawGLWireWire(void) const;
	virtual	void	DrawGLLatheFace(void) const;
	virtual	void	DrawGLBottomFace(void) const;
	virtual	int		AddGLWireVertex(std::vector<GLfloat>&, std::vector<GLfloat>&) const;
	virtual	int		AddGLWireTexture(int, double&, double, GLfloat*) const;
	//
	virtual	void	DrawWireLine(EN_NCDRAWVIEW, CDC*, BOOL) const;

	virtual	CRect3D	GetMaxRect(void) const;
	virtual	CRect3D	GetMaxCutRect(void) const;
	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, double, BOOL) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const;
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
