// DXFdata.h: CDXFdata �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
class	CLayerData;
class	CDXFshape;

// ���̊p�x�ƒ���
#define	ARRAWANGLE		(15.0*RAD)
#define	ARRAWLENGTH		100

// DXF����׸�
#define	DXFFLG_MAKE			0x0001
#define	DXFFLG_SEARCH		0x0002
#define	DXFSEL_SELECT		0x0001
#define	DXFSEL_DIRECTIONFIX	0x0100

// ��޼ު�Đ������̈���
// �e pLayer �́CDataOperation()�� strLayer ���� CLayerData��޼ު�Ă�o�^����
typedef	struct	tagDXF_POINT {
	CLayerData*	pLayer;
	CPointD		c;			// �������ʒu
} DXFPARGV, *LPDXFPARGV;

typedef	struct	tagDXF_LINE {
	CLayerData*	pLayer;
	CPointD		s, e;		// �J�n�E�I���_
} DXFLARGV, *LPDXFLARGV;

typedef	struct	tagDXF_CIRCLE {
	CLayerData*	pLayer;
	CPointD		c;			// ���S
	double		r;			// ���a
} DXFCARGV, *LPDXFCARGV;

typedef	struct	tagDXF_ARC {
	CLayerData*	pLayer;
	CPointD		c;			// ���S
	double		r;			// ���a
	double		sq, eq;		// �n�_�E�I�_�p�x
} DXFAARGV, *LPDXFAARGV;

typedef	struct	tagDXF_ELLIPSE {
	CLayerData*	pLayer;
	CPointD		c;			// ���S
	CPointD		l;			// ����(���S����̑���)
	double		s;			// �Z��(�{��)
	double		sq, eq;		// �n�_�E�I�_�p�x
	BOOL		bRound;		// Default==TRUE(�����v���)
} DXFEARGV, *LPDXFEARGV;

typedef	struct	tagDXF_TEXT {
	CLayerData*	pLayer;
	CPointD		c;			// �����ʒu
	CString		strValue;	// ������
} DXFTARGV, *LPDXFTARGV;

// ��ۯ��̕t�����
#define	DXFBLFLG_X		0x0001
#define	DXFBLFLG_Y		0x0002
#define	DXFBLFLG_Z		0x0004
#define	DXFBLFLG_R		0x0008
typedef	struct	tagDXF_BLOCK {
	DWORD		dwBlockFlg;
	CPointD		ptOrg;			// �}���̾��
	double		dMagni[NCXYZ];	// �e���̔{��
	double		dRound;			// ��]�p�x(�x)
} DXFBLOCK, *LPDXFBLOCK;

// CDXFpoint�p���I�֐��Ăяo��
class	CDXFpoint;
typedef double (*PFNORGDRILLTUNING)(const CDXFpoint*);

// CDXFcircleEx�p��߼������
enum	ENDXFTYPE2
	{DXFORGDATA = 0, DXFSTADATA = 1};

/*
	SwapPt()    : �ߐڍ��W�v�Z�Ɏg�p�D������p(�~���W������ւ�)
	ReversePt() : �����̐[���؍�CPOLYLINE��SwapPt()�D(�~���W�͓���ւ��Ȃ�)
*/

/////////////////////////////////////////////////////////////////////////////
// �b�`�l�f�[�^�̃w�b�_�[�N���X
class CCAMHead : public CObject
{
public:
	CCAMHead() {}
	virtual	void	Serialize(CArchive&);

