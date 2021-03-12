// DXFdata.h: CDXFdata �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
class	CLayerData;
class	CDXFshape;

// ���̊p�x�ƒ���
#define	ARRAWANGLE		RAD(15.0f)
#define	ARRAWLENGTH		100

// DXF����׸�
#define	DXFFLG_MAKE				0x0001
#define	DXFFLG_SEARCH			0x0002
#define	DXFFLG_EDGE				0x0004
#define	DXFFLG_CLRWORK			0x000f
#define	DXFFLG_POLYCHILD		0x0010
#define	DXFFLG_SELECT			0x0080
#define	DXFFLG_DIRECTIONFIX		0x0100
#define	DXFFLG_OFFSET_EXCLUDE	0x1000
// DXFPOLYLINE�׸�
#define	DXFPOLY_SEQ				0x0001	// �i�s���� TRUE:Head->Tail, FALSE:Tail->Head
#define	DXFPOLY_SEQBAK			0x0002	// �����ޯ�����
#define	DXFPOLY_CLOSED			0x0100
#define	DXFPOLY_INTERSEC		0x0200
#define	DXFPOLY_ELLIPSE			0x1000

// ��޼ު�Đ������̈���
// �e pLayer �́CDataOperation()�� strLayer ���� CLayerData��޼ު�Ă�o�^����
struct	DXFBASE
{
	CLayerData*	pLayer;
};
struct	DXFPARGV : public DXFBASE
{
	CPointF		c;			// �������ʒuor���S
};
typedef	DXFPARGV*		LPDXFPARGV;
typedef	const DXFPARGV*	LPCDXFPARGV;	

struct	DXFLARGV : public DXFBASE
{
	CPointF		s, e;		// �J�n�E�I���_
};
typedef	DXFLARGV*		LPDXFLARGV;
typedef	const DXFLARGV*	LPCDXFLARGV;

struct	DXFCARGV : public DXFPARGV
{
	float		r;			// ���a
};
typedef	DXFCARGV*		LPDXFCARGV;
typedef	const DXFCARGV*	LPCDXFCARGV;

struct	DXFAARGV : public DXFCARGV
{
	float		sq, eq;		// �n�_�E�I�_�p�x
};
typedef	DXFAARGV*		LPDXFAARGV;
typedef	const DXFAARGV*	LPCDXFAARGV;

struct	DXFEARGV : public DXFPARGV
{
	CPointF		l;			// ����(���S����̑���)
	float		s;			// �Z��(�{��)
	float		sq, eq;		// �n�_�E�I�_�p�x
	BOOL		bRound;		// Default==TRUE(�����v���)
};
typedef	DXFEARGV*		LPDXFEARGV;
typedef	const DXFEARGV*	LPCDXFEARGV;

struct	DXFTARGV : public DXFPARGV
{
	CString		strValue;	// ������
};
typedef	DXFTARGV*		LPDXFTARGV;
typedef	const DXFTARGV*	LPCDXFTARGV;

// ��ۯ��̕t�����
#define	DXFBLFLG_X		0x0001
#define	DXFBLFLG_Y		0x0002
#define	DXFBLFLG_Z		0x0004
#define	DXFBLFLG_R		0x0008
struct	DXFBLOCK
{
	DWORD		dwBlockFlg;
	CPointF		ptOrg;			// �}���̾��
	float		dMagni[NCXYZ];	// �e���̔{��
	float		dRound;			// ��]�p�x(�x)
};
typedef	DXFBLOCK*		LPDXFBLOCK;
typedef	const DXFBLOCK*	LPCDXFBLOCK;

// CDXFpoint�p���I�֐��Ăяo��
class	CDXFpoint;
typedef float (*PFNORGDRILLTUNING)(const CDXFpoint*);

// CDXFcircleEx�p��߼������
enum	ENDXFTYPE2
	{DXFORGDATA = 0, DXFSTADATA = 1};

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
	DWORD		m_dwFlags,		// DXF����׸�
				m_nSerialSeq;

protected:
	int			m_nPoint;		// m_ptTun, m_ptMake �m�ې�
	CPointF*	m_pt;			// �e��޼ު�ČŗL���W
	CPointF*	m_ptTun;		// ���_������̍��W
	CPointF*	m_ptMake;		// ������R�ʂ܂ł̔�r�p(NC�����ް�)
	CRect3F		m_rcMax;		// ��޼ު�ċ�`
	// CDXFpolyline ������ CDXFpolyline ����X�V����
	CLayerData*	m_pParentLayer;	// �ް��̑�����ڲԏ��
	CDXFshape*	m_pParentMap;	// �ް��̑�����ϯ�ߏ��
	//
	virtual	void	XRev(void);		// X���̕������]
	virtual	void	YRev(void);		// Y���̕������]
	virtual	void	SetMaxRect(void) = 0;

	void	OrgTuningBase(void);

	CDXFdata(ENDXFTYPE, CLayerData*, int, DWORD);

