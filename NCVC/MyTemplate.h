// MyTemplate.h
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "boost/operators.hpp"		// ���Z�q�̎蔲����`
#include "boost/math/constants/constants.hpp"	// PI�̒�`

//////////////////////////////////////////////////////////////////////

// ��ʒ�`
const float PI  = boost::math::constants::pi<float>();	// 3.141592...
const float PI2 = 2.0f*PI;

// �~��64(360�x/64��5.6�x)�����ŕ`��
const int	ARCCOUNT = 64;
const float	ARCSTEP  = PI/32.0f;	// 2��[rad]��ARCCOUNT

//////////////////////////////////////////////////////////////////////

// Radian�ϊ�
inline float RAD(float dVal)
{
	return dVal * PI / 180.0f;
}
inline double RAD(double dVal)
{
	return dVal * boost::math::constants::pi<double>() / 180.0;
}
// Degree�ϊ�
inline	float	DEG(float dVal)
{
	return dVal * 180.0f / PI;
}
inline	double	DEG(double dVal)
{
	return dVal * 180.0 / boost::math::constants::pi<double>();
}
// ��̫�Ẳ�]�p�x
const float RX = RAD(-60.0f);
const float RY = 0.0f;
const float RZ = RAD(-35.0f);

//	�l�̌ܓ��Ɛ؂�̂�
class DECIMALPOINT
{
	float	RoundUp(float dVal, float m) {
		return copysign( floor(fabs(dVal) * m + 0.5f) / m, dVal );
	}
	float	RoundCt(float dVal, float m) {
		return copysign( floor(fabs(dVal) * m) / m, dVal );
	}

public:
	DECIMALPOINT() {
		SetDecimal3();
	}
	void	SetDecimal3(void);
	void	SetDecimal4(void);
	//
	float	RoundUp3(float dVal) {
		return RoundUp(dVal, 1000.0f);
	}
	float	RoundUp4(float dVal) {
		return RoundUp(dVal, 10000.0f);
	}
	float	RoundCt3(float dVal) {
		return RoundCt(dVal, 1000.0f);
	}
	float	RoundCt4(float dVal) {
		return RoundCt(dVal, 10000.0f);
	}
};
extern	boost::function<float(float)>	RoundUp;
extern	boost::function<float(float)>	RoundCt;
extern	float			NCMIN;	// NC�̌������덷�iDECIMALPOINT�ŃZ�b�g�j
extern	UINT			IDS_MAKENCD_FORMAT,
						IDS_MAKENCD_CIRCLEBREAK,
						IDS_MAKENCD_LATHEDRILL;
extern	DECIMALPOINT	_dp;

//////////////////////////////////////////////////////////////////////
// �����^ CPoint �̐��`

