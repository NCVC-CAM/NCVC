// NCdata.h: CNCdata クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NCDATA_H__E80DE2E0_814F_11D3_B0D5_004005691B12__INCLUDED_)
#define AFX_NCDATA_H__E80DE2E0_814F_11D3_B0D5_004005691B12__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MainFrm.h"
#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀ，描画ｵﾌﾞｼﾞｪｸﾄ以外のﾍﾞｰｽｸﾗｽ
// 描画ｵﾌﾞｼﾞｪｸﾄ以外でも生成することで，ﾄﾚｰｽの重要ﾎﾟｲﾝﾄで停止させる
class CNCdata
{
	ENNCDTYPE	m_enType;	// ﾃﾞｰﾀﾀｲﾌﾟ : NCVCdefine.h
	void		Constracter(LPNCARGV lpArgv);

protected:
	NCDATA		m_nc;			// NC基礎ﾃﾞｰﾀ -> NCVCaddin.h
	CPointD		m_pt2D;			// ２次元変換後の座標計算結果
	double		m_dFeed;		// このｵﾌﾞｼﾞｪｸﾄの切削送り速度
	double		m_dMove[NCXYZ];	// 各軸ごとの移動距離(早送りの時間計算用)
	//	固定ｻｲｸﾙでは，指定された座標が最終座標ではないので
	//	m_nc.dValue[] ではない最終座標の保持が必要
	CPoint3D	m_ptValS, m_ptValE;		// 開始,終了座標
	CRect3D		m_rcMax;		// 空間占有矩形

	// CPoint3Dから平面の2D座標を抽出
	CPointD	GetPlaneValue(const CPoint3D&);
	// CPoint3Dから平面の2D座標を抽出して原点補正
	CPointD	GetPlaneValueOrg(const CPoint3D&, const CPoint3D&);

	CNCdata(ENNCDTYPE, const CNCdata*, LPNCARGV);	// 派生ｸﾗｽ用ｺﾝｽﾄﾗｸﾀ
public:
	CNCdata(LPNCARGV);				// 初回の初期化ｺﾝｽﾄﾗｸﾀ
	CNCdata(const CNCdata*, LPNCARGV);	// 本ﾃﾞｰﾀ
	virtual	~CNCdata() {}

public:
	ENNCDTYPE	GetType(void) const;
	DWORD	GetNCFlags(void) const;
	void	SetNCFlags(DWORD dwFlags);
	int		GetStrLine(void) const;
	int		GetGtype(void) const;
	int		GetGcode(void) const;
	ENPLANE	GetPlane(void) const;
	DWORD	GetValFlags(void) const;
	double	GetValue(size_t a) const;
	const CPoint3D	GetStartPoint(void) const;
	const CPoint3D	GetEndPoint(void) const;
	double	GetEndValue(size_t a) const;
	const CPointD	Get2DPoint(void) const;
	double	GetCutLength(void) const;
	double	GetFeed(void) const;
	double	GetMove(size_t a) const;
	const CRect3D	GetMaxRect(void) const;

	// 純粋仮想関数にするとｵﾌﾞｼﾞｪｸﾄ生成できないので定義のみ
	virtual	void	DrawTuning(double) {}
	virtual	void	DrawTuningXY(double) {}
	virtual	void	DrawTuningXZ(double) {}
	virtual	void	DrawTuningYZ(double) {}
	virtual	void	Draw(CDC*) {}
	virtual	void	DrawXY(CDC*) {}
	virtual	void	DrawXZ(CDC*) {}
	virtual	void	DrawYZ(CDC*) {}

	// 丸め(ｺｰﾅｰR)の座標計算
	virtual	BOOL	CalcRoundPoint(const CNCdata*, double, CPointD&, double&, double&);
	// 面取り位置の座標設定
	virtual	BOOL	SetChamferingPoint(BOOL, double, CPointD&);
	// ２線がなす角度を求める
	virtual	double	CalcBetweenAngle(const CPoint3D&, const CNCdata*);
	// ｵﾌｾｯﾄ値の符号計算
	virtual	int		CalcOffsetSign(const CPoint3D&);
	// [始点|終点]を垂直にｵﾌｾｯﾄ(90°回転)した座標計算
	virtual	CPointD	CalcPerpendicularPoint(const CPoint3D&, const CPoint3D&, double, int);
	// ｵﾌｾｯﾄ分移動させた交点
	virtual	CPointD	CalcOffsetIntersectionPoint(const CPoint3D&, const CNCdata*, double, int, int);
	// ｵﾌｾｯﾄ分移動させた交点(円弧は接線)
	virtual	CPointD	CalcOffsetIntersectionPoint2(const CPoint3D&, const CNCdata*, double, int, int);
	// 補正座標の設定
	virtual	BOOL	SetCorrectPoint(BOOL, const CPointD&, double);

#ifdef _DEBUG
	void	DbgDump(void);
#endif
};

