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

// 内部情報を double -> float へ変更したことに伴い
// NCVCdefine.h で定義された構造体を再定義
struct	_NCDATA
{
	UINT		nErrorCode;			// NCﾃﾞｰﾀのｴﾗｰｺｰﾄﾞ
	int			nLine;				// 行番号
	int			nGtype;				// GSMOF
	int			nGcode;				// ｺｰﾄﾞに続く値
	ENPLANE		enPlane;			// 平面
	DWORD		dwValFlags;			// dValue で指定されている値
	float		dValue[VALUESIZE];	// X,Y,Z,U,V,W,I,J,K,R,P,L,D,H
	float		dLength;			// 移動・切削長
};
struct	_G68ROUND
{
	ENPLANE		enPlane;		// 処理中の平面
	float		dRound;			// 回転角度(rad)
	float		dOrg[NCXYZ];	// 回転中心座標
	_G68ROUND(const _G68ROUND*);
	_G68ROUND(const G68ROUND&);
};
struct	_TAPER
{
	int		nTaper;		// 1:G51, -1:G52
	float	dTaper;		// ﾃｰﾊﾟ角度(rad)
	int		nDiff;		// 上下独立ｺｰﾅｰ 0:G60, 1:G61, 2:G62, 3:G63
	BOOL	bTonly;		// Tｺｰﾄﾞ単独指示
	_TAPER(const _TAPER*);
	_TAPER(const TAPER&);
};
typedef	_G68ROUND*	_LPG68ROUND;
typedef	_TAPER*		_LPTAPER;

// 刃物ﾀｲﾌﾟ
enum
{
	NCMIL_SQUARE = 0,
	NCMIL_BALL,
	NCMIL_CHAMFER,		// 面取りミル(先端角90°)
	NCMIL_DRILL,		// ドリル(先端角118°)
		NCMIL_MAXTYPE		// [4]
};

// NCﾃﾞｰﾀ状態ﾌﾗｸﾞ
#define	NCFLG_ENDMILL		0x00000007	// 刃物ﾀｲﾌﾟﾋﾞｯﾄ
#define	NCFLG_G02G03		0x00000010	// 0:G02, 1:G03
#define	NCFLG_G98			0x00000020	// G98,G99（旋盤ﾓｰﾄﾞで使用）
#define	NCFLG_LATHE_INPASS	0x00010000	// 中ぐり（旋盤ﾓｰﾄﾞで使用 径補正と同じ上位ﾋﾞｯﾄ使用）
#define	NCFLG_LATHE_HOLE	0x00020000	// 穴あけ（旋盤ﾓｰﾄﾞで使用 径補正と同じ上位ﾋﾞｯﾄ使用）
#define	NCFLG_LATHE_INSIDE	(NCFLG_LATHE_INPASS|NCFLG_LATHE_HOLE)	// 内側

// 始点終点指示
enum	ENPOINTORDER
	{STARTPOINT=0, ENDPOINT=1};
// 描画ﾋﾞｭｰ指定
enum	ENNCDRAWVIEW
{
	NCDRAWVIEW_XYZ=0, NCDRAWVIEW_XY, NCDRAWVIEW_XZ, NCDRAWVIEW_YZ,
		NCDRAWVIEW_NUM		// [4]
};
#define	NCVIEW_OPENGL			6
//
typedef	std::vector<GLuint>		CVelement;
typedef	std::vector<GLfloat>	CVfloat;
//typedef	std::vector<GLdouble>	CVdouble;

// ｴﾝﾄﾞﾐﾙ底面描画用
struct BOTTOMDRAW
{
	GLenum		mode;
	CVfloat		vpt;
	CVelement	vel;
	GLuint		rs, re;
};
class CVBtmDraw : public std::vector<BOTTOMDRAW>
{
public:
	void	Draw(void);
};

// 旋盤用定数
const float		LATHELINEWIDTH = 3.0f;	// ﾃﾞﾌﾟｽ値を拾うための線幅
const double	LATHEHEIGHT = 6.0;		// glOrtho()のbottomとtop