template<typename T>
class CPointT :
	// +=, -=, *=, /= �� +, -, * / ��������`
	boost::operators< CPointT<T> >
{
public:
	union {
		struct {
			T	x, y;
		};
		T	xy[2];
	};

	// �R���X�g���N�^
	CPointT() {
		x = y = 0;
	}
	CPointT(T xy) {
		x = y = xy;
	}
	CPointT(T xx, T yy) {
		SetPoint(xx, yy);
	}
	CPointT(const CPoint& pt) {
		SetPoint((T)pt.x, (T)pt.y);
	}
	CPointT(const CPointT<double>& pt) {
		// double -> float ������
		SetPoint((T)pt.x, (T)pt.y);
	}
	// ���Z�q��`
	CPointT<T>&	operator = (T xy) {
		x = y = xy;
		return *this;
	}
	CPointT<T>&	operator = (const CPoint& pt) {
		SetPoint((T)pt.x, (T)pt.y);
		return *this;
	}
	CPointT<float>&	operator = (const CPointT<double>& pt) {
		SetPoint((T)pt.x, (T)pt.y);
		return *this;
	}
	CPointT<T>&	operator += (T d) {
		x += d;		y += d;
		return *this;
	}
	CPointT<T>&	operator += (const CPointT<T>& pt) {
		x += pt.x;		y += pt.y;
		return *this;
	}
	CPointT<T>&	operator -= (T d) {
		x -= d;		y -= d;
		return *this;
	}
	CPointT<T>&	operator -= (const CPointT<T>& pt) {
		x -= pt.x;		y -= pt.y;
		return *this;
	}
	CPointT<T>&	operator *= (T d) {
		x *= d;		y *= d;
		return *this;
	}
	CPointT<T>&	operator *= (const CPointT<T>& pt) {
		x *= pt.x;		y *= pt.y;
		return *this;
	}
	CPointT<T>&	operator /= (T d) {
		x /= d;		y /= d;
		return *this;
	}
	CPointT<T>&	operator /= (const CPointT<T>& pt) {
		x /= pt.x;		y /= pt.y;
		return *this;
	}
	BOOL		operator == (T pt) const {
		return ( fabs(x-pt)<NCMIN && fabs(y-pt)<NCMIN );
	}
	BOOL		operator == (const CPointT& pt) const {
		return IsMatchPoint(&pt);
	}
	BOOL	IsMatchPoint(const CPointT* pt) const {
		CPointT<T>	ptw(x - pt->x, y - pt->y);
		return ptw.hypot() < NCMIN;
	}
	T&		operator[] (size_t a) {
		ASSERT(a>=0 && a<SIZEOF(xy));
		return xy[a];
	}
	T		operator[] (size_t a) const {
		ASSERT(a>=0 && a<SIZEOF(xy));
		return xy[a];
	}
	T		hypot(void) const {
		return ::sqrt(x*x + y*y);
	}
	// �ϊ��֐�
	operator CPoint() const {
		return CPoint((int)x, (int)y);
	}
	CPointT<T>	RoundUp(void) const {
		return CPointT<T>(::RoundUp(x), ::RoundUp(y));
	}
	// ����֐�
	void	SetPoint(T xx, T yy) {
		x = xx;		y = yy;
	}
	// ���W��]
	void	RoundPoint(T q) {
		T	cos_q = cos(q), sin_q = sin(q);
		CPointT<T>	pt(x, y);
		x = pt.x * cos_q - pt.y * sin_q;
		y = pt.x * sin_q + pt.y * cos_q;
	}
	// �p�x�iatan2(pt.y, pt.x)������ԏȗ��j
	T		arctan(void) const {
		return atan2(y, x);
	}
	// ��������_�Ƃ����w��_�ւ̊p�x
	T		arctan(const CPointT<T> e) const {
		return atan2(e.y-y, e.x-x);
	}
};
typedef	CPointT<double>			CPointD;
typedef	CPointT<float>			CPointF;
typedef	std::vector<CPointF>	CVPointF;
//BOOST_GEOMETRY_REGISTER_POINT_2D(CPointF, float,  cs::cartesian, x, y)
//BOOST_GEOMETRY_REGISTER_POINT_2D(CPointD, double, cs::cartesian, x, y)
/*
template<typename T> inline T CPointT<T>::hypot(void) const
{
	return ::hypotf(x, y);	// hypot()�͂Ȃ���float��double�ŋ��
}
template<> inline double CPointT<double>::hypot(void) const
{
	return ::hypot(x, y);
}
*/
//////////////////////////////////////////////////////////////////////
// 3D-CPointD �N���X