/////////////////////////////////////////////////////////////////////////////
// G0,G1 直線補間ｸﾗｽ
class CNCline : public CNCdata
{
	EN_NCPEN	GetPenType(void);
	// 移動長，切削長の計算
	void	CalcLength(void);

protected:
	CPointD		m_pts, m_pte;			// 2次元変換後の始点終点(XYZ平面用)
	CPoint		m_ptDrawS, m_ptDrawE,	// 拡大係数込みの描画始点終点
				m_ptDrawS_XY, m_ptDrawE_XY,
				m_ptDrawS_XZ, m_ptDrawE_XZ,
				m_ptDrawS_YZ, m_ptDrawE_YZ;
	void	SetMaxRect(void);

	CNCline(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv) :	// 派生ｸﾗｽ用
		CNCdata(enType, pData, lpArgv) {}
public:
	CNCline(const CNCdata*, LPNCARGV);

public:
	virtual	void	DrawTuning(double f);
	virtual	void	DrawTuningXY(double f);
	virtual	void	DrawTuningXZ(double f);
	virtual	void	DrawTuningYZ(double f);
	virtual	void	Draw(CDC* pDC);
	virtual	void	DrawXY(CDC* pDC);
	virtual	void	DrawXZ(CDC* pDC);
	virtual	void	DrawYZ(CDC* pDC);

