// 3Dto2D.h
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "boost/operators.hpp"		// 演算子の手抜き定義

const double EPS = 0.0005;			// NCの計算誤差
const double NCMIN = 0.001;			// NCの桁落ち誤差
const double PI = 3.1415926535897932384626433832795;
const double RAD = PI/180.0;
const double DEG = 180.0/PI;

const double RX = -60.0*RAD;		// ﾃﾞﾌｫﾙﾄの回転角度
const double RY =   0.0*RAD;
const double RZ = -35.0*RAD;

// 1/1000 四捨五入
inline	double	RoundUp(double dVal)
{
	return _copysign( floor(fabs(dVal) * 1000.0 + 0.5) / 1000.0, dVal );
}

//////////////////////////////////////////////////////////////////////
// double型の CPointクラス

class CPointD :
	// +=, -=, *=, /= で +, -, * / も自動定義
	boost::arithmetic			< CPointD,
	boost::arithmetic			< CPointD, double,
	boost::equality_comparable	< CPointD,
	boost::equality_comparable	< CPointD, double
	> > > >
{
public:
	union {
		struct {
			double	x, y;
		};
		double	xy[2];
	};

	// コンストラクタ
	CPointD() {
		x = y = 0;
	}
	CPointD(double xy) {
		x = y = xy;
	}
	CPointD(double xx, double yy) {
		SetPoint(xx, yy);
	}
	CPointD(const CPoint& pt) {
		SetPoint(pt.x, pt.y);
	}
	// 演算子定義
	CPointD&	operator = (double xy) {
		x = y = xy;
		return *this;
	}
	CPointD&	operator = (const CPoint& pt) {
		SetPoint(pt.x, pt.y);
		return *this;
	}
	CPointD&	operator += (double d) {
		x += d;		y += d;
		return *this;
	}
	CPointD&	operator += (const CPointD& pt) {
		x += pt.x;		y += pt.y;
		return *this;
	}
	CPointD&	operator -= (double d) {
		x -= d;		y -= d;
		return *this;
	}
	CPointD&	operator -= (const CPointD& pt) {
		x -= pt.x;		y -= pt.y;
		return *this;
	}
	CPointD&	operator *= (double d) {
		x *= d;		y *= d;
		return *this;
	}
	CPointD&	operator *= (const CPointD& pt) {
		x *= pt.x;		y *= pt.y;
		return *this;
	}
	CPointD&	operator /= (double d) {
		x /= d;		y /= d;
		return *this;
	}
	CPointD&	operator /= (const CPointD& pt) {
		x /= pt.x;		y /= pt.y;
		return *this;
	}
	BOOL		operator == (double pt) const {
		return ( fabs(x-pt)<EPS && fabs(y-pt)<EPS );
	}
	BOOL		operator == (const CPointD& pt) const {
		return IsMatchPoint(&pt);
	}
	BOOL	IsMatchPoint(const CPointD* pt) const {
		return ( fabs(x-pt->x)<EPS && fabs(y-pt->y)<EPS );
	}
	double&		operator[] (size_t a) {
		ASSERT(a>=0 && a<SIZEOF(xy));
		return xy[a];
	}
	double		operator[] (size_t a) const {
		ASSERT(a>=0 && a<SIZEOF(xy));
		return xy[a];
	}
	double		hypot(void) const {
		return ::_hypot(x, y);
	}
	// 変換関数
	operator CPoint() const {
		return CPoint((int)x, (int)y);
	}
	CPointD	RoundUp(void) const {
		return CPointD(::RoundUp(x), ::RoundUp(y));
	}
	// 代入関数
	void	SetPoint(double xx, double yy) {
		x = xx;		y = yy;
	}
	// 座標回転
	void	RoundPoint(double q) {
		double	cosq = cos(q), sinq = sin(q);
		CPointD	pt(x, y);
		x = pt.x * cosq - pt.y * sinq;
		y = pt.x * sinq + pt.y * cosq;
	}
};

//////////////////////////////////////////////////////////////////////
// 3D-CPointD クラス

