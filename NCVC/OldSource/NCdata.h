// NCdata.h: CNCdata �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "MainFrm.h"
#include "NCVCdefine.h"

// �n�_�I�_�w��
enum	ENPOINTORDER
	{STARTPOINT, ENDPOINT};

/////////////////////////////////////////////////////////////////////////////
// NC�ް���ǂݍ��ގ��ɂ����K�v���ް�
// �ǂݍ��ݏI����ɏ������邱�ƂŁA��؎g�p�������グ��
class CNCread
{
public:
	CPoint3D	m_ptValOrg,		// ��]�E���ٓ������̏����ȉ��H�I�_
				m_ptOffset;		// ܰ����W�n�̵̾��
	G68ROUND	m_g68;			// G68���W��]���
};

/////////////////////////////////////////////////////////////////////////////
// NC�ް��̊�b�ް��C�`���޼ު�ĈȊO���ް��׽
// �`���޼ު�ĈȊO�ł��������邱�ƂŁC�ڰ��̏d�v�߲�ĂŒ�~������
class CNCdata
{
	ENNCDTYPE	m_enType;	// �ް����� : NCVCdefine.h
	void		Constracter(LPNCARGV);

protected:
	NCDATA		m_nc;			// NC��b�ް� -> NCVCdefine.h
	CPointD		m_pt2D;			// �Q�����ϊ���̍��W�v�Z����(�I�_)
	double		m_dFeed,		// ���̵�޼ު�Ă̐؍푗�葬�x
				m_dMove[NCXYZ],	// �e�����Ƃ̈ړ�����(������̎��Ԍv�Z�p)
				m_dEndmill;		// �����ٌa
	int			m_nEndmillType;	// ����������
	CTypedPtrArrayEx<CPtrArray, CNCdata*>
				m_obCdata;		// �␳�p��޼ު��
	//	�Œ軲�قł́C�w�肳�ꂽ���W���ŏI���W�ł͂Ȃ��̂�
	//	m_nc.dValue[] �ł͂Ȃ��ŏI���W�̕ێ����K�v
	// ���A���W��](G68)�ł�ؼ��ٍ��W�ƌv�Z���W��ʂɕۊ�
	CPoint3D	m_ptValS, m_ptValE;		// �J�n,�I�����W
	CRect3D		m_rcMax;		// ��Ԑ�L��`
	CNCread*	m_pRead;		// �ǂݍ��ݏI����ɏ��������ް��Q

	// CPoint3D���畽�ʂ�2D���W�𒊏o
	CPointD	GetPlaneValue(const CPoint3D&) const;
	// CPointD����CPoint3D�֍��W�ݒ�
	void	SetPlaneValue(const CPointD&, CPoint3D&);
	// CPoint3D���畽�ʂ�2D���W�𒊏o���Č��_�␳
	CPointD	GetPlaneValueOrg(const CPoint3D&, const CPoint3D&) const;
	// G68���W��]
	void	CalcG68Round(LPG68ROUND, CPoint3D&);

	// �h���׽�p�ݽ�׸�
	CNCdata(ENNCDTYPE, const CNCdata*, LPNCARGV, const CPoint3D&);
	CNCdata(const CNCdata*);

public:
	CNCdata(LPNCARGV);				// ����̏������ݽ�׸�
	CNCdata(const CNCdata*, LPNCARGV, const CPoint3D&);	// �{�ް�
	virtual	~CNCdata();

public:
	ENNCDTYPE	GetType(void) const;
	UINT	GetNCObjErrorCode(void) const;
	int		GetBlockLineNo(void) const;
	int		GetGtype(void) const;
	int		GetGcode(void) const;
	BOOL	IsCutter(void) const;
	ENPLANE	GetPlane(void) const;
	DWORD	GetValFlags(void) const;
	double	GetValue(size_t) const;
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
	double	GetFeed(void) const;
	double	GetMove(size_t) const;
	double	GetEndmill(void) const;
	int		GetEndmillType(void) const;
	const CRect3D	GetMaxRect(void) const;
	CNCdata*	NC_CopyObject(void);			// from TH_Correct.cpp
	void		AddCorrectObject(CNCdata*);
	CTypedPtrArrayEx<CPtrArray, CNCdata*>*
				GetCorrectArray(void);			// DXF�o�͗p
	const CNCread*	GetReadData(void) const;
	void		DeleteReadData(void);