	virtual	BOOL	CalcRoundPoint(const CNCdata*, double, CPointD&, double&, double&);
	virtual	BOOL	SetChamferingPoint(BOOL, double, CPointD&);
	virtual	double	CalcBetweenAngle(const CPoint3D&, const CNCdata*);
	virtual	int		CalcOffsetSign(const CPoint3D&);
	virtual	CPointD	CalcPerpendicularPoint(const CPoint3D&, const CPoint3D&, double, int);
	virtual	CPointD	CalcOffsetIntersectionPoint(const CPoint3D&, const CNCdata*, double, int, int);
	virtual	CPointD	CalcOffsetIntersectionPoint2(const CPoint3D&, const CNCdata*, double, int, int);
	virtual	BOOL	SetCorrectPoint(BOOL, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// G8x 固定ｻｲｸﾙｸﾗｽ
struct PTCYCLE
{
	CPointD		ptI, ptR, ptZ;
	CPoint		ptDrawI, ptDrawR, ptDrawZ;
	void	DrawTuning(double f);
};
struct PTCYCLE_XY
{
	CPointD		pt;
	CRect		rcDraw;
	void	DrawTuning(double f);
};

class CNCcycle : public CNCline
{
	int			m_nDrawCnt;		// ｵﾌﾞｼﾞｪｸﾄ座標値生成数==描画用繰り返し数
	PTCYCLE*	m_Cycle;
	PTCYCLE_XY*	m_CycleXY;
	PTCYCLE*	m_CycleXZ;
	PTCYCLE*	m_CycleYZ;

	CPoint3D	m_ptValI,		// 前回位置と１点目の穴加工座標
				m_ptValR;
	CPoint		m_ptDrawI, m_ptDrawR,		// 拡大係数
				m_ptDrawI_XZ, m_ptDrawR_XZ,
				m_ptDrawI_YZ, m_ptDrawR_YZ;

	double		m_dInitial,		// ｲﾆｼｬﾙ点の記憶
				m_dCycleMove,	// 移動距離(切削距離はm_nc.dLength)
				m_dDwell;		// ﾄﾞｳｪﾙ時間

public:
	CNCcycle(const CNCdata*, LPNCARGV);
	virtual ~CNCcycle();

	int		GetDrawCnt(void) const;
	const	PTCYCLE_XY*	GetCycleInsideXY(void) const;
	const	PTCYCLE*	GetCycleInsideXZ(void) const;
	const	PTCYCLE*	GetCycleInsideYZ(void) const;
	const	CPoint3D	GetIPoint(void) const;
	const	CPoint3D	GetRPoint(void) const;
	double	GetInitialValue(void) const;
	double	GetCycleMove(void) const;
	double	GetDwell(void) const;

	virtual	void	DrawTuning(double f);
	virtual	void	DrawTuningXY(double f);
	virtual	void	DrawTuningXZ(double f);
	virtual	void	DrawTuningYZ(double f);
	virtual	void	Draw(CDC* pDC);
	virtual	void	DrawXY(CDC* pDC);
	virtual	void	DrawXZ(CDC* pDC);
	virtual	void	DrawYZ(CDC* pDC);

	virtual	BOOL	CalcRoundPoint(const CNCdata*, double, CPointD&, double&, double&);
	virtual	BOOL	SetChamferingPoint(BOOL, double, CPointD&);
	virtual	double	CalcBetweenAngle(const CPoint3D&, const CNCdata*);
	virtual	int		CalcOffsetSign(const CPoint3D&);
	virtual	CPointD	CalcPerpendicularPoint(const CPoint3D&, const CPoint3D&, double, int);
	virtual	CPointD	CalcOffsetIntersectionPoint(const CPoint3D&, const CNCdata*, double, int, int);
	virtual	CPointD	CalcOffsetIntersectionPoint2(const CPoint3D&, const CNCdata*, double, int, int);
	virtual	BOOL	SetCorrectPoint(BOOL, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// G2,G3 円弧補間ｸﾗｽ
class	CNCcircle;
enum	EN_NCCIRCLEDRAW
	{NCCIRCLEDRAW_XYZ, NCCIRCLEDRAW_XY, NCCIRCLEDRAW_XZ, NCCIRCLEDRAW_YZ};
typedef void (CNCcircle::*PFNCIRCLEDRAW)(EN_NCCIRCLEDRAW, CDC*);

class CNCcircle : public CNCdata  
{
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
	void		Draw_G17(EN_NCCIRCLEDRAW, CDC*);
	void		Draw_G18(EN_NCCIRCLEDRAW, CDC*);
	void		Draw_G19(EN_NCCIRCLEDRAW, CDC*);

	// IJK指定なしの時，円の方程式から中心の算出
	BOOL	CalcCenter(const CPointD&, const CPointD&);
	void	SetCenter(const CPointD&);
	// 平面座標からの角度計算と調整
	void	AngleTuning(const CPointD&, const CPointD&);
	// 切削長の計算
	void	CalcLength(void);

protected:
	// 外接する四角形
	void	SetMaxRect(void);

public:
	CNCcircle(const CNCdata*, LPNCARGV);

	int		GetG23(void) const;
	const	CPoint3D	GetOrg(void) const;
	double	GetR(void) const;
	double	GetStartAngle(void) const;
	double	GetEndAngle(void) const;

	virtual	void	DrawTuning(double f);
	virtual	void	DrawTuningXY(double f);
	virtual	void	DrawTuningXZ(double f);
	virtual	void	DrawTuningYZ(double f);
	virtual	void	Draw(CDC* pDC);
	virtual	void	DrawXY(CDC* pDC);
	virtual	void	DrawXZ(CDC* pDC);
	virtual	void	DrawYZ(CDC* pDC);

	virtual	BOOL	CalcRoundPoint(const CNCdata*, double, CPointD&, double&, double&);
	virtual	BOOL	SetChamferingPoint(BOOL, double, CPointD&);
	virtual	double	CalcBetweenAngle(const CPoint3D&, const CNCdata*);
	virtual	int		CalcOffsetSign(const CPoint3D&);
	virtual	CPointD	CalcPerpendicularPoint(const CPoint3D&, const CPoint3D&, double, int);
	virtual	CPointD	CalcOffsetIntersectionPoint(const CPoint3D&, const CNCdata*, double, int, int);
	virtual	CPointD	CalcOffsetIntersectionPoint2(const CPoint3D&, const CNCdata*, double, int, int);
	virtual	BOOL	SetCorrectPoint(BOOL, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀのﾌﾞﾛｯｸﾃﾞｰﾀｸﾗｽ
class CNCblock
{
	CString		m_strLine,	// 行番号
				m_strGcode;	// Gｺｰﾄﾞ文字列
	DWORD		m_dwFlags;	// ﾌﾞﾛｯｸにﾘﾝｸしたﾌﾗｸﾞ
	CNCdata*	m_pData;	// ﾌﾞﾛｯｸに対応した最後のCNCdataｵﾌﾞｼﾞｪｸﾄ
	int			m_nArray;	// CNCDoc::m_obGdata 内の番号

public:
	CNCblock(CString strLine, CString strBlock, DWORD dwFlags = 0);

	CString	GetStrLine(void) const;
	CString	GetStrGcode(void) const;
	CString	GetStrBlock(void) const;
	DWORD	GetBlockFlag(void) const;
	void	SetBlockFlag(DWORD dwFlags, BOOL bAdd = TRUE);
	CNCdata*	GetBlockToNCdata(void) const;
	int		GetBlockToNCdataArrayNo(void) const;
	void	SetBlockToNCdata(CNCdata* pData, int nArray);
};

typedef	CTypedPtrArray<CPtrArray, CNCblock*>	CNCblockArray;

#include "NCdata.inl"

#endif // !defined(AFX_NCDATA_H__E80DE2E0_814F_11D3_B0D5_004005691B12__INCLUDED_)