	DECLARE_SERIAL(CCAMHead)
};

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̃x�[�X�N���X
class CDXFdata : public CObject
{
	ENDXFTYPE	m_enMakeType,	// ��������(�~->�����H�Ȃ�)
				m_enType;		// �ް����� : NCVCdefine.h
	DWORD		m_dwFlags,		// DXF����׸�(Make, Search)
				m_dwSelect,		// DXF�I������׸�(Group)
				m_nSerialSeq;

protected:
	int			m_nPoint;		// m_ptTun, m_ptMake �m�ې�
	CPointD*	m_pt;			// �e��޼ު�ČŗL���W
	CPointD*	m_ptTun;		// ���_������̍��W
	CPointD*	m_ptMake;		// ������R�ʂ܂ł̔�r�p(NC�����ް�)
	CRect3D		m_rcMax;		// ��޼ު�ċ�`
	// CDXFpolyline ������ CDXFpolyline ����X�V����
	CLayerData*	m_pParentLayer;	// �ް��̑�����ڲԏ��
	CDXFshape*	m_pParentMap;	// �ް��̑�����ϯ�ߏ��
	virtual	void	SwapPt(int);	// m_ptTun, m_ptMake �̓���ւ�
	virtual	void	XRev(void);		// X���̕������]
	virtual	void	YRev(void);		// Y���̕������]

	void	OrgTuningBase(void);

	// CDXFpolyline ����̌Ăяo�������邽��
	// CDXF[line|circle]���ނł͂Ȃ��C�ް��׽�ɒ�`����
	BOOL	GetDirectionArraw_Line(const CPointD[], CPointD [][3]) const;
	BOOL	GetDirectionArraw_Circle(BOOL, const double[], const CPointD[], CPointD[][3]) const;

protected:
	CDXFdata(ENDXFTYPE, CLayerData*, int);

public:
	virtual	~CDXFdata();

	static	BOOL		ms_fXRev;		// X�����]���ǂ���
	static	BOOL		ms_fYRev;		// Y�����]���ǂ���
	static	CPointD		ms_ptOrg;		// �؍팴�_
	static	CDXFdata*	ms_pData;		// �P�O�̐����ް�
	static	DWORD		ms_nSerialSeq;	// �رײ�޶���

	ENDXFTYPE	GetType(void) const;
	ENDXFTYPE	GetMakeType(void) const;
	DWORD		GetSerializeSeq(void) const;
	void		ChangeMakeType(ENDXFTYPE);	// �~�ް��ɂ�錊���H�Ȃ�
	CLayerData*	GetParentLayer(void) const;
	CDXFshape*	GetParentMap(void) const;
	void		SetParentMap(CDXFshape*);
	BOOL		IsMakeFlg(void) const;
	BOOL		IsSearchFlg(void) const;
	void		SetMakeFlg(void);
	void		SetSearchFlg(void);
	void		ClearMakeFlg(void);
	void		ClearSearchFlg(void);
	BOOL		IsMoveFlg(void) const;
	DWORD		GetSelectFlg(void) const;
	void		SetSelectFlg(BOOL);
	void		SetWorkingFlag(DWORD, BOOL = TRUE);
	//
	CPen*		GetDrawPen(void) const;
	const CRect3D	GetMaxRect(void) const;
	//
	int			GetPointNumber(void) const;
	const CPointD	GetNativePoint(size_t) const;
	virtual	void	SetNativePoint(size_t, const CPointD&);		// virtual -> CDXFarc
	const CPointD	GetTunPoint(size_t) const;
	const CPointD	GetMakePoint(size_t) const;
	BOOL		IsMatchObject(const CDXFdata*);
	double		GetEdgeGap(const CDXFdata*, BOOL = TRUE);
	//	
	virtual	void	ReversePt(void);
	// �e��޼ު�Ăɂ�������Ȃ��Ǝ��̏��� -> �������z�֐�
	virtual	BOOL	IsMatchPoint(const CPointD&) = 0;
	virtual BOOL	IsStartEqEnd(void) const = 0;	// �n�_�I�_��������޼ު�ĂȂ�TRUE
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE) = 0;
	virtual	const CPointD	GetStartCutterPoint(void) const = 0;// ���H�J�n�ʒu
	virtual	const CPointD	GetStartMakePoint(void) const = 0;
	virtual	const CPointD	GetEndCutterPoint(void) const = 0;	// ���H�I���ʒu(���H�I�_)
	virtual	const CPointD	GetEndMakePoint(void) const = 0;
	virtual	double	GetLength(void) const = 0;
	//
	virtual	void	DrawTuning(double) = 0;
	virtual	void	Draw(CDC*) const = 0;
	virtual	double	OrgTuning(BOOL = TRUE) = 0;
	//
	virtual	double	GetSelectPointGap(const CPointD&) = 0;
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const = 0;
	virtual	void	SetDirectionFixed(const CPointD&) = 0;
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const = 0;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const = 0;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const = 0;
	virtual	boost::optional<CPointD>	CalcExpandPoint(const CDXFdata*) const = 0;

	virtual	void	Serialize(CArchive&);
	DECLARE_DYNAMIC(CDXFdata)
};

typedef CSortArray<CObArray, CDXFdata*>			CDXFsort;
typedef CTypedPtrList<CObList, CDXFdata*>		CDXFlist;
typedef	CTypedPtrArrayEx<CObArray, CDXFdata*>	CDXFarray;

/////////////////////////////////////////////////////////////////////////////
// Point �N���X
class CDXFpoint : public CDXFdata  
{
protected:
	CPoint		m_ptDraw;	// �`�撲���p(��CDXFarc, CDXFellipse)
	CRect		m_rcDraw;	// ��`�`��(��CDXFcircle)