public:
	virtual	~CDXFdata();
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	static	BOOL		ms_fXRev;		// X�����]���ǂ���
	static	BOOL		ms_fYRev;		// Y�����]���ǂ���
	static	CPointF		ms_ptOrg;		// �؍팴�_
	static	CDXFdata*	ms_pData;		// �P�O�̐����ް�
	static	DWORD		ms_nSerialSeq;	// �رײ�޶���

	ENDXFTYPE	GetType(void) const;
	ENDXFTYPE	GetMakeType(void) const;
	DWORD		GetSerializeSeq(void) const;
	void		ChangeMakeType(ENDXFTYPE);	// �~�ް��ɂ�錊���H�Ȃ�
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
	virtual	void	SwapMakePt(int);	// m_ptTun, m_ptMake �̎n�_�I�_����ւ�
	virtual	void	SwapNativePt(void);	// �ŗL���W�l�̓���ւ�
	virtual	void	RoundObjPoint(const CPointF&, float);
	//
	virtual	BOOL	IsMakeTarget(void) const = 0;
	virtual	BOOL	IsMakeMatchPoint(const CPointF&) = 0;
	virtual BOOL	IsStartEqEnd(void) const = 0;	// �n�_�I�_��������޼ު�ĂȂ�TRUE
	virtual	BOOL	IsDirectionPoint(const CPointF&, const CPointF&);
			float	GetEdgeGap(const CDXFdata*, BOOL = TRUE);
	virtual	float	GetEdgeGap(const CPointF&,  BOOL = TRUE) = 0;
	virtual	const CPointF	GetStartCutterPoint(void) const = 0;// ���H�J�n�ʒu
	virtual	const CPointF	GetStartMakePoint(void) const = 0;
	virtual	const CPointF	GetEndCutterPoint(void) const = 0;	// ���H�I���ʒu(���H�I�_)
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
// Point �N���X
class CDXFpoint : public CDXFdata  
{
protected:
	CPoint	m_ptDraw;	// �`�撲���p(��CDXFarc, CDXFellipse)
	CRect	m_rcDraw;	// ��`�`��(��CDXFcircle)
	// CDXFtext ������Q��
	virtual	void	SetMaxRect(void);
	//
	CDXFpoint();
	CDXFpoint(ENDXFTYPE, CLayerData*, int, DWORD);
public:
	CDXFpoint(LPCDXFPARGV, DWORD = 0);
	// BLOCK�ް�����̺�߰�p
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

	// ����ɂ����H
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
// Line �N���X
//		���ʂ�Circle/Arc�׽�p��CDXFpoint����̔h���Ƃ���
//		CDXFarc�׽�ł͎n�_, �I�_��񂪕K�v
class CDXFline : public CDXFpoint
{
protected:
	CPoint	m_ptDrawS, m_ptDrawE;	// �`��p�n�_�C�I�_
	virtual	void	SetMaxRect(void);
	//
	CDXFline();
	CDXFline(ENDXFTYPE, CLayerData*, int, DWORD);
public:
	CDXFline(LPCDXFLARGV, DWORD = 0);
	// BLOCK�ް�����̺�߰�p
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

protected:
	int		m_nArrayExt;	// m_ptTun, m_ptMake �̔z����w��(0,1:�}X�� or 2,3:�}Y��)
	float	m_r, m_rMake;	// ���a, ������R�ʂ܂ł̔��a -> NC�����Ŏg�p
	CPointF	m_ct, m_ctTun;	// ���S���W
	BOOL	m_bRound;		// G2(FALSE)/G3(TRUE) DXF�̊�{�� G3(�����v���)

	virtual	void	SetCirclePoint(void);	// m_pt �ւ̑��
	virtual	void	XRev(void);
	virtual	void	YRev(void);

	// CDXFcircleEx ������Q��
	virtual	void	SetMaxRect(void);
	// CDXFellipse ������Q��
	void	GetQuarterPoint(const CPointF&, CPointF[]) const;

	// �~�C�~�ʁC���ʏ���
	void	SetEllipseArgv_Circle(LPCDXFBLOCK, LPDXFEARGV, float, float, BOOL);
	float	GetSelectPointGap_Circle(const CPointF&, float, float) const;
	BOOL	GetDirectionArraw_Circle(const float[], const CPointF[], CPointF[][3]);
	size_t	SetVectorPointSub(BOOL, float, float, float, const CPointF&, CVPointF&) const;