template<typename T>
class CPoint3T :
	boost::operators< CPoint3T<T> >
{
	static	T	ms_rx_cos, ms_rx_sin,		// ��]�p�x
				ms_ry_cos, ms_ry_sin,
				ms_rz_cos, ms_rz_sin;
public:
	union {
		struct {
			T	x, y, z;
		};
		T	xyz[3];
	};

	// �R���X�g���N�^
	CPoint3T() {
		x = y = z = 0;
	}
	CPoint3T(T xyz) {
		x = y = z = xyz;
	}
	CPoint3T(T xx, T yy, T zz) {
		SetPoint(xx, yy, zz);
	}
	CPoint3T(const CPointT<T> pt, T zz = 0) {
		SetPoint(pt.x, pt.y, zz);
	}
	CPoint3T(const CPoint3T<double>& pt) {
		// double -> float ������
		SetPoint((T)pt.x, (T)pt.y, (T)pt.z);
	}
	// ���Z�q��`
	CPoint3T<T>&	operator = (T xyz) {
		x = y = z = xyz;
		return *this;
	}
	CPoint3T<float>& operator = (const CPoint3T<double>& pt) {
		x = (float)pt.x;	y = (float)pt.y;	z = (float)pt.z
		return *this;
	}
	CPoint3T<T>&	operator += (T d) {
		x += d;		y += d;		z += d;
		return *this;
	}
	CPoint3T<T>&	operator += (const CPoint3T<T>& pt) {
		x += pt.x;		y += pt.y;		z += pt.z;
		return *this;
	}
	CPoint3T<T>&	operator -= (T d) {
		x -= d;		y -= d;		z -= d;
		return *this;
	}
	CPoint3T<T>&	operator -= (const CPoint3T<T>& pt) {
		x -= pt.x;		y -= pt.y;		z -= pt.z;
		return *this;
	}
	CPoint3T<T>&	operator *= (T d) {
		x *= d;		y *= d;		z *= d;
		return *this;
	}
	CPoint3T<T>&	operator *= (const CPoint3T<T>& pt) {
		x *= pt.x;	y *= pt.y;	z *= pt.z;
		return *this;
	}
	CPoint3T<T>&	operator /= (T d) {
		x /= d;		y /= d;		z /= d;
		return *this;
	}
	CPoint3T<T>&	operator /= (const CPoint3T<T>& pt) {
		x /= pt.x;	y /= pt.y;	z /= pt.z;
		return *this;
	}
	BOOL		operator == (T pt) const {
		return ( fabs(x-pt)<NCMIN && fabs(y-pt)<NCMIN && fabs(z-pt)<NCMIN );
	}
	BOOL		operator == (const CPoint3T<T>& pt) const {
		return IsMatchPoint(&pt);
	}
	BOOL	IsMatchPoint(const CPoint3T<T>* pt) const {
		CPoint3T<T>	ptw(x - pt->x, y - pt->y, z - pt->z);
		return ptw.hypot() < NCMIN;
	}
	T&		operator[] (size_t a) {
		ASSERT(a>=0 && a<SIZEOF(xyz));
		return xyz[a];
	}
	T		operator[] (size_t a) const {
		ASSERT(a>=0 && a<SIZEOF(xyz));
		return xyz[a];
	}
	T		hypot(void) const {
		return ::sqrt(x*x + y*y + z*z);
	}
	// �ϊ��֐�
	CPointT<T>	GetXY(void) const {
		return CPointT<T>(x, y);
	}
	CPointT<T>	GetXZ(void) const {
		return CPointT<T>(x, z);
	}
	CPointT<T>	GetYZ(void) const {
		return CPointT<T>(y, z);
	}
	operator CPointT<T>() const {
		return GetXY();
	}
	// ����֐�
	void	SetPoint(T xx, T yy, T zz) {
		x = xx;		y = yy;		z = zz;
	}
	// �p�x�iatan2(pt.y, pt.x)������ԏȗ� 2D only�j
	T		arctan(void) const {
		return atan2(y, x);
	}
	// ��������_�Ƃ����w��_�ւ̊p�x(2D only)
	T		arctan(const CPoint3T<T> e) const {
		return atan2(e.y-y, e.x-x);
	}
	// 2D�ϊ�
	CPointT<T>	PointConvert(void) const {
		// �y�����̉�]�ψ�
		CPoint3T<T>	pt1(x*ms_rz_cos-y*ms_rz_sin, x*ms_rz_sin+y*ms_rz_cos, z);
		// �w�����̉�]�ψ�
		CPoint3T<T>	pt2(pt1.x, pt1.y*ms_rx_cos-pt1.z*ms_rx_sin, pt1.y*ms_rx_sin+pt1.z*ms_rx_cos);
		// �x�����̉�]�ψ�(�ŏI) // X-Z���ʂɓ��e
		return CPointT<T>(pt2.x*ms_ry_cos+pt2.z*ms_ry_sin, pt2.y);
	}
	static	void	ChangeAngle(T rx, T ry, T rz) {
		ms_rx_cos = cos(rx);		ms_rx_sin = sin(rx);
		ms_ry_cos = cos(ry);		ms_ry_sin = sin(ry);
		ms_rz_cos = cos(rz);		ms_rz_sin = sin(rz);
	}
};
typedef	CPoint3T<double>	CPoint3D;
typedef	CPoint3T<float>		CPoint3F;
typedef	std::vector<CPoint3F>	CVPoint3F;
//BOOST_GEOMETRY_REGISTER_POINT_3D(CPoint3F, float,  cs::cartesian, x, y, z)
//BOOST_GEOMETRY_REGISTER_POINT_3D(CPoint3D, double, cs::cartesian, x, y, z)