	// CDXFtext ������Q��
	void	SetMaxRect(void);

protected:
	CDXFpoint();
	CDXFpoint(ENDXFTYPE, CLayerData*, int);
public:
	CDXFpoint(LPDXFPARGV);
	// BLOCK�ް�����̺�߰�p
	CDXFpoint(CLayerData*, const CDXFpoint*, LPDXFBLOCK);

	const	CPoint	GetDrawPoint(void) const;

	virtual	BOOL	IsMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	double	GetLength(void) const;

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	// ����ɂ����H
	static	PFNORGDRILLTUNING	ms_pfnOrgDrillTuning;
	static	double	OrgTuning_Seq(const CDXFpoint*);
	static	double	OrgTuning_XY(const CDXFpoint*);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	void	SetDirectionFixed(const CPointD&);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;
	virtual	boost::optional<CPointD>	CalcExpandPoint(const CDXFdata*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFpoint)
};

/////////////////////////////////////////////////////////////////////////////
// Line �N���X
//		���ʂ�Circle/Arc�׽�p��CDXFpoint����̔h���Ƃ���
//		CDXFarc�׽�ł͎n�_, �I�_��񂪕K�v
class CDXFline : public CDXFpoint
{
	CPoint	m_ptDrawS, m_ptDrawE;	// �`��p�n�_�C�I�_
	void	SetMaxRect(void);

protected:
	double	GetSelectPointGap_Line(const CRectD&, const CPointD&, const CPointD&, const CPointD&) const;

protected:
	CDXFline();
	CDXFline(ENDXFTYPE, CLayerData*, int);
public:
	CDXFline(LPDXFLARGV);
	// BLOCK�ް�����̺�߰�p
	CDXFline(CLayerData*, const CDXFline*, LPDXFBLOCK);

	virtual	BOOL	IsMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	double	GetLength(void) const;

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	void	SetDirectionFixed(const CPointD&);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;
	virtual	boost::optional<CPointD>	CalcExpandPoint(const CDXFdata*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFline)
};

/////////////////////////////////////////////////////////////////////////////
// Circle �N���X
//		Line�׽�̔h���Ƃ���K�v�͂Ȃ����C
//		���ʂ�Arc�׽�Ŏn�_�I�_��Line�׽�ƌ����Ă邽�߁D
//
//		���W�ް��̊i�[��
//			m_pt[]		-> 0, 90, 180, 270
//			m_ptTun[]	-> 0, 180, 90, 270
//		���̕����s�����悢
//
//		m_bRound -> �{����Arc�p�D�����w���ɑΉ����邽��Circle�׽�����ނɂ���
//
class CDXFcircle : public CDXFline
{
	BOOL	m_bRoundFixed;	// �����w��(��]�����̌Œ�)
	void	SetCirclePoint(void);	// m_pt �ւ̑��

protected:
	int		m_nArrayExt;	// m_ptTun, m_ptMake �̔z����w��(0-1:�}X�� or 2-3:�}Y��)
	double	m_r, m_rMake;	// ���a, ������R�ʂ܂ł̔��a -> NC�����Ŏg�p
	CPointD	m_ct, m_ctTun;	// ���S���W
	BOOL	m_bRound;		// G2(FALSE)/G3(TRUE) DXF�̊�{�� G3(�����v���)

	virtual	void	SwapPt(int);
	virtual	void	XRev(void);
	virtual	void	YRev(void);

	// CDXFcircleEx ������Q��
	void	SetMaxRect(void);
	// CDXFellipse ������Q��
	void	GetQuarterPoint(const CPointD&, CPointD[]) const;

	// �~�C�~�ʁC���ʏ���
	void	SetEllipseArgv_Circle(const LPDXFBLOCK, LPDXFEARGV, double, double, BOOL);
	double	GetSelectPointGap_Circle(const CPointD&, double, double) const;

protected:
	CDXFcircle();
	CDXFcircle(ENDXFTYPE, CLayerData*, const CPointD&, double, BOOL, int);
public:
	CDXFcircle(LPDXFCARGV);
	// BLOCK�ް�����̺�߰�p
	CDXFcircle(CLayerData*, const CDXFcircle*, LPDXFBLOCK);

	// ��ۯ���߰���̎ړx�ŉ~���ȉ~�ɂȂ�
	void	SetEllipseArgv(const LPDXFBLOCK, LPDXFEARGV);

