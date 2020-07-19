// NCdata.h: CNCdata �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "MainFrm.h"
#include "DXFOption.h"		// enMAKETYPE
#include "NCVCdefine.h"

#ifdef _DEBUG
//#define	_DEBUG_DUMP
#endif

// �������� double -> float �֕ύX�������Ƃɔ���
// NCVCdefine.h �Œ�`���ꂽ�\���̂��Ē�`
struct	_NCDATA
{
	UINT		nErrorCode;			// NC�ް��̴װ����
	int			nLine;				// �s�ԍ�
	int			nGtype;				// GSMOF
	int			nGcode;				// ���ނɑ����l
	ENPLANE		enPlane;			// ����
	DWORD		dwValFlags;			// dValue �Ŏw�肳��Ă���l
	float		dValue[VALUESIZE];	// X,Y,Z,U,V,W,I,J,K,R,P,L,D,H
	float		dLength;			// �ړ��E�؍풷
};
struct	_G68ROUND
{
	ENPLANE		enPlane;		// �������̕���
	float		dRound;			// ��]�p�x(rad)
	float		dOrg[NCXYZ];	// ��]���S���W
	_G68ROUND(const _G68ROUND*);
	_G68ROUND(const G68ROUND&);
};
struct	_TAPER
{
	int		nTaper;		// 1:G51, -1:G52
	float	dTaper;		// ð�ߊp�x(rad)
	int		nDiff;		// �㉺�Ɨ���Ű 0:G60, 1:G61, 2:G62, 3:G63
	BOOL	bTonly;		// T���ޒP�Ǝw��
	_TAPER(const _TAPER*);
	_TAPER(const TAPER&);
};
typedef	_G68ROUND*	_LPG68ROUND;
typedef	_TAPER*		_LPTAPER;

// �n������
enum
{
	NCMIL_SQUARE = 0,
	NCMIL_BALL,
	NCMIL_CHAMFER,		// �ʎ��~��(��[�p90��)
	NCMIL_DRILL,		// �h����(��[�p118��)
		NCMIL_MAXTYPE		// [4]
};

// NC�ް�����׸�
#define	NCFLG_ENDMILL		0x00000007	// �n�������ޯ�
#define	NCFLG_G02G03		0x00000010	// 0:G02, 1:G03
#define	NCFLG_G98			0x00000020	// G98,G99�i����Ӱ�ނŎg�p�j
#define	NCFLG_LATHE_INPASS	0x00010000	// ������i����Ӱ�ނŎg�p �a�␳�Ɠ�������ޯĎg�p�j
#define	NCFLG_LATHE_HOLE	0x00020000	// �������i����Ӱ�ނŎg�p �a�␳�Ɠ�������ޯĎg�p�j
#define	NCFLG_LATHE_INSIDE	(NCFLG_LATHE_INPASS|NCFLG_LATHE_HOLE)	// ����

// �n�_�I�_�w��
enum	ENPOINTORDER
	{STARTPOINT=0, ENDPOINT=1};
// �`���ޭ��w��
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

// �����ْ�ʕ`��p
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

// ���՗p�萔
const float		LATHELINEWIDTH = 3.0f;	// ���߽�l���E�����߂̐���
const double	LATHEHEIGHT = 6.0;		// glOrtho()��bottom��top

// ܲԕ��d���H�@�`��p
struct WIRELINE
{
	COLORREF	col;
	GLushort	pattern;
	CVelement	vel;
};
struct WIREDRAW
{
	CVfloat		vpt,	// ���_���W
				vnr;	// �@���޸��
	std::vector<CVelement>	vvef;	// �ʐ����p�̒��_���ޯ��
	std::vector<WIRELINE>	vwl;	// ���`��p�̏��
	std::vector<float>		vlen;	// ø����Ŏg�p
	void	clear(void);
};

/////////////////////////////////////////////////////////////////////////////
// NC�ް���ǂݍ��ގ��ɂ����K�v���ް�
// �ǂݍ��ݏI����ɏ������邱�ƂŁA��؎g�p�������グ��
struct CNCread
{
	CPoint3F	m_ptValOrg,		// ��]�E���ٓ������̏����ȉ��H�I�_
				m_ptOffset;		// ܰ����W�n�̵̾��
	_LPG68ROUND	m_pG68;			// G68���W��]���
	_LPTAPER	m_pTaper;		// ð�߉��H���
	virtual ~CNCread();
};