//////////////////////////////////////////////////////////////////////
// �����^�� CRect�׽

template<class T>
class CRectT :
	boost::operators< CRectT<T> >
{
public:
	union {
		struct {
			T	left, top, right, bottom;
		};
		T	rect[4];
	};

	// �������̂��߂̺ݽ�׸�
	CRectT() {
		SetRectEmpty();
	}
	CRectT(T l, T t, T r, T b) {
		SetRect(l, t, r, b);
	}
	CRectT(const CRect& rc) {
		SetRect((T)rc.left, (T)rc.top, (T)rc.right, (T)rc.bottom);
	}
	CRectT(const CPointT<T>& ptTopLeft, const CPointT<T>& ptBottomRight) {
		TopLeft() = ptTopLeft;
		BottomRight() = ptBottomRight;
		NormalizeRect();
	};
	CRectT(const CRectT<double>& rc) {
		SetRect((T)rc.left, (T)rc.top, (T)rc.right, (T)rc.bottom);
	}
	// ���ƍ����̐��K��
	void	NormalizeRect() {
		if ( left > right )
			boost::swap(left, right);
		if ( top > bottom )
			boost::swap(top, bottom);
	}
	// �w����W����`���ɂ��邩(��`������܂߂�)
	BOOL	PtInRect(const CPointT<T>& pt) const {
		return left<=pt.x && pt.x<=right && top<=pt.y && pt.y<=bottom;
	}
	// ���͊댯
//	BOOL	PtInRect(const CRectT<T>& rc) const {
//		return PtInRect(rc.TopLeft()) || PtInRect(rc.BottomRight());
//	}
	// �w���`����`���Ɋ��S�Ɋ܂܂�邩
	BOOL	RectInRect(const CRectT<T>& rc) const {
		return PtInRect(rc.TopLeft()) && PtInRect(rc.BottomRight());
	}
	BOOL	RectInRect(const CRectT<T>* rc) const {
		return PtInRect(rc->TopLeft()) && PtInRect(rc->BottomRight());
	}
	// 2�̎l�p�`������镔���ɑ�������l�p�`��ݒ�
	BOOL	CrossRect(const CRectT<T>& rc1, const CRectT<T>& rc2) {
		left	= max(rc1.left,		rc2.left);
		top		= max(rc1.top,		rc2.top);
		right	= min(rc1.right,	rc2.right);
		bottom	= min(rc1.bottom,	rc2.bottom);
		if ( rc1.RectInRect(this) )
			return TRUE;
		SetRectEmpty();
		return FALSE;
	}
	// ���S���W...��
	CPointT<T>	CenterPoint(void) const {
		return CPointT<T>( (left+right)/2, (top+bottom)/2 );
	}
	T	Width(void) const {
		return right - left;
	}
	T	Height(void) const {
		return bottom - top;
	}
	const CPointT<T>&	TopLeft(void) const {
		return *((CPointT<T> *)rect);
	}
	CPointT<T>&	TopLeft(void) {
		return *((CPointT<T> *)rect);
	}
	const CPointT<T>&	BottomRight(void) const {
		return *(((CPointT<T> *)rect)+1);
	}
	CPointT<T>&	BottomRight(void) {
		return *(((CPointT<T> *)rect)+1);
	}
	// �w�肳�ꂽ�̾�Ă� CRectT ���ړ�
	void	OffsetRect(T x, T y) {
		left	+= x;		right	+= x;
		top		+= y;		bottom	+= y;
	}
	void	OffsetRect(const CPointT<T>& pt) {
		left	+= pt.x;	right	+= pt.x;
		top		+= pt.y;	bottom	+= pt.y;
	}
	// �e�ӂ𒆐S����O���Ɍ������Ĉړ�
	void	InflateRect(T w, T h) {
		left	-= w;		right	+= w;
		top		-= h;		bottom	+= h;
	}
	// ���Z�q��`
	CRectT<float>& operator = (const CRectT<double>& rc) {
		SetRect((float)rc.left, (float)rc.top, (float)rc.right, (float)rc.bottom);
		return *this;
	}
	BOOL	operator == (const CRectT<T>& rc) const {
		return	left   == rc.left &&
				top    == rc.top &&
				right  == rc.right &&
				bottom == rc.bottom;
	}
	CRectT<T>&	operator /= (T d) {
		left /= d;		right /= d;
		top /= d;		bottom /= d;
		return *this;
	}
	CRectT<T>&	operator |= (const CRectT<T>& rc) {	// ��`�����킹��(����)
		if ( left   > rc.left )		left = rc.left;
		if ( top    > rc.top )		top = rc.top;
		if ( right  < rc.right )	right = rc.right;
		if ( bottom < rc.bottom )	bottom = rc.bottom;
		return *this;
	}
	CRectT<T>&	operator |= (const CPointT<T>& pt) {
		if ( left   > pt.x )		left = pt.x;
		if ( top    > pt.y )		top = pt.y;
		if ( right  < pt.x )		right = pt.x;
		if ( bottom < pt.y )		bottom = pt.y;
		return *this;
	}
	T&		operator[] (size_t a) {
		ASSERT(a>=0 && a<SIZEOF(rect));
		return rect[a];
	}
	T		operator[] (size_t a) const {
		ASSERT(a>=0 && a<SIZEOF(rect));
		return rect[a];
	}
	// ����֐�
	void	SetRect(const CRectT<T>* rc) {
		TopLeft() = rc->TopLeft();
		BottomRight() = rc->BottomRight();
	}
	void	SetRect(T l, T t, T r, T b) {
		left	= l;		top		= t;
		right	= r;		bottom	= b;
	}
	void	SetRect(const CPointT<T> pt, T w, T h) {
		TopLeft() = pt;
		right = left + w;
		bottom = top + h;
	}
	// �ϊ��֐�
	operator	CRect() const {
		return CRect((int)left, (int)top, (int)right, (int)bottom);
	}
	// ������
	void	SetRectEmpty(void) {
		left = top = right = bottom = 0;
	}
	void	SetRectMinimum(void);
};
typedef	CRectT<double>	CRectD;
typedef	CRectT<float>	CRectF;
//BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES(CRectF, CPointF, left, top, right, bottom)
//BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES(CRectD, CPointD, left, top, right, bottom)
template<typename T> inline void CRectT<T>::SetRectMinimum(void)
{
	left  = top    =  FLT_MAX;
	right = bottom = -FLT_MAX;
}
template<> inline void CRectT<double>::SetRectMinimum(void)
{
	left  = top    =  DBL_MAX;
	right = bottom = -DBL_MAX;
}