// ﾜｲﾔ放電加工機描画用
struct WIRELINE
{
	COLORREF	col;
	GLushort	pattern;
	CVelement	vel;
};
struct WIREDRAW
{
	CVfloat		vpt,	// 頂点座標
				vnr;	// 法線ﾍﾞｸﾄﾙ
	std::vector<CVelement>	vvef;	// 面生成用の頂点ｲﾝﾃﾞｯｸｽ
	std::vector<WIRELINE>	vwl;	// 線描画用の情報
	std::vector<float>		vlen;	// ﾃｸｽﾁｬで使用
	void	clear(void);
};

/////////////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀを読み込む時にだけ必要なﾃﾞｰﾀ
// 読み込み終了後に消去することで、ﾒﾓﾘ使用効率を上げる
struct CNCread
{
	CPoint3F	m_ptValOrg,		// 回転・ｽｹｰﾙ等無視の純粋な加工終点
				m_ptOffset;		// ﾜｰｸ座標系のｵﾌｾｯﾄ
	_LPG68ROUND	m_pG68;			// G68座標回転情報
	_LPTAPER	m_pTaper;		// ﾃｰﾊﾟ加工情報
	virtual ~CNCread();
};

/////////////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀ，描画ｵﾌﾞｼﾞｪｸﾄ以外のﾍﾞｰｽｸﾗｽ
// 描画ｵﾌﾞｼﾞｪｸﾄ以外でも生成することで，ﾄﾚｰｽの重要ﾎﾟｲﾝﾄで停止させる
class CNCdata;
typedef	CTypedPtrArrayEx<CPtrArray, CNCdata*>	CNCarray;
class CNCdata
{
	ENNCDTYPE	m_enType;		// ﾃﾞｰﾀﾀｲﾌﾟ : NCVCdefine.h
	void		Constracter(LPNCARGV);

protected:
	DWORD		m_dwFlags;		// NCﾃﾞｰﾀ状態ﾌﾗｸﾞ
	_NCDATA		m_nc;			// NC基礎ﾃﾞｰﾀ : NCVCdefine.h
	int			m_nSpindle;		// 主軸回転数（主に旋盤ﾓｰﾄﾞで使用）
	float		m_dFeed,		// このｵﾌﾞｼﾞｪｸﾄの切削送り速度
				m_dMove[NCXYZ],	// 各軸ごとの移動距離(早送りの時間計算用)
				m_dEndmill;		// ｴﾝﾄﾞﾐﾙ半径
	CNCarray	m_obCdata;		// 補正用ｵﾌﾞｼﾞｪｸﾄ
	CNCdata*	m_pWireObj;		// ﾜｲﾔ加工用UV軸ｵﾌﾞｼﾞｪｸﾄ
	//	固定ｻｲｸﾙでは，指定された座標が最終座標ではないので
	//	m_nc.dValue[] ではない最終座標の保持が必要
	// 他、座標回転(G68)でもｵﾘｼﾞﾅﾙ座標と計算座標を別に保管
	CPoint3F	m_ptValS, m_ptValE;	// 開始,終了座標
	CPointF		m_pt2D;			// ２次元変換後の座標計算結果(終点)
	CNCread*	m_pRead;		// 読み込み終了後に消去するﾃﾞｰﾀ群

	// G68座標回転
	void	CalcG68Round( LPG68ROUND, CPoint3F&) const;
	void	CalcG68Round(_LPG68ROUND, CPoint3F&) const;

	// 派生ｸﾗｽ用ｺﾝｽﾄﾗｸﾀ
	CNCdata(ENNCDTYPE, const CNCdata*, LPNCARGV, const CPoint3F&);
	CNCdata(const CNCdata*);

public:
	CNCdata(LPNCARGV);				// 初回の初期化ｺﾝｽﾄﾗｸﾀ
	CNCdata(const CNCdata*, LPNCARGV, const CPoint3F&);	// 本ﾃﾞｰﾀ
	virtual	~CNCdata();