/////////////////////////////////////////////////////////////////////////////
// NC�ް��̊�b�ް��C�`���޼ު�ĈȊO���ް��׽
// �`���޼ު�ĈȊO�ł��������邱�ƂŁC�ڰ��̏d�v�߲�ĂŒ�~������
class CNCdata;
typedef	CTypedPtrArrayEx<CPtrArray, CNCdata*>	CNCarray;
class CNCdata
{
	ENNCDTYPE	m_enType;		// �ް����� : NCVCdefine.h
	void		Constracter(LPNCARGV);

protected:
	DWORD		m_dwFlags;		// NC�ް�����׸�
	_NCDATA		m_nc;			// NC��b�ް� : NCVCdefine.h
	int			m_nSpindle;		// �厲��]���i��ɐ���Ӱ�ނŎg�p�j
	float		m_dFeed,		// ���̵�޼ު�Ă̐؍푗�葬�x
				m_dMove[NCXYZ],	// �e�����Ƃ̈ړ�����(������̎��Ԍv�Z�p)
				m_dEndmill;		// �����ٔ��a
	CNCarray	m_obCdata;		// �␳�p��޼ު��
	CNCdata*	m_pWireObj;		// ܲԉ��H�pUV����޼ު��
	//	�Œ軲�قł́C�w�肳�ꂽ���W���ŏI���W�ł͂Ȃ��̂�
	//	m_nc.dValue[] �ł͂Ȃ��ŏI���W�̕ێ����K�v
	// ���A���W��](G68)�ł�ؼ��ٍ��W�ƌv�Z���W��ʂɕۊ�
	CPoint3F	m_ptValS, m_ptValE;	// �J�n,�I�����W
	CPointF		m_pt2D;			// �Q�����ϊ���̍��W�v�Z����(�I�_)
	CNCread*	m_pRead;		// �ǂݍ��ݏI����ɏ��������ް��Q

	// G68���W��]
	void	CalcG68Round( LPG68ROUND, CPoint3F&) const;
	void	CalcG68Round(_LPG68ROUND, CPoint3F&) const;

	// �h���׽�p�ݽ�׸�
	CNCdata(ENNCDTYPE, const CNCdata*, LPNCARGV, const CPoint3F&);
	CNCdata(const CNCdata*);

public:
	CNCdata(LPNCARGV);				// ����̏������ݽ�׸�
	CNCdata(const CNCdata*, LPNCARGV, const CPoint3F&);	// �{�ް�
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
	// CPoint3F���畽�ʂ�2D���W�𒊏o
	CPointF	GetPlaneValue(const CPoint3F&) const;
	// CPointF����CPoint3F�֍��W�ݒ�
	void	SetPlaneValue(const CPointF&, CPoint3F&) const;
	// CPoint3F���畽�ʂ�2D���W�𒊏o���Č��_�␳
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
				GetCorrectArray(void);			// DXF�o�͗p
	void		SetWireObj(CNCdata*);			// from TH_UVWire.cpp
	CNCdata*	GetWireObj(void) const;
	const CNCread*	GetReadData(void) const;
	void		DeleteReadData(void);
	// NCViewGL.cpp ������Ăяo��
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

	// ��޼ު�Đ�L��`(�s�x���)
	virtual	CRect3F	GetMaxRect(void) const;
	// �؍��L��`
	virtual	CRect3F	GetMaxCutRect(void) const;
	// �ړ����C�؍풷�̌v�Z
	virtual	float	SetCalcLength(void);
	// �ۂ�(��ŰR)�̍��W�v�Z
	virtual	boost::tuple<BOOL, CPointF, float, float>	CalcRoundPoint(const CNCdata*, float) const;
	// �ʎ��ʒu�̍��W�ݒ�
	virtual	boost::optional<CPointF>	SetChamferingPoint(BOOL, float);
	// �Q�����Ȃ��p�x�����߂�
	virtual	float	CalcBetweenAngle(const CNCdata*) const;
	// �̾�Ēl�̕����v�Z
	virtual	int		CalcOffsetSign(void) const;
	// [�n�_|�I�_]�𐂒��ɵ̾��(90����])�������W�v�Z
	virtual	boost::optional<CPointF>	CalcPerpendicularPoint(ENPOINTORDER, float, int) const;
	// �̾�ĕ��ړ���������_
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint(const CNCdata*, float, float, BOOL) const;
	// �̾�ĕ��ړ���������_(�~�ʂ͐ڐ�)
	virtual	boost::optional<CPointF>	CalcOffsetIntersectionPoint2(const CNCdata*, float, BOOL) const;
	// �␳���W�̐ݒ�
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointF&, float);

