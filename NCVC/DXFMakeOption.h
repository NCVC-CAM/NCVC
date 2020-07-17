// DXFMakeOption.h: DXF�o�͵�߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

enum {
	MKDX_NUM_LTYPE_O = 0,
	MKDX_NUM_LTYPE_C,
	MKDX_NUM_LTYPE_M,
	MKDX_NUM_LTYPE_H,
	MKDX_NUM_LCOL_O,
	MKDX_NUM_LCOL_C,
	MKDX_NUM_LCOL_M,
	MKDX_NUM_LCOL_H,
	MKDX_NUM_PLANE,
	MKDX_NUM_CYCLE,
		MKDX_NUM_NUMS		// [10]
};
enum {
	MKDX_DBL_ORGLENGTH = 0,
	MKDX_DBL_CYCLER,
		MKDX_DBL_NUMS		// [2]
};
enum {
	MKDX_FLG_OUT_O = 0,
	MKDX_FLG_OUT_C,
	MKDX_FLG_OUT_M,
	MKDX_FLG_OUT_H,
	MKDX_FLG_ORGCIRCLE,
	MKDX_FLG_ORGCROSS,
		MKDX_FLG_NUMS		// [6]
};
enum {
	MKDX_STR_ORIGIN = 0,
	MKDX_STR_CAMLINE,
	MKDX_STR_MOVE,
	MKDX_STR_CORRECT,
		MKDX_STR_NUMS		// [4]
};

class CDXFMakeOption  
{
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
		int			m_unNums[MKDX_NUM_NUMS];
	};
	// float�^��߼��
	union {
		struct {
			float	m_dOrgLength,	// ���_����(�a)
					m_dCycleR;		// �Œ軲�ى~�o�͂̌a
		};
		float		m_udNums[MKDX_DBL_NUMS];
	};
	// BOOL�^��߼��
	union {
		struct {
			BOOL	m_bOut[4],		// �eڲԂ̏o���׸�
					m_bOrgCircle,	// ���_�~�o��
					m_bOrgCross;	// ���_�\���o��
		};
		BOOL		m_ubFlags[MKDX_FLG_NUMS];
	};
	// CString�^��߼��
	CString		m_strOption[MKDX_STR_NUMS];	// �e��ڲ�

	//
	void	Initialize_Registry(void);
	void	Initialize_Default(void);

public:
	CDXFMakeOption(BOOL bRegist = TRUE);
	BOOL	SaveDXFMakeOption(void);		// ڼ޽�؂ւ̕ۑ�

	int		GetNum(size_t n) const {		// ������߼��
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	float	GetDbl(size_t n) const {		// ������߼��
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