class CPoint3D :
	boost::arithmetic			< CPoint3D,
	boost::arithmetic			< CPoint3D, double,
	boost::equality_comparable	< CPoint3D,
	boost::equality_comparable	< CPoint3D, double
	> > > >
{
	static	double	ms_rx_cos, ms_rx_sin,		// 回転角度
					ms_ry_cos, ms_ry_sin,
					ms_rz_cos, ms_rz_sin;
public:
	union {
		struct {
			double	x, y, z;
		};
		double	xyz[3];
	};

	// コンストラクタ
	CPoint3D() {
		x = y = z = 0;
	}
	CPoint3D(double xyz) {
		x = y = z = xyz;
	}
	CPoint3D(double xx, double yy, double zz) {
		SetPoint(xx, yy, zz);
	}
	// 演算子定義
	CPoint3D&	operator = (double xyz) {
		x = y = z = xyz;
		return *this;
	}
	CPoint3D&	operator += (double d) {
		x += d;		y += d;		z += d;
		return *this;
	}
	CPoint3D&	operator += (const CPoint3D& pt) {
		x += pt.x;		y += pt.y;		z += pt.z;
		return *this;
	}
	CPoint3D&	operator -= (double d) {
		x -= d;		y -= d;		z -= d;
		return *this;
	}
	CPoint3D&	operator -= (const CPoint3D& pt) {
		x -= pt.x;		y -= pt.y;		z -= pt.z;
		return *this;
	}
	CPoint3D&	operator *= (double d) {
		x *= d;		y *= d;		z *= d;
		return *this;
	}
	CPoint3D&	operator *= (const CPoint3D& pt) {
		x *= pt.x;	y *= pt.y;	z *= pt.z;
		return *this;
	}
	CPoint3D&	operator /= (double d) {
		x /= d;		y /= d;		z /= d;
		return *this;
	}
	CPoint3D&	operator /= (const CPoint3D& pt) {
		x /= pt.x;	y /= pt.y;	z /= pt.z;
		return *this;
	}
	BOOL		operator == (double pt) const {
		return ( fabs(x-pt)<EPS && fabs(y-pt)<EPS && fabs(z-pt)<EPS );
	}
	BOOL		operator == (const CPoint3D& pt) const {
		return IsMatchPoint(&pt);
	}
	BOOL	IsMatchPoint(const CPoint3D* pt) const {
		return ( fabs(x-pt->x)<EPS && fabs(y-pt->y)<EPS && fabs(z-pt->z)<EPS );
	}
	double&		operator[] (size_t a) {
		ASSERT(a>=0 && a<SIZEOF(xyz));
		return xyz[a];
	}
	double		operator[] (size_t a) const {
		ASSERT(a>=0 && a<SIZEOF(xyz));
		return xyz[a];
	}
	double		hypot(void) const {
		return ::sqrt(x*x + y*y + z*z);
	}
	// 変換関数
	CPointD		GetXY(void) const {
		return CPointD(x, y);
	}
	CPointD		GetXZ(void) const {
		return CPointD(x, z);
	}
	CPointD		GetYZ(void) const {
		return CPointD(y, z);
	}
	operator CPointD() const {
		return CPointD(x, y);
	}
	// 代入関数
	void	SetPoint(double xx, double yy, double zz) {
		x = xx;		y = yy;		z = zz;
	}
	// 2D変換
	CPointD		PointConvert(void) const {
		// Ｚ軸回りの回転変位
		CPoint3D	pt1(x*ms_rz_cos-y*ms_rz_sin, x*ms_rz_sin+y*ms_rz_cos, z);
		// Ｘ軸回りの回転変位
		CPoint3D	pt2(pt1.x, pt1.y*ms_rx_cos-pt1.z*ms_rx_sin, pt1.y*ms_rx_sin+pt1.z*ms_rx_cos);
		// Ｙ軸回りの回転変位(最終) // X-Z平面に投影
		return CPointD(pt2.x*ms_ry_cos+pt2.z*ms_ry_sin, pt2.y);
	}
	static	void	ChangeAngle(double rx, double ry, double rz) {
		ms_rx_cos = cos(rx);		ms_rx_sin = sin(rx);
		ms_ry_cos = cos(ry);		ms_ry_sin = sin(ry);
		ms_rz_cos = cos(rz);		ms_rz_sin = sin(rz);
	}
};

//////////////////////////////////////////////////////////////////////
// double型の CRectｸﾗｽ

class CRectD
{
public:
	union {
		struct {
			double	left, top, right, bottom;
		};
		double	rect[4];
	};