	virtual	void	DrawTuning(const double);
	virtual	void	DrawTuningXY(const double);
	virtual	void	DrawTuningXZ(const double);
	virtual	void	DrawTuningYZ(const double);
	virtual	void	Draw(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawXY(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawXZ(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawYZ(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawGL(BOOL = FALSE) const;
	virtual	void	DrawSideFace(double) const;
	virtual	void	DrawBottomFace(boost::optional<double>&) const;

	// �ړ����C�؍풷�̌v�Z
	virtual	double	SetCalcLength(void);
	// �ۂ�(��ŰR)�̍��W�v�Z
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	// �ʎ��ʒu�̍��W�ݒ�
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	// �Q�����Ȃ��p�x�����߂�
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	// �̾�Ēl�̕����v�Z
	virtual	int		CalcOffsetSign(void) const;
	// [�n�_|�I�_]�𐂒��ɵ̾��(90����])�������W�v�Z
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	// �̾�ĕ��ړ���������_
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, BOOL) const;
	// �̾�ĕ��ړ���������_(�~�ʂ͐ڐ�)
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const;
	// �␳���W�̐ݒ�
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);

	// OpenGL �֘A
	GLuint		m_glList;		// �`��p�ި���ڲؽ�

#ifdef _DEBUG_DUMP
	void	DbgDump(void);
#endif
};

typedef	CTypedPtrArrayEx<CPtrArray, CNCdata*>	CNCarray;

/////////////////////////////////////////////////////////////////////////////
// G0,G1 ������Ը׽
class CNCline : public CNCdata
{
	EN_NCPEN	GetPenType(void) const;
	int			GetLineType(void) const;
	void	DrawLine(CDC*, size_t, BOOL, BOOL) const;
	void	SetEndmillPath(CPointD*, CPointD*, CPointD*) const;

protected:
	CPointD		m_pt2Ds;				// 2�����ϊ���̎n�_(XYZ���ʗp)
	CPoint		m_ptDrawS[1+NCXYZ],		// �g��W�����݂̕`��n�_�I�_
				m_ptDrawE[1+NCXYZ];
	void	SetMaxRect(void);

	CNCline(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset) :	// �h���׽�p
		CNCdata(enType, pData, lpArgv, ptOffset) {}
public:
	CNCline(const CNCdata*, LPNCARGV, const CPoint3D&);
	CNCline(const CNCdata*);

public:
	virtual	void	DrawTuning(const double);
	virtual	void	DrawTuningXY(const double);
	virtual	void	DrawTuningXZ(const double);
	virtual	void	DrawTuningYZ(const double);
	virtual	void	Draw(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawXY(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawXZ(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawYZ(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawGL(BOOL = FALSE) const;
	virtual	void	DrawSideFace(double) const;
	virtual	void	DrawBottomFace(boost::optional<double>&) const;

	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, BOOL) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// G8x �Œ軲�ٸ׽
struct PTCYCLE
{
	CPointD		ptI, ptR, ptC;	// �Ƽ�ٓ_�AR�_�A�؂荞�ݓ_
	CPoint		ptDrawI, ptDrawR, ptDrawC;
	CRect		rcDraw;			// ���ꕽ�ʂ̊ۈ�`��p
	void	DrawTuning(const double f);
};
struct PTCYCLE3D
{
	CPoint3D	ptI, ptR, ptC;
};

class CNCcycle : public CNCline
{
	int			m_nDrawCnt;			// ��޼ު�č��W�l������==�`��p�J��Ԃ���
	PTCYCLE*	m_Cycle[1+NCXYZ];	// XYZ��XY,XZ,YZ
	PTCYCLE3D*	m_Cycle3D;			// OpenGL�p

	CPoint3D	m_ptValI,		// �O��ʒu�ƂP�_�ڂ̌����H���W
				m_ptValR;
	CPoint		m_ptDrawI[1+NCXYZ],		// �g��W��
				m_ptDrawR[1+NCXYZ];

	double		m_dInitial,		// �Ƽ�ٓ_�̋L��
				m_dCycleMove,	// �ړ�����(�؍틗����m_nc.dLength)
				m_dDwell;		// �޳�َ���

	void	DrawCyclePlane(CDC*, size_t, BOOL) const;
	void	DrawCycle(CDC*, size_t, BOOL) const;
	void	SetEndmillPath(const CPointD&, CPointD*) const;

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

	virtual	void	DrawTuning(const double);
	virtual	void	DrawTuningXY(const double);
	virtual	void	DrawTuningXZ(const double);
	virtual	void	DrawTuningYZ(const double);
	virtual	void	Draw(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawXY(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawXZ(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawYZ(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawGL(BOOL = FALSE) const;
	virtual	void	DrawSideFace(double) const;
	virtual	void	DrawBottomFace(boost::optional<double>&) const;

	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, BOOL) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);
};

/////////////////////////////////////////////////////////////////////////////
// G2,G3 �~�ʕ�Ը׽
class	CNCcircle;
enum	EN_NCCIRCLEDRAW
	{NCCIRCLEDRAW_XYZ, NCCIRCLEDRAW_XY, NCCIRCLEDRAW_XZ, NCCIRCLEDRAW_YZ};
typedef void (CNCcircle::*PFNCIRCLEDRAW)(EN_NCCIRCLEDRAW, CDC*) const;

class CNCcircle : public CNCdata  
{
	void		Constracter(void);

	int			m_nG23;			// G02:0 G03:1
	CPoint3D	m_ptOrg;		// ���S���W
	double		m_r,			// ���a
				m_sq, m_eq,		// �J�n�E�I���p�x(׼ޱݒP��)
				m_dHelicalStep;	// �ضٓ���̈ړ���
	// �`��p
	double		m_dFactor,		// ���݂̊g�嗦(�v�Z���Ȃ���`��)
				m_dFactorXY,
				m_dFactorXZ,
				m_dFactorYZ;
	PFNCIRCLEDRAW	m_pfnCircleDraw;	// ���ʕʂ̕`��֐�
	void	Draw_G17(EN_NCCIRCLEDRAW, CDC*) const;
	void	Draw_G18(EN_NCCIRCLEDRAW, CDC*) const;
	void	Draw_G19(EN_NCCIRCLEDRAW, CDC*) const;
	void	SetEndmillPath(std::vector<CPointD>&, std::vector<CPointD>&, CPointD*) const;

	// IJK�w��Ȃ��̎��C�~�̕��������璆�S�̎Z�o
	BOOL	CalcCenter(const CPointD&, const CPointD&);
	void	SetCenter(const CPointD&);
	// ���ʍ��W����̊p�x�v�Z�ƒ���
	void	AngleTuning(const CPointD&, const CPointD&);

protected:
	// �O�ڂ���l�p�`
	void	SetMaxRect(void);

public:
	CNCcircle(const CNCdata*, LPNCARGV, const CPoint3D&);
	CNCcircle(const CNCdata*);

	int		GetG23(void) const;
	const	CPoint3D	GetOrg(void) const;
	double	GetR(void) const;
	double	GetStartAngle(void) const;
	double	GetEndAngle(void) const;

	virtual	void	DrawTuning(const double);
	virtual	void	DrawTuningXY(const double);
	virtual	void	DrawTuningXZ(const double);
	virtual	void	DrawTuningYZ(const double);
	virtual	void	Draw(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawXY(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawXZ(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawYZ(CDC*, BOOL, BOOL = FALSE) const;
	virtual	void	DrawGL(BOOL = FALSE) const;
	virtual	void	DrawSideFace(double) const;
	virtual	void	DrawBottomFace(boost::optional<double>&) const;

	virtual	double	SetCalcLength(void);
	virtual	boost::tuple<BOOL, CPointD, double, double>	CalcRoundPoint(const CNCdata*, double) const;
	virtual	boost::optional<CPointD>	SetChamferingPoint(BOOL, double);
	virtual	double	CalcBetweenAngle(const CNCdata*) const;
	virtual	int		CalcOffsetSign(void) const;
	virtual	boost::optional<CPointD>	CalcPerpendicularPoint(ENPOINTORDER, double, int) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint(const CNCdata*, double, BOOL) const;
	virtual	boost::optional<CPointD>	CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const;
	virtual	void	SetCorrectPoint(ENPOINTORDER, const CPointD&, double);
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
	int			m_nArray;	// CNCDoc::m_obGdata ���̔ԍ�

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