	CDXFcircle();
	CDXFcircle(ENDXFTYPE, CLayerData*, const CPointF&, float, BOOL, int, DWORD);
public:
	CDXFcircle(LPCDXFCARGV, DWORD = 0);
	// BLOCK�ް�����̺�߰�p
	CDXFcircle(CLayerData*, const CDXFcircle*, LPCDXFBLOCK, DWORD = 0);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	// ��ۯ���߰���̎ړx�ŉ~���ȉ~�ɂȂ�
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
// CircleEx �N���X
//		���_�w���C���H�J�n�ʒu�̎w���ް��Ɏg�p
class CDXFcircleEx : public CDXFcircle
{
	ENDXFTYPE2	m_enType2;

protected:
	CPointF		m_ctMake;			// ������R�ʂ܂ł̒��S���W -> NC�����Ŏg�p

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
// Arc �N���X
//		�{���Ȃ�
//		class CDXFarc : public CDXFline, public CDXFcircle
//		�Ƃ��ׂ������C���d�p���ɂ��s��̂��߁C
//		CDXFcircle �� CDXFline �̔h���ɂ��ĉ������
class CDXFarc : public CDXFcircle
{
	void	SetRsign(void);

protected:
	BOOL	m_bRoundOrig;	// �������Ɍ������ς��\��������̂��ޯ�����
	float	m_sq, m_eq,		// �J�n�E�I���p�x(׼ޱݒP��)
			m_sqDraw, m_eqDraw,	m_rDraw;	// �`��p(Swap���Ȃ�)

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
	// BLOCK�ް�����̺�߰�p
	CDXFarc(CLayerData*, const CDXFarc*, LPCDXFBLOCK, DWORD = 0);
#ifdef _DEBUG
	virtual	void	DbgDump(void);
#endif

	// ��ۯ���߰���̎ړx�ŉ~�ʂ��ȉ~�ʂɂȂ�
	void	SetEllipseArgv(LPCDXFBLOCK, LPDXFEARGV);

	BOOL	GetRoundOrig(void) const;
	float	GetStartAngle(void) const;
	float	GetEndAngle(void) const;

	virtual	void	SetNativePoint(size_t, const CPointF&);		// �p�x�̍X�V���܂�

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
// Ellipse �N���X
//		�ȉ~�̋�`�v�Z�́C�v�Z���ώG�Ȃ̂�
//		���������𔼌a�ƌ��Ȃ� CDXFarc �Ɠ���ɂ���
class CDXFellipse : public CDXFarc
{
	CPointF	m_ptLong;	// ����(���S����̑���)
	float	m_dShort,	// �Z��(�{��)
			m_dLongLength, m_dDrawLongLength,
			m_lq, m_lqMake,		// �����p�x(==�ȉ~�̌X��)
			m_lqMakeCos, m_lqMakeSin,	// �X���� cos(), sin() 
			m_lqDrawCos, m_lqDrawSin;	// �`��␶���Ŏg�p�p�x�������̂Ōv�Z���ʂ�ۑ�
	BOOL	m_bArc;		// TRUE:�ȉ~�ʁCFALSE:�ȉ~

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
	// BLOCK�ް�����̺�߰�p
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
// PolyLine �N���X
//		�n�_�C�I�_�̊Ǘ��̂���
//		CDXFline ����̔h��
class CDXFpolyline : public CDXFline
{
	int			m_nObjCnt[3];	// ���ײݗv�f�Ɋ܂܂���[0]�Ɖ~��[1]�C�ȉ~[2]�̐�
	DWORD		m_dwPolyFlags;	// ���ײݕ`���׸�
	CDXFlist	m_ltVertex;		// �e���_(CDXFpoint or CDXFarc or CDXFellipse �i�[)
	POSITION	m_posSel;		// GetSelectPointGap() �ň�ԋ߂�������޼ު���߼޼��
	CDXFline*	m_pArrawLine;	// �����w������p

	void		CheckPolylineIntersection_SubLoop(const CPointF&, const CPointF&, POSITION);
	void		CheckPolylineIntersection_SubLoop(const CDXFarc*, POSITION);
	void		CheckPolylineIntersection_SubLoop(const CDXFellipse*, POSITION);

protected:
	virtual	void	XRev(void);
	virtual	void	YRev(void);
	virtual	void	SetMaxRect(void);

public:
	CDXFpolyline();		// ���������ڲԏ�񖳂�
	// BLOCK�ް�����̺�߰�p
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

	// Polyline�̓��ꏈ��(from ReadDXF.cpp)
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
	CDXFtext(LPCDXFTARGV lpText, DWORD = 0);
	// BLOCK�ް�����̺�߰�p
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