	// 初期化のためのｺﾝｽﾄﾗｸﾀ
	CRectD() {
		SetRectEmpty();
	}
	CRectD(double l, double t, double r, double b) {
		left = l;			top = t;
		right = r;			bottom = b;
	}
	CRectD(const CRect& rc) {
		left = rc.left;		top = rc.top;
		right = rc.right;	bottom = rc.bottom;
	}
	CRectD(const CPointD& ptTopLeft, const CPointD& ptBottomRight) {
		TopLeft() = ptTopLeft;
		BottomRight() = ptBottomRight;
		NormalizeRect();
	};
	// 幅と高さの正規化
	void	NormalizeRect() {
		if ( left > right )
			std::swap(left, right);
		if ( top > bottom )
			std::swap(top, bottom);
	}
	// 指定座標が矩形内にあるか(矩形線上を含める)
	BOOL	PtInRect(const CPointD& pt) const {
		return left<=pt.x && pt.x<=right && top<=pt.y && pt.y<=bottom;
	}
	BOOL	PtInRect(const CRectD& rc) const {
		return PtInRect(rc.TopLeft()) && PtInRect(rc.BottomRight());
	}
	BOOL	PtInRectpt(const CRectD& rc) const {
		return PtInRect(rc.TopLeft()) || PtInRect(rc.BottomRight()) ||
				PtInRect(CPointD(rc.left,rc.bottom)) || PtInRect(CPointD(rc.right,rc.top));
	}
	// 2つの四角形が交わる部分に相当する四角形を設定
	BOOL	IntersectRect(const CRectD& rc1, const CRectD& rc2) {
		left	= max(rc1.left, rc2.left);
		top		= max(rc1.top, rc2.top);
		right	= min(rc1.right, rc2.right);
		bottom	= min(rc1.bottom, rc2.bottom);
		if ( rc1.PtInRect(TopLeft()) && rc1.PtInRect(BottomRight()) )
			return TRUE;
		SetRectEmpty();
		return FALSE;
	}
	// 中心座標...他
	CPointD	CenterPoint(void) const {
		return CPointD( (left+right)/2, (top+bottom)/2 );
	}
	double	Width(void) const {
		return right - left;
	}
	double	Height(void) const {
		return bottom - top;
	}
	const CPointD&	TopLeft(void) const {
		return *((CPointD *)rect);
	}
	CPointD&	TopLeft(void) {
		return *((CPointD *)rect);
	}
	const CPointD&	BottomRight(void) const {
		return *(((CPointD *)rect)+1);
	}
	CPointD&	BottomRight(void) {
		return *(((CPointD *)rect)+1);
	}
	// 指定されたｵﾌｾｯﾄで CRectD を移動
	void	OffsetRect(double x, double y) {
		left	+= x;		right	+= x;
		top		+= y;		bottom	+= y;
	}
	void	OffsetRect(const CPointD& pt) {
		left	+= pt.x;	right	+= pt.x;
		top		+= pt.y;	bottom	+= pt.y;
	}
	// 各辺を中心から外側に向かって移動
	void	InflateRect(double w, double h) {
		left	-= w;		right	+= w;
		top		-= h;		bottom	+= h;
	}
	// 演算子定義
	CRectD&	operator /= (double d) {
		left /= d;		right /= d;
		top /= d;		bottom /= d;
		return *this;
	}
	CRectD&	operator |= (const CRectD& rc) {	// 矩形を合わせる(合体)
		if ( left   > rc.left )		left = rc.left;
		if ( top    > rc.top )		top = rc.top;
		if ( right  < rc.right )	right = rc.right;
		if ( bottom < rc.bottom )	bottom = rc.bottom;
		return *this;
	}
	double&		operator[] (size_t a) {
		ASSERT(a>=0 && a<SIZEOF(rect));
		return rect[a];
	}
	double		operator[] (size_t a) const {
		ASSERT(a>=0 && a<SIZEOF(rect));
		return rect[a];
	}
	// 変換関数
	operator	CRect() const {
		return CRect((int)left, (int)top, (int)right, (int)bottom);
	}
	// 初期化
	void	SetRectEmpty(void) {
		left = top = right = bottom = 0;
	}
	void	SetRectMinimum(void) {
		left  = top    =  DBL_MAX;
		right = bottom = -DBL_MAX;
	}
};

//////////////////////////////////////////////////////////////////////
// CRect３次元矩形ｸﾗｽ