	ENNCDTYPE	GetType(void) const;
	DWORD	GetFlags(void) const;
	UINT	GetNCObjErrorCode(void) const;
	int		GetBlockLineNo(void) const;
	int		GetGtype(void) const;
	int		GetGcode(void) const;
	BOOL	IsCutCode(void) const;
	BOOL	IsCircle(void) const;
	ENPLANE	GetPlane(void) const;
	DWORD	GetValFlags(void) const;
	float	GetValue(size_t) const;
	// CPoint3Fから平面の2D座標を抽出
	CPointF	GetPlaneValue(const CPoint3F&) const;
	// CPointFからCPoint3Fへ座標設定
	void	SetPlaneValue(const CPointF&, CPoint3F&) const;
	// CPoint3Fから平面の2D座標を抽出して原点補正
	CPointF	GetPlaneValueOrg(const CPoint3F&, const CPoint3F&) const;
	//
	const CPoint3F	GetStartPoint(void) const;
	const CPoint3F	GetEndPoint(void) const;
	const CPoint3F	GetOriginalEndPoint(void) const;
	const CPoint3F	GetOffsetPoint(void) const;
	float	GetEndValue(size_t) const;
	float	GetOriginalEndValue(size_t) const;
	const CPoint3F	GetEndCorrectPoint(void) const;
	CNCdata*	GetEndCorrectObject(void);
	const CPointF	Get2DPoint(void) const;
	float	GetCutLength(void) const;
	int		GetSpindle(void) const;
	float	GetFeed(void) const;
	float	GetMove(size_t) const;
	float	SetMove(size_t, float);
	float	GetEndmill(void) const;
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
	// NCViewGL.cpp からも呼び出し
	void	SetEndmillOrgCircle(const CPoint3F&, CVfloat&) const;
	void	SetChamfermillOrg(const CPoint3F&, CVfloat&) const;
	void	SetDrillOrg(const CPoint3F&, CVfloat&) const;
	void	AddEndmillSphere(const CPoint3F&, BOTTOMDRAW&, CVBtmDraw&) const;

	virtual	void	DrawTuning(float);
	virtual	void	DrawTuningXY(float);
	virtual	void	DrawTuningXZ(float);
	virtual	void	DrawTuningYZ(float);
	virtual	void	Draw(CDC*, BOOL) const;
	virtual	void	DrawXY(CDC*, BOOL) const;
	virtual	void	DrawXZ(CDC*, BOOL) const;
	virtual	void	DrawYZ(CDC*, BOOL) const;
	virtual	void	DrawWire(CDC*, BOOL) const;
	virtual	void	DrawWireXY(CDC*, BOOL) const;
	virtual	void	DrawWireXZ(CDC*, BOOL) const;
	virtual	void	DrawWireYZ(CDC*, BOOL) const;
	virtual	void	DrawGLWirePass(void) const;
	virtual	void	DrawGLLatheDepth(void) const;
	virtual	BOOL	AddGLBottomFaceVertex(CVBtmDraw&, BOOL) const;
	virtual	BOOL	AddGLWireVertex(CVfloat&, CVfloat&, CVelement&, WIRELINE&, BOOL) const;
	virtual	int		AddGLWireTexture(size_t, float&, float, GLfloat*) const;
	//
	virtual	CPoint	GetDrawStartPoint(size_t) const;
	virtual	CPoint	GetDrawEndPoint(size_t) const;
	virtual	void	DrawWireLine(ENNCDRAWVIEW, CDC*, BOOL) const;

	// ｵﾌﾞｼﾞｪｸﾄ占有矩形(都度ｾｯﾄ)
	virtual	CRect3F	GetMaxRect(void) const;
	// 切削占有矩形
	virtual	CRect3F	GetMaxCutRect(void) const;
	// 移動長，切削長の計算
	virtual	float	SetCalcLength(void);
	// 丸め(ｺｰﾅｰR)の座標計算
	virtual	boost::tuple<BOOL, CPointF, float, float>	CalcRoundPoint(const CNCdata*, float) const;
	// 面取り位置の座標設定
	virtual	boost::optional<CPointF>	SetChamferingPoint(BOOL, float);
	// ２線がなす角度を求める
	virtual	float	CalcBetweenAngle(const CNCdata*) const;
	// ｵﾌｾｯﾄ値の符号計算
	virtual	int		CalcOffsetSign(void) const;
	// [始点|終点]を垂直にｵﾌｾｯﾄ(90°回転)した座標計算
	virtual	boost::optional<CPointF>	CalcPerpendicularPoint(ENPOINTORDER, float, int) const;
	// ｵﾌｾｯﾄ分移動させた交点
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CNCdata*, float, float, BOOL) const;
	// ｵﾌｾｯﾄ分移動させた交点(円弧は接線)
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint2(const CNCdata*, float, BOOL) const;
	// 補正座標の設定
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointF&, float);

#ifdef _DEBUG_DUMP
	void	DbgDump(void);