//////////////////////////////////////////////////////////////////////
// CRect�R������`�׽

template<class T>
class CRect3T : public CRectT<T>,
		boost::operators< CRect3T<T> >
{
public:
	T	high, low;		// ����

	// �������̂��߂̺ݽ�׸�
	CRect3T() : CRectT<T>() {
		high = low = 0.0;	// CRectT::SetRectEmpty() ���ް��׽�ŌĂ΂��
	}
	CRect3T(T l, T t, T r, T b, T h, T w) :
			CRectT<T>(l, t, r, b) {
		high = h;	low = w;
	}
	// ���ƍ����̐��K��
	void	NormalizeRect() {
		CRectT<T>::NormalizeRect();
		if ( low > high )
			boost::swap(low, high);
	}
	// ���S���W...��
	CPoint3T<T>	CenterPoint(void) const {
		return CPoint3T<T>( (left+right)/2, (top+bottom)/2, (high+low)/2 );
	}
	T	Depth(void) const {
		return high - low;
	}
	// �w�肳�ꂽ�̾�Ă� CRectD ���ړ�
	void	OffsetRect(T x, T y, T z) {
		CRectT<T>::OffsetRect(x, y);
		high += z;		low += z;
	}
	void	OffsetRect(const CPoint3T<T>& pt) {
		CRectT<T>::OffsetRect(pt.GetXY());
		high += pt.z;	low += pt.z;
	}
	// �e�ӂ𒆐S����O���Ɍ������Ĉړ�
	void	InflateRect(T w, T h) {
		CRectT<T>::InflateRect(w, h);
	}
	void	InflateRect(T w, T h, T t) {
		CRectT<T>::InflateRect(w, h);
		high += t;		low  -= t;
	}
	// ���Z�q��`
	CRect3T<float>& operator = (const CRect3T<double>& rc) {
		SetRect((float)rc.left, (float)rc.top, (float)rc.right, (float)rc.bottom, (float)rc.high, (float)rc.low);
		return *this;
	}
	BOOL	operator == (const CRect3T<T>& rc) const {
		return	CRectT<T>::operator ==(rc) &&
				high == rc.high &&
				low  == rc.low;
	}
	CRect3T<T>&	operator /= (T d) {
		CRectT<T>::operator /= (d);
		high /= d;		low /= d;
		return *this;
	}
	CRect3T<T>&	operator |= (const CRect3T<T>& rc) {
		CRectT<T>::operator |= (rc);
		if ( low  > rc.low )	low = rc.low;
		if ( high < rc.high )	high = rc.high;
		return *this;
	}
	CRect3T<T>& operator |= (const CPoint3T<T>& pt) {
		CRectT<T>::operator |= (pt);
		if ( low  > pt.z )		low = pt.z;
		if ( high < pt.z )		high = pt.z;
		return *this;
	}
	// ����֐�
	void	SetRect(const CRect3T<T>* rc) {
		CRectT<T>::SetRect(rc);
		high = rc->high;	low  = rc->low;
	}
	void	SetRect(T l, T t, T r, T b, T h, T w) {
		CRectT<T>::SetRect(l, t, r, b);
		high = h;			low = w;
	}
	// ������
	void	SetRectEmpty(void) {
		CRectT<T>::SetRectEmpty();
		high = low = 0;
	}
	void	SetRectMinimum(void);
};
typedef	CRect3T<double>	CRect3D;
typedef	CRect3T<float>	CRect3F;
template<typename T> inline void CRect3T<T>::SetRectMinimum(void)
{
	CRectT<T>::SetRectMinimum();
	high  = -FLT_MAX;
	low   =  FLT_MAX;
}
template<> inline void CRect3T<double>::SetRectMinimum(void)
{
	CRectD::SetRectMinimum();
	high  = -DBL_MAX;
	low   =  DBL_MAX;
}