class CRect3D : public CRectD
{
public:
	double	high, low;		// 高さ
	// 初期化のためのｺﾝｽﾄﾗｸﾀ
	CRect3D() {
		high = low = 0.0;	// CRectD::SetRectEmpty() は自動で呼ばれる
	}
	CRect3D(double l, double t, double r, double b, double h, double w) :
			CRectD(l, t, r, b) {
		high = h;	low = w;
	}
	// 幅と高さの正規化
	void	NormalizeRect() {
		CRectD::NormalizeRect();
		if ( low > high )
			std::swap(low, high);
	}
	// 中心座標...他
	CPoint3D	CenterPoint(void) const {
		return CPoint3D( (left+right)/2, (top+bottom)/2, (high+low)/2 );
	}
	double	Depth(void) const {
		return high - low;
	}
	// 指定されたｵﾌｾｯﾄで CRectD を移動
	void	OffsetRect(double x, double y, double z) {
		CRectD::OffsetRect(x, y);
		high += z;		low += z;
	}
	void	OffsetRect(const CPointD& pt) {
		CRectD::OffsetRect(pt);
	}
	void	OffsetRect(const CPoint3D& pt) {
		CRectD::OffsetRect(pt.GetXY());
		high += pt.z;	low += pt.z;
	}
	// 演算子定義
	CRect3D&	operator |= (const CRect3D& rc) {
		CRectD::operator |= (rc);
		if ( low  > rc.low )	low = rc.low;
		if ( high < rc.high )	high = rc.high;
		return *this;
	}
	// 初期化
	void	SetRectEmpty(void) {
		CRectD::SetRectEmpty();
		high = low = 0;
	}
	void	SetRectMinimum(void) {
		CRectD::SetRectMinimum();
		high  = -DBL_MAX;
		low   =  DBL_MAX;
	}
};

//////////////////////////////////////////////////////////////////////
// NCVC数値演算共通関数

// ｷﾞｬｯﾌﾟ計算のｲﾝﾗｲﾝ関数
inline	double	GAPCALC(double x, double y)
{
//	return	_hypot(x, y);		// 時間がかかりすぎ
	return	x * x + y * y;		// 平方根を取らず２乗で処理
}
inline	double	GAPCALC(const CPointD& pt)
{
	return pt.x * pt.x + pt.y * pt.y;
}
// ｵﾌｾｯﾄ符号(進行方向左側)
inline int	CalcOffsetSign(const CPointD& pt)
{
	double	dAngle = RoundUp(atan2(pt.y, pt.x)*DEG);
	return (90.0<=dAngle || -90.0>dAngle) ? -1 : 1;
}

// ２線の交点を求める
boost::optional<CPointD>	CalcIntersectionPoint_LL
	(const CPointD&, const CPointD&, const CPointD&, const CPointD&, BOOL = TRUE);
// 直線と円の交点を求める
boost::tuple<int, CPointD, CPointD>	CalcIntersectionPoint_LC
	(const CPointD&, const CPointD&, const CPointD&, double, BOOL = TRUE);
// ２つの円の交点を求める
boost::tuple<int, CPointD, CPointD>	CalcIntersectionPoint_CC
	(const CPointD&, const CPointD&, double, double);
// 直線と楕円の交点を求める
boost::tuple<int, CPointD, CPointD>	CalcIntersectionPoint_LE
	(const CPointD&, const CPointD&, const CPointD&, double, double, double, BOOL = TRUE);
// 線と線の端点を中心とした円との交点を求める
CPointD	CalcIntersectionPoint_TC(const CPointD&, double, const CPointD&);
// 直線のｵﾌｾｯﾄ座標
boost::tuple<CPointD, CPointD> CalcOffsetLine(const CPointD&, const CPointD&, double, BOOL);
// ２線がなす角度を求める(交点を原点ｾﾞﾛ扱い)
double	CalcBetweenAngle(const CPointD&, const CPointD&);
// ｵﾌｾｯﾄ分平行移動させた線分同士の交点を求める
boost::optional<CPointD>	CalcOffsetIntersectionPoint_LL
	(const CPointD&, const CPointD&, int, int, double);
// ｵﾌｾｯﾄ分平行移動させた線と円弧の交点を求める
boost::tuple<BOOL, CPointD, double>	CalcOffsetIntersectionPoint_LC
	(const CPointD&, const CPointD&, double, double, int, int, int);
// ｵﾌｾｯﾄ分平行移動させた線と楕円弧の交点を求める
boost::optional<CPointD>	CalcOffsetIntersectionPoint_LE
	(const CPointD&, const CPointD&, double, double, double, double, BOOL, BOOL);
// ２次方程式の解を求める
boost::tuple<int, double, double>	GetKon(double, double, double);
// 引数を越える素数を返す
UINT	GetPrimeNumber(UINT);