	BOOL	IsRoundFixed(void) const;
	double	GetR(void) const;
	double	GetMakeR(void) const;
	BOOL	GetRound(void) const;
	void	SetRoundFixed(const CPointD&, const CPointD&);
	int		GetG(void) const;
	int		GetBaseAxis(void) const;
	double	GetIJK(int nType) const;
	const CPointD	GetCenter(void) const;
	const CPointD	GetMakeCenter(void) const;

	virtual	BOOL	IsMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	double	GetLength(void) const;

	virtual	void	ReversePt(void);
	virtual	BOOL	IsRangeAngle(const CPointD&) const;

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	void	SetDirectionFixed(const CPointD&);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;
	virtual	boost::optional<CPointD>	CalcExpandPoint(const CDXFdata*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFcircle)
};

/////////////////////////////////////////////////////////////////////////////
// CircleEx �N���X
//		���_�w���C���H�J�n�ʒu�̎w���ް��Ɏg�p
class CDXFcircleEx : public CDXFcircle
{
	ENDXFTYPE2	m_enType2;

protected:
	CPointD		m_ctMake;			// ������R�ʂ܂ł̒��S���W -> NC�����Ŏg�p

	virtual	void	XRev(void);
	virtual	void	YRev(void);

protected:
	CDXFcircleEx();
public:
	CDXFcircleEx(ENDXFTYPE2, CLayerData*, const CPointD&, double);

	virtual	BOOL	IsMatchPoint(const CPointD&);
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
// Arc �N���X
//		�{���Ȃ�
//		class CDXFarc : public CDXFline, public CDXFcircle
//		�Ƃ��ׂ������C���d�p���ɂ��s��̂��߁C
//		CDXFcircle �� CDXFline �̔h���ɂ��ĉ������
class CDXFarc : public CDXFcircle
{
	void	SetRsign(void);
	void	SetMaxRect(void);

protected:
	BOOL		m_bRoundOrig;	// �������Ɍ������ς��\��������̂��ޯ�����
	double		m_sq, m_eq,		// �J�n�E�I���p�x(׼ޱݒP��)
				m_sqDraw, m_eqDraw,	m_rDraw;	// �`��p(Swap���Ȃ�)

	void	AngleTuning(void);
	void	SwapRound(void);
	void	SwapAngle(void);

	virtual	void	SwapPt(int);
	virtual	void	XRev(void);
	virtual	void	YRev(void);

protected:
	CDXFarc();
	CDXFarc(ENDXFTYPE, CLayerData*, const CPointD&, double, double, double, BOOL, int);
public:
	CDXFarc(LPDXFAARGV);
	// from CDXFpolyline::SetVertex()
	CDXFarc(LPDXFAARGV, BOOL, const CPointD&, const CPointD&);
	// BLOCK�ް�����̺�߰�p
	CDXFarc(CLayerData*, const CDXFarc*, LPDXFBLOCK);

	// ��ۯ���߰���̎ړx�ŉ~�ʂ��ȉ~�ʂɂȂ�
	void	SetEllipseArgv(const LPDXFBLOCK, LPDXFEARGV);

	BOOL	GetRoundOrig(void) const;
	double	GetStartAngle(void) const;
	double	GetEndAngle(void) const;

	virtual	void	SetNativePoint(size_t, const CPointD&);		// �p�x�̍X�V���܂�

	virtual	BOOL	IsMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;
	virtual	double	GetLength(void) const;

	virtual	void	ReversePt(void);
	virtual	BOOL	IsRangeAngle(const CPointD&) const;

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	void	SetDirectionFixed(const CPointD&);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;
	virtual	boost::optional<CPointD>	CalcExpandPoint(const CDXFdata*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFarc)
};

/////////////////////////////////////////////////////////////////////////////
// Ellipse �N���X
//		�ȉ~�̋�`�v�Z�́C�v�Z���ώG�Ȃ̂�
//		���������𔼌a�ƌ��Ȃ� CDXFarc �Ɠ���ɂ���
class CDXFellipse : public CDXFarc
{
	CPointD		m_ptLong;	// ����(���S����̑���)
	double		m_dShort;	// �Z��(�{��)
	double		m_dLongLength, m_dDrawLongLength;
	double		m_lq, m_lqMake;		// �����p�x(==�ȉ~�̌X��)
	double		m_lqMakeCos, m_lqMakeSin,	// �X���� cos(), sin()
				m_lqDrawCos, m_lqDrawSin;	// �g�p�p�x�������̂Ōv�Z���ʂ�ۑ�
	BOOL		m_bArc;		// TRUE:�ȉ~�ʁCFALSE:�ȉ~

