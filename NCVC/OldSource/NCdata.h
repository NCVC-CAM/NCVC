// NCdata.h: CNCdata �N���X�̃C���^�[�t�F�C�X
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
// NC�ް��̊�b�ް��C�`���޼ު�ĈȊO���ް��׽
// �`���޼ު�ĈȊO�ł��������邱�ƂŁC�ڰ��̏d�v�߲�ĂŒ�~������
class CNCdata
{
	ENNCDTYPE	m_enType;	// �ް����� : NCVCdefine.h
	void		Constracter(LPNCARGV lpArgv);

protected:
	NCDATA		m_nc;			// NC��b�ް� -> NCVCaddin.h
	CPointD		m_pt2D;			// �Q�����ϊ���̍��W�v�Z����
	double		m_dFeed;		// ���̵�޼ު�Ă̐؍푗�葬�x
	double		m_dMove[NCXYZ];	// �e�����Ƃ̈ړ�����(������̎��Ԍv�Z�p)
	//	�Œ軲�قł́C�w�肳�ꂽ���W���ŏI���W�ł͂Ȃ��̂�
	//	m_nc.dValue[] �ł͂Ȃ��ŏI���W�̕ێ����K�v
	CPoint3D	m_ptValS, m_ptValE;		// �J�n,�I�����W
	CRect3D		m_rcMax;		// ��Ԑ�L��`

	// CPoint3D���畽�ʂ�2D���W�𒊏o
	CPointD	GetPlaneValue(const CPoint3D&);
	// CPoint3D���畽�ʂ�2D���W�𒊏o���Č��_�␳
	CPointD	GetPlaneValueOrg(const CPoint3D&, const CPoint3D&);

	CNCdata(ENNCDTYPE, const CNCdata*, LPNCARGV);	// �h���׽�p�ݽ�׸�
public:
	CNCdata(LPNCARGV);				// ����̏������ݽ�׸�
	CNCdata(const CNCdata*, LPNCARGV);	// �{�ް�
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

	// �������z�֐��ɂ���Ƶ�޼ު�Đ����ł��Ȃ��̂Œ�`�̂�
	virtual	void	DrawTuning(double) {}
	virtual	void	DrawTuningXY(double) {}
	virtual	void	DrawTuningXZ(double) {}
	virtual	void	DrawTuningYZ(double) {}
	virtual	void	Draw(CDC*) {}
	virtual	void	DrawXY(CDC*) {}
	virtual	void	DrawXZ(CDC*) {}
	virtual	void	DrawYZ(CDC*) {}

	// �ۂ�(��ŰR)�̍��W�v�Z
	virtual	BOOL	CalcRoundPoint(const CNCdata*, double, CPointD&, double&, double&);
	// �ʎ��ʒu�̍��W�ݒ�
	virtual	BOOL	SetChamferingPoint(BOOL, double, CPointD&);
	// �Q�����Ȃ��p�x�����߂�
	virtual	double	CalcBetweenAngle(const CPoint3D&, const CNCdata*);
	// �̾�Ēl�̕����v�Z
	virtual	int		CalcOffsetSign(const CPoint3D&);
	// [�n�_|�I�_]�𐂒��ɵ̾��(90����])�������W�v�Z
	virtual	CPointD	CalcPerpendicularPoint(const CPoint3D&, const CPoint3D&, double, int);
	// �̾�ĕ��ړ���������_
	virtual	CPointD	CalcOffsetIntersectionPoint(const CPoint3D&, const CNCdata*, double, int, int);
	// �̾�ĕ��ړ���������_(�~�ʂ͐ڐ�)
	virtual	CPointD	CalcOffsetIntersectionPoint2(const CPoint3D&, const CNCdata*, double, int, int);
	// �␳���W�̐ݒ�
	virtual	BOOL	SetCorrectPoint(BOOL, const CPointD&, double);

#ifdef _DEBUG
	void	DbgDump(void);
#endif
};

/////////////////////////////////////////////////////////////////////////////
// G0,G1 ������Ը׽
class CNCline : public CNCdata
{
	EN_NCPEN	GetPenType(void);
	// �ړ����C�؍풷�̌v�Z
	void	CalcLength(void);

protected:
	CPointD		m_pts, m_pte;			// 2�����ϊ���̎n�_�I�_(XYZ���ʗp)
	CPoint		m_ptDrawS, m_ptDrawE,	// �g��W�����݂̕`��n�_�I�_
				m_ptDrawS_XY, m_ptDrawE_XY,
				m_ptDrawS_XZ, m_ptDrawE_XZ,
				m_ptDrawS_YZ, m_ptDrawE_YZ;
	void	SetMaxRect(void);

	CNCline(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv) :	// �h���׽�p
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
// G8x �Œ軲�ٸ׽
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
	int			m_nDrawCnt;		// ��޼ު�č��W�l������==�`��p�J��Ԃ���
	PTCYCLE*	m_Cycle;
	PTCYCLE_XY*	m_CycleXY;
	PTCYCLE*	m_CycleXZ;
	PTCYCLE*	m_CycleYZ;

	CPoint3D	m_ptValI,		// �O��ʒu�ƂP�_�ڂ̌����H���W
				m_ptValR;
	CPoint		m_ptDrawI, m_ptDrawR,		// �g��W��
				m_ptDrawI_XZ, m_ptDrawR_XZ,
				m_ptDrawI_YZ, m_ptDrawR_YZ;

	double		m_dInitial,		// �Ƽ�ٓ_�̋L��
				m_dCycleMove,	// �ړ�����(�؍틗����m_nc.dLength)
				m_dDwell;		// �޳�َ���

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
// G2,G3 �~�ʕ�Ը׽
class	CNCcircle;
enum	EN_NCCIRCLEDRAW
	{NCCIRCLEDRAW_XYZ, NCCIRCLEDRAW_XY, NCCIRCLEDRAW_XZ, NCCIRCLEDRAW_YZ};
typedef void (CNCcircle::*PFNCIRCLEDRAW)(EN_NCCIRCLEDRAW, CDC*);

class CNCcircle : public CNCdata  
{
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
	void		Draw_G17(EN_NCCIRCLEDRAW, CDC*);
	void		Draw_G18(EN_NCCIRCLEDRAW, CDC*);
	void		Draw_G19(EN_NCCIRCLEDRAW, CDC*);

	// IJK�w��Ȃ��̎��C�~�̕��������璆�S�̎Z�o
	BOOL	CalcCenter(const CPointD&, const CPointD&);
	void	SetCenter(const CPointD&);
	// ���ʍ��W����̊p�x�v�Z�ƒ���
	void	AngleTuning(const CPointD&, const CPointD&);
	// �؍풷�̌v�Z
	void	CalcLength(void);

protected:
	// �O�ڂ���l�p�`
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
// NC�ް�����ۯ��ް��׽
class CNCblock
{
	CString		m_strLine,	// �s�ԍ�
				m_strGcode;	// G���ޕ�����
	DWORD		m_dwFlags;	// ��ۯ����ݸ�����׸�
	CNCdata*	m_pData;	// ��ۯ��ɑΉ������Ō��CNCdata��޼ު��
	int			m_nArray;	// CNCDoc::m_obGdata ���̔ԍ�

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