//////////////////////////////////////////////////////////////////////
// NCVC���l���Z���ʊ֐�

//	�ެ��ߌv�Z�̲�ײ݊֐�
//template<typename T> inline	T	GAPCALC(T x, T y)
//template<> inline	double	GAPCALC(double x, double y)
inline	float	GAPCALC(float x, float y)
{
//	return	_hypot(x, y);	// ���Ԃ������肷��
	return	x*x + y*y;		// ����������炸�Q��ŏ���
}
inline	double	GAPCALC(double x, double y)
{
	return	x*x + y*y;
}
inline	float	GAPCALC(const CPointF& pt)
{
	return GAPCALC(pt.x, pt.y);
}
inline	double	GAPCALC(const CPointD& pt)
{
	return GAPCALC(pt.x, pt.y);
}

//	�̾�ĕ���(�i�s��������)
inline int	CalcOffsetSign(const CPointF& pt)
{
	float	dAngle = RoundUp( DEG(pt.arctan()) );
	return ( dAngle < -90.0f || 90.0f <= dAngle ) ? -1 : 1;
}
/*
// (sx,sy)-(ex,ey) ���� ax+by+c=0 �̌W�������߂�
inline boost::tuple<float, float, float> CalcLineABC(const CPointF& pts, const CPointF& pte)
{
	float	a = pte.y - pts.y,
			b = pte.x - pts.x,
			c = pte.x*pts.y - pts.x*pte.y;
	boost::make_tuple(a, b, c);
}
*/
//	�Q���̌�_�����߂�
boost::optional<CPointF>	CalcIntersectionPoint_LL
	(const CPointF&, const CPointF&, const CPointF&, const CPointF&, BOOL = TRUE);