#endif
};

typedef void (CNCdata::*PFNNCDRAWPROC)(CDC*, BOOL) const;
typedef	void (CNCdata::*PFNSETENDMILLFUNC)(const CPoint3F&, CVfloat&) const;

/////////////////////////////////////////////////////////////////////////////
// G0,G1 直線補間ｸﾗｽ
class CNCline : public CNCdata
{
	EN_NCPEN	GetPenType(void) const;
	int			GetLineType(void) const;
	void	DrawLine(ENNCDRAWVIEW, CDC*, BOOL) const;

protected:
	CPointF		m_pt2Ds;				// 2次元変換後の始点(XYZ平面用)
	CPoint		m_ptDrawS[NCDRAWVIEW_NUM],	// 拡大係数込みの描画始点終点
				m_ptDrawE[NCDRAWVIEW_NUM];

	CNCline(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset) :	// 派生ｸﾗｽ用
		CNCdata(enType, pData, lpArgv, ptOffset) {}
public:
	CNCline(const CNCdata*, LPNCARGV, const CPoint3F&);
	CNCline(const CNCdata*);

	virtual	void	DrawTuning(float);
	virtual	void	DrawTuningXY(float);
	virtual	void	DrawTuningXZ(float);
	virtual	void	DrawTuningYZ(float);
	virtual	void	Draw(CDC*, BOOL) const;
	virtual	void	DrawXY(CDC*, BOOL) const;
	virtual	void	DrawXZ(CDC*, BOOL) const;
	virtual	void	DrawYZ(CDC*, BOOL) const;
	virtual	void	DrawWire(CDC*, BOOL) const;
	virtual	void	DrawWireXY(CDC*, BOOL) const;
	virtual	void	DrawWireXZ(CDC*, BOOL) const;
	virtual	void	DrawWireYZ(CDC*, BOOL) const;
	virtual	void	DrawGLWirePass(void) const;
	virtual	void	DrawGLLatheDepth(void) const;
	virtual	BOOL	AddGLBottomFaceVertex(CVBtmDraw&, BOOL) const;
	virtual	BOOL	AddGLWireVertex(CVfloat&, CVfloat&, CVelement&, WIRELINE&, BOOL) const;
	virtual	int		AddGLWireTexture(size_t, float&, float, GLfloat*) const;
	//
	virtual	CPoint	GetDrawStartPoint(size_t) const;
	virtual	CPoint	GetDrawEndPoint(size_t) const;
	virtual	void	DrawWireLine(ENNCDRAWVIEW, CDC*, BOOL) const;

	virtual	CRect3F	GetMaxRect(void) const;
	virtual	CRect3F	GetMaxCutRect(void) const;
	virtual	float	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointF, float, float>	CalcRoundPoint(const CNCdata*, float) const;
	virtual	boost::optional<CPointF>	SetChamferingPoint(BOOL, float);
	virtual	float	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointF>	CalcPerpendicularPoint(ENPOINTORDER, float, int) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CNCdata*, float, float, BOOL) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint2(const CNCdata*, float, BOOL) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointF&, float);
};

/////////////////////////////////////////////////////////////////////////////
// G8x 固定ｻｲｸﾙｸﾗｽ
struct PTCYCLE
{
	CPointF		ptI, ptR, ptC;	// ｲﾆｼｬﾙ点、R点、切り込み点
	CPoint		ptDrawI, ptDrawR, ptDrawC;
	CRect		rcDraw;			// 同一平面の丸印描画用
	void	DrawTuning(float f);
};
struct PTCYCLE3D
{
	CPoint3F	ptI, ptR, ptC;
};

class CNCcycle : public CNCline
{
	int			m_nDrawCnt;			// ｵﾌﾞｼﾞｪｸﾄ座標値生成数==描画用繰り返し数
	PTCYCLE*	m_Cycle[NCDRAWVIEW_NUM];
	PTCYCLE3D*	m_Cycle3D;			// 実際の3D座標(兼OpenGL)

	CPoint3F	m_ptValI,			// 前回位置と１点目の穴加工座標
				m_ptValR;
	CPoint		m_ptDrawI[NCDRAWVIEW_NUM],	// 拡大係数
				m_ptDrawR[NCDRAWVIEW_NUM];

