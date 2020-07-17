// DXFMakeOption.h: DXF�o�͵�߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define	MKDX_NUM_LTYPE_O		0
#define	MKDX_NUM_LTYPE_C		1
#define	MKDX_NUM_LTYPE_M		2
#define	MKDX_NUM_LTYPE_H		3
#define	MKDX_NUM_LCOL_O			4
#define	MKDX_NUM_LCOL_C			5
#define	MKDX_NUM_LCOL_M			6
#define	MKDX_NUM_LCOL_H			7
#define	MKDX_NUM_PLANE			8
#define	MKDX_NUM_CYCLE			9

#define	MKDX_DBL_ORGLENGTH		0
#define	MKDX_DBL_CYCLER			1

#define	MKDX_FLG_ORGCIRCLE		0
#define	MKDX_FLG_ORGCROSS		1

#define	MKDX_STR_ORIGIN			0
#define	MKDX_STR_CAMLINE		1
#define	MKDX_STR_MOVE			2
#define	MKDX_STR_CORRECT		3

class CDXFMakeOption  
{
// ���Ұ��ݒ���޲�۸ނ͂��F�B
friend class CMakeDXFDlg1;
friend class CMakeDXFDlg2;

	// int�^��߼��
	union {
		struct {
			int		m_nLType[4],	// �eڲԂ̐���
					m_nLColor[4],	// �@�V �̐F
					m_nPlane,		// ���ʎw��
					m_nCycle;		// �Œ軲�ُo������
		};
		int			m_unNums[10];
	};
	// double�^��߼��
	union {
		struct {
			double	m_dOrgLength,	// ���_����(�a)
					m_dCycleR;		// �Œ軲�ى~�o�͂̌a
		};
		double		m_udNums[2];
	};
	// BOOL�^��߼��
	union {
		struct {
			BOOL	m_bOrgCircle,	// ���_�~�o��
					m_bOrgCross;	// ���_�\���o��
		};
		BOOL		m_ubFlags[2];
	};
	// CString�^��߼��
	CString		m_strOption[4];		// �e��ڲ�

public:
	CDXFMakeOption();
	BOOL	SaveDXFMakeOption(void);		// ڼ޽�؂ւ̕ۑ�

	int		GetNum(size_t n) const {		// ������߼��
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	double	GetDbl(size_t n) const {		// ������߼��
		ASSERT( n>=0 && n<SIZEOF(m_udNums) );
		return m_udNums[n];
	}
	BOOL	GetFlag(size_t n) const {		// �׸޵�߼��
		ASSERT( n>=0 && n<SIZEOF(m_ubFlags) );
		return m_ubFlags[n];
	}
	CString	GetStr(size_t n) const {		// �������߼��
		ASSERT( n>=0 && n<SIZEOF(m_strOption) );
		return m_strOption[n];
	}
};