//	�����Ɖ~�̌�_�����߂�
boost::tuple<int, CPointF, CPointF>	CalcIntersectionPoint_LC
	(const CPointF&, const CPointF&, const CPointF&, float, BOOL = TRUE);
//	�Q�̉~�̌�_�����߂�
boost::tuple<int, CPointF, CPointF>	CalcIntersectionPoint_CC
	(const CPointF&, const CPointF&, float, float);
//	�����Ƒȉ~�̌�_�����߂�
boost::tuple<int, CPointF, CPointF>	CalcIntersectionPoint_LE
	(const CPointF&, const CPointF&, const CPointF&, float, float, float, BOOL = TRUE);
//	���Ɛ��̒[�_�𒆐S�Ƃ����~�Ƃ̌�_�����߂�
CPointF	CalcIntersectionPoint_TC(const CPointF&, float, const CPointF&);
//	�����̵̾�č��W
boost::tuple<CPointF, CPointF> CalcOffsetLine(const CPointF&, const CPointF&, float, BOOL);
//	�Q�����Ȃ��p�x�����߂�(��_�����_��ۈ���)
float	CalcBetweenAngle(const CPointF&, const CPointF&);
//	�̾�ĕ����s�ړ��������������m�̌�_�����߂�
boost::optional<CPointF>	CalcOffsetIntersectionPoint_LL
	(const CPointF&, const CPointF&, float, float, BOOL);
//	�̾�ĕ����s�ړ����������Ɖ~�ʂ̌�_�����߂�
boost::optional<CPointF>	CalcOffsetIntersectionPoint_LC
	(const CPointF&, const CPointF&, float, float, float, BOOL, BOOL);
//	�̾�ĕ����s�ړ����������Ƒȉ~�ʂ̌�_�����߂�
boost::optional<CPointF>	CalcOffsetIntersectionPoint_LE
	(const CPointF&, const CPointF&, float, float, float, float, BOOL, BOOL);
//	�_�����p�`�̓������ۂ�
BOOL IsPointInPolygon(const CPointF&, const CVPointF&);
//	�_�ƒ����̋��������߂�
float	CalcLineDistancePt(const CPointF&, const CPointF&, const CPointF&);

//	�Q���������̉������߂�
boost::tuple<int, float, float>	GetKon(float, float, float);
//	�������z����f����Ԃ�
UINT	GetPrimeNumber(UINT);