	void	Construct(void);
	void	EllipseCalc(void);
	void	SetMaxRect(void);
	void	XYRev(const CPointD&, const CPointD&);
	void	SetEllipseTunPoint(void);

protected:
	virtual	void	SwapPt(int);
	virtual	void	XRev(void);
	virtual	void	YRev(void);

	CDXFellipse();
public:
	CDXFellipse(LPDXFEARGV);
	// BLOCK�ް�����̺�߰�p
	CDXFellipse(CLayerData*, const CDXFellipse*, LPDXFBLOCK);

	const CPointD	GetLongPoint(void) const;
	double	GetShortMagni(void) const;
	double	GetLongLength(void) const;
	double	GetShortLength(void) const;
	double	GetLean(void) const;
	double	GetLeanCos(void) const;
	double	GetLeanSin(void) const;
	BOOL	IsArc(void) const;
	void	SetRoundFixed(BOOL);

	virtual	BOOL	IsMatchPoint(const CPointD&);
	virtual BOOL	IsStartEqEnd(void) const;
	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	const CPointD	GetStartCutterPoint(void) const;
	virtual	const CPointD	GetStartMakePoint(void) const;
	virtual	const CPointD	GetEndCutterPoint(void) const;
	virtual	const CPointD	GetEndMakePoint(void) const;

	virtual	void	ReversePt(void);

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	void	SetDirectionFixed(const CPointD&);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;
	virtual	boost::optional<CPointD>	CalcExpandPoint(const CDXFdata*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFellipse)
};

/////////////////////////////////////////////////////////////////////////////
// PolyLine �N���X
//		�n�_�C�I�_�̊Ǘ��̂���
//		CDXFline ����̔h��
class CDXFpolyline : public CDXFline
{
	int			m_nObjCnt[3];	// ���ײݗv�f�Ɋ܂܂���[0]�Ɖ~��[1]�C�ȉ~[2]�̐�
	int			m_nPolyFlag;	// ���ײݕ`���׸�
	CDXFlist	m_ltVertex;		// �e���_(CDXFpoint or CDXFarc or CDXFellipse �i�[)
	BOOL		m_bSeq, m_bSeqBak;	// TRUE:Head, FALSE:Tail
	POSITION	m_posSel;		// GetSelectPointGap() �ň�ԋ߂�������޼ު���߼޼��

protected:
	virtual	void	SwapPt(int);
	virtual	void	XRev(void);
	virtual	void	YRev(void);

public:
	CDXFpolyline();		// ���������ڲԏ�񖳂�
	// BLOCK�ް�����̺�߰�p
	CDXFpolyline(CLayerData*, const CDXFpolyline*, LPDXFBLOCK);
	virtual ~CDXFpolyline();

	void	SetPolyFlag(int);
	int		GetPolyFlag(void) const;
	BOOL	GetSequence(void) const;
	INT_PTR	GetVertexCount(void) const;
	int		GetObjectCount(int) const;
	POSITION	GetFirstVertex(void) const;
	CDXFdata*	GetNextVertex(POSITION&) const;
	const	CPointD		GetFirstPoint(void) const;

	BOOL	SetVertex(LPDXFPARGV);
	BOOL	SetVertex(LPDXFPARGV, double, const CPointD&);
	void	EndSeq(void);

	virtual BOOL	IsStartEqEnd(void) const;
	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	double	GetSelectPointGap(const CPointD&);
	virtual	BOOL	GetDirectionArraw(const CPointD&, CPointD[][3]) const;
	virtual	void	SetDirectionFixed(const CPointD&);
	virtual	int		GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL = TRUE) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const;
	virtual	int		CheckIntersectionCircle(const CPointD&, double) const;
	virtual	boost::optional<CPointD>	CalcExpandPoint(const CDXFdata*) const;

	// Polyline�̓��ꏈ��(from DXFDoc.cpp2)
	void	SetParentLayer(CLayerData*);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFpolyline)
};

/////////////////////////////////////////////////////////////////////////////
// �R�����g���� �N���X
class CDXFtext : public CDXFpoint
{
protected:
	CString	m_strValue;

	CDXFtext();
public:
	CDXFtext(LPDXFTARGV lpText);
	// BLOCK�ް�����̺�߰�p
	CDXFtext(CLayerData*, const CDXFtext*, LPDXFBLOCK);

	CString		GetStrValue(void) const;

	virtual	double	GetEdgeGap(const CPointD&, BOOL = TRUE);
	virtual	void	Draw(CDC*) const;
	virtual	double	OrgTuning(BOOL = TRUE);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFtext)
};

#include "DXFdata.inl"