	float		m_dInitial;			// ｲﾆｼｬﾙ点の記憶
	float		m_dCycleMove,		// 移動距離(切削距離はm_nc.dLength)
				m_dDwell;			// ﾄﾞｳｪﾙ時間

	void	DrawCyclePlane(ENNCDRAWVIEW, CDC*, BOOL) const;
	void	DrawCycle(ENNCDRAWVIEW, CDC*, BOOL) const;

public:
	CNCcycle(const CNCdata*, LPNCARGV, const CPoint3F&, BOOL, enMAKETYPE);
	virtual ~CNCcycle();

	int		GetDrawCnt(void) const;
	const	PTCYCLE*	GetCycleInside(size_t) const;
	const	CPoint3F	GetIPoint(void) const;
	const	CPoint3F	GetRPoint(void) const;
	float	GetInitialValue(void) const;
	float	GetCycleMove(void) const;
	float	GetDwell(void) const;

	virtual	void	DrawTuning(float);
	virtual	void	DrawTuningXY(float);
	virtual	void	DrawTuningXZ(float);
	virtual	void	DrawTuningYZ(float);
	virtual	void	Draw(CDC*, BOOL) const;
	virtual	void	DrawXY(CDC*, BOOL) const;
	virtual	void	DrawXZ(CDC*, BOOL) const;
	virtual	void	DrawYZ(CDC*, BOOL) const;
	virtual	void	DrawWire(CDC*, BOOL) const;
	virtual	void	DrawWireXY(CDC*, BOOL) const;
	virtual	void	DrawWireXZ(CDC*, BOOL) const;
	virtual	void	DrawWireYZ(CDC*, BOOL) const;
	virtual	void	DrawGLWirePass(void) const;
	virtual	void	DrawGLLatheDepth(void) const;
	virtual	BOOL	AddGLBottomFaceVertex(CVBtmDraw&, BOOL) const;
	virtual	BOOL	AddGLWireVertex(CVfloat&, CVfloat&, CVelement&, WIRELINE&, BOOL) const;
	virtual	int		AddGLWireTexture(size_t, float&, float, GLfloat*) const;

	virtual	CRect3F	GetMaxRect(void) const;
	virtual	CRect3F	GetMaxCutRect(void) const;
	virtual	float	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointF, float, float>	CalcRoundPoint(const CNCdata*, float) const;
	virtual	boost::optional<CPointF>	SetChamferingPoint(BOOL, float);
	virtual	float	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointF>	CalcPerpendicularPoint(ENPOINTORDER, float, int) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CNCdata*, float, float, BOOL) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint2(const CNCdata*, float, BOOL) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointF&, float);
};

/////////////////////////////////////////////////////////////////////////////
// G2,G3 円弧補間ｸﾗｽ
class	CNCcircle;
typedef void (CNCcircle::*PFNCIRCLEDRAW)(ENNCDRAWVIEW, CDC*) const;

struct DRAWGLWIRECIRCLE	// DrawGLWirePassCircle()引数
{
	BOOL		bG03,	// TRUE:G03, FALSE:G02
				bLatheDepth;	// 旋盤内径描画の特殊性
	float		sq, eq;
	CPoint3F	pts, pte;
};
typedef	DRAWGLWIRECIRCLE*		LPDRAWGLWIRECIRCLE;

class CNCcircle : public CNCline  
{
	void		Constracter(void);

	CPoint3F	m_ptOrg;		// 中心座標
	float		m_r,			// 半径
				m_sq, m_eq,		// 開始・終了角度(ﾗｼﾞｱﾝ単位)
				m_dHelicalStep;	// ﾍﾘｶﾙ動作の移動量
	// 描画用
	float		m_dFactor,		// 現在の拡大率(計算しながら描画)
				m_dFactorXY,
				m_dFactorXZ,
				m_dFactorYZ;
	PFNCIRCLEDRAW	m_pfnCircleDraw;	// 平面別の描画関数
	//
	void	SetEndmillXYPath(CVfloat&) const;
	void	SetEndmillSquare(BOTTOMDRAW&, CVBtmDraw&) const;
	void	SetEndmillBall(BOTTOMDRAW&, CVBtmDraw&) const;
	void	SetEndmillChamferAndDrill(BOTTOMDRAW&, CVBtmDraw&) const;
	void	SetEndmillSpherePathCircle(float, const CPoint3F&, CVfloat&) const;
	void	SetEndmillPathXZ_Sphere(float, const CPoint3F&, CVfloat&) const;
	void	SetEndmillPathYZ_Sphere(float, const CPoint3F&, CVfloat&) const;
	void	AddEndmillPipe(GLuint, BOTTOMDRAW&, CVBtmDraw&) const;
	void	AddEndmillChamferAndDrill(BOTTOMDRAW&, CVBtmDraw&) const;
	void	DrawG17(ENNCDRAWVIEW, CDC*) const;
	void	DrawG18(ENNCDRAWVIEW, CDC*) const;
	void	DrawG19(ENNCDRAWVIEW, CDC*) const;
	void	DrawGLWirePassCircle(const CPoint3F&, const CPoint3F&) const;
	void	DrawGLWirePassCircle(LPDRAWGLWIRECIRCLE) const;