#ifdef _DEBUG_DUMP
	void	DbgDump(void);
#endif
};

typedef void (CNCdata::*PFNNCDRAWPROC)(CDC*, BOOL) const;
typedef	void (CNCdata::*PFNSETENDMILLFUNC)(const CPoint3F&, CVfloat&) const;

/////////////////////////////////////////////////////////////////////////////
// G0,G1 ������Ը׽
class CNCline : public CNCdata
{
	EN_NCPEN	GetPenType(void) const;
	int			GetLineType(void) const;
	void	DrawLine(ENNCDRAWVIEW, CDC*, BOOL) const;

protected:
	CPointF		m_pt2Ds;				// 2�����ϊ���̎n�_(XYZ���ʗp)
	CPoint		m_ptDrawS[NCDRAWVIEW_NUM],	// �g��W�����݂̕`��n�_�I�_
				m_ptDrawE[NCDRAWVIEW_NUM];

	CNCline(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset) :	// �h���׽�p
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
// G8x �Œ軲�ٸ׽
struct PTCYCLE
{
	CPointF		ptI, ptR, ptC;	// �Ƽ�ٓ_�AR�_�A�؂荞�ݓ_
	CPoint		ptDrawI, ptDrawR, ptDrawC;
	CRect		rcDraw;			// ���ꕽ�ʂ̊ۈ�`��p
	void	DrawTuning(float f);
};
struct PTCYCLE3D
{
	CPoint3F	ptI, ptR, ptC;
};

class CNCcycle : public CNCline
{
	int			m_nDrawCnt;			// ��޼ު�č��W�l������==�`��p�J��Ԃ���
	PTCYCLE*	m_Cycle[NCDRAWVIEW_NUM];
	PTCYCLE3D*	m_Cycle3D;			// ���ۂ�3D���W(��OpenGL)

	CPoint3F	m_ptValI,			// �O��ʒu�ƂP�_�ڂ̌����H���W
				m_ptValR;
	CPoint		m_ptDrawI[NCDRAWVIEW_NUM],	// �g��W��
				m_ptDrawR[NCDRAWVIEW_NUM];

	float		m_dInitial;			// �Ƽ�ٓ_�̋L��
	float		m_dCycleMove,		// �ړ�����(�؍틗����m_nc.dLength)
				m_dDwell;			// �޳�َ���

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
// G2,G3 �~�ʕ�Ը׽
class	CNCcircle;
typedef void (CNCcircle::*PFNCIRCLEDRAW)(ENNCDRAWVIEW, CDC*) const;

struct DRAWGLWIRECIRCLE	// DrawGLWirePassCircle()����
{
	BOOL		bG03,	// TRUE:G03, FALSE:G02
				bLatheDepth;	// ���Փ��a�`��̓��ꐫ
	float		sq, eq;
	CPoint3F	pts, pte;
};
typedef	DRAWGLWIRECIRCLE*		LPDRAWGLWIRECIRCLE;

class CNCcircle : public CNCline  
{
	void		Constracter(void);

	CPoint3F	m_ptOrg;		// ���S���W
	float		m_r,			// ���a
				m_sq, m_eq,		// �J�n�E�I���p�x(׼ޱݒP��)
				m_dHelicalStep;	// �ضٓ���̈ړ���
	// �`��p
	float		m_dFactor,		// ���݂̊g�嗦(�v�Z���Ȃ���`��)
				m_dFactorXY,
				m_dFactorXZ,
				m_dFactorYZ;
	PFNCIRCLEDRAW	m_pfnCircleDraw;	// ���ʕʂ̕`��֐�
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

	// IJK�w��Ȃ��̎��C�~�̕��������璆�S�̎Z�o
	BOOL	CalcCenter(const CPointF&, const CPointF&);
	void	SetCenter(const CPointF&);
	// ���ʍ��W����̊p�x�v�Z�ƒ���
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
// NC�ް�����ۯ��ް��׽

// ؿ�����̴װ����
#define	NCERROR_RES_MIN	25000
#define	NCERROR_RES_MAX	25999

class CNCblock
{
	CString		m_strLine,	// �s�ԍ�
				m_strGcode;	// G���ޕ�����
	DWORD		m_dwFlags;	// ��ۯ����ݸ�����׸�
	UINT		m_nError;	// �װ����
	CNCdata*	m_pData;	// ��ۯ��ɑΉ������Ō��CNCdata��޼ު��
	size_t		m_nArray;	// CNCDoc::m_obGdata ���̔ԍ�

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