	// IJK指定なしの時，円の方程式から中心の算出
	BOOL	CalcCenter(const CPointF&, const CPointF&);
	void	SetCenter(const CPointF&);
	// 平面座標からの角度計算と調整
	boost::tuple<float, float>	CalcAngle(BOOL, const CPointF&, const CPointF&) const;
	void	AngleTuning(const CPointF&, const CPointF&);

public:
	CNCcircle(const CNCdata*, LPNCARGV, const CPoint3F&, enMAKETYPE);
	CNCcircle(const CNCdata*);

	BOOL	GetG03(void) const;
	const	CPoint3F	GetOrg(void) const;
	float	GetR(void) const;
	float	GetStartAngle(void) const;
	float	GetEndAngle(void) const;
	boost::tuple<float, float>	GetSqEq(void) const;

	virtual	void	DrawTuning(float);
	virtual	void	DrawTuningXY(float);
	virtual	void	DrawTuningXZ(float);
	virtual	void	DrawTuningYZ(float);
	virtual	void	Draw(CDC*, BOOL) const;
	virtual	void	DrawXY(CDC*, BOOL) const;
	virtual	void	DrawXZ(CDC*, BOOL) const;
	virtual	void	DrawYZ(CDC*, BOOL) const;
	virtual	void	DrawWire(CDC*, BOOL) const;
	virtual	void	DrawWireXY(CDC*, BOOL) const;
	virtual	void	DrawWireXZ(CDC*, BOOL) const;
	virtual	void	DrawWireYZ(CDC*, BOOL) const;
	virtual	void	DrawGLWirePass(void) const;
	virtual	void	DrawGLLatheDepth(void) const;
	virtual	BOOL	AddGLBottomFaceVertex(CVBtmDraw&, BOOL) const;
	virtual	BOOL	AddGLWireVertex(CVfloat&, CVfloat&, CVelement&, WIRELINE&, BOOL) const;
	virtual	int		AddGLWireTexture(size_t, float&, float, GLfloat*) const;
	//
	virtual	void	DrawWireLine(ENNCDRAWVIEW, CDC*, BOOL) const;

	virtual	CRect3F	GetMaxRect(void) const;
	virtual	CRect3F	GetMaxCutRect(void) const;
	virtual	float	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointF, float, float>	CalcRoundPoint(const CNCdata*, float) const;
	virtual	boost::optional<CPointF>	SetChamferingPoint(BOOL, float);
	virtual	float	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointF>	CalcPerpendicularPoint(ENPOINTORDER, float, int) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CNCdata*, float, float, BOOL) const;
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint2(const CNCdata*, float, BOOL) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointF&, float);
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
	size_t		m_nArray;	// CNCDoc::m_obGdata 内の番号

public:
	CNCblock(const CString&, DWORD = 0);

	CString	GetStrLine(void) const;
	CString	GetStrGcode(void) const;
	CString	GetStrBlock(void) const;
	DWORD	GetBlockFlag(void) const;
	void	SetBlockFlag(DWORD, BOOL = TRUE);
	UINT	GetNCBlkErrorCode(void) const;
	void	SetNCBlkErrorCode(UINT);
	CNCdata*	GetBlockToNCdata(void) const;
	size_t	GetBlockToNCdataArrayNo(void) const;
	void	SetBlockToNCdata(CNCdata*, size_t);
};

typedef	CTypedPtrArrayEx<CPtrArray, CNCblock*>	CNCblockArray;

#include "NCdata.inl"
