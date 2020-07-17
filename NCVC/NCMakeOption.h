// NCMakeOption.h: NC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

#define	MKNC_NUM_SPINDLE			0
#define	MKNC_NUM_LINEADD			1
#define	MKNC_NUM_G90				2
#define	MKNC_NUM_ZRETURN			3
#define	MKNC_NUM_DOT				4
#define	MKNC_NUM_FDOT				5
#define	MKNC_NUM_CIRCLECODE			6
#define	MKNC_NUM_IJ					7
#define	MKNC_NUM_MAKEEND			8
#define	MKNC_NUM_DEEPSPINDLE		9
#define	MKNC_NUM_DEEPZPROCESS		10
#define	MKNC_NUM_DEEPAPROCESS		11
#define	MKNC_NUM_DEEPCPROCESS		12
#define	MKNC_NUM_DRILLSPINDLE		13
#define	MKNC_NUM_DWELL				14
#define	MKNC_NUM_DWELLFORMAT		15
#define	MKNC_NUM_DRILLZPROCESS		16
#define	MKNC_NUM_DRILLPROCESS		17
#define	MKNC_NUM_DRILLSORT			18
#define	MKNC_NUM_DRILLCIRCLEPROCESS	19
#define	MKNC_NUM_MOVEZ				20
#define	MKNC_NUM_TOLERANCE			21
#define	MKNC_NUM_OPTIMAIZEDRILL		22

#define	MKNC_DBL_FEED			0
#define	MKNC_DBL_ZFEED			1
#define	MKNC_DBL_ZG0STOP		2
#define	MKNC_DBL_ZCUT			3
#define	MKNC_DBL_G92X			4
#define	MKNC_DBL_G92Y			5
#define	MKNC_DBL_G92Z			6
#define	MKNC_DBL_ELLIPSE		7
#define	MKNC_DBL_MAKEEND		8
#define	MKNC_DBL_MAKEENDFEED	9
#define	MKNC_DBL_DEEP			10
#define	MKNC_DBL_ZSTEP			11
#define	MKNC_DBL_DEEPFEED		12
#define	MKNC_DBL_DRILLFEED		13
#define	MKNC_DBL_DRILLR			14
#define	MKNC_DBL_DRILLZ			15
#define	MKNC_DBL_DRILLCIRCLE	16
#define	MKNC_DBL_TOLERANCE		17
#define	MKNC_DBL_DRILLMARGIN	18

#define	MKNC_FLG_XREV			0
#define	MKNC_FLG_YREV			1
#define	MKNC_FLG_LINEADD		2
#define	MKNC_FLG_ZEROCUT		3
#define	MKNC_FLG_GCLIP			4
#define	MKNC_FLG_DISABLESPINDLE	5
#define	MKNC_FLG_CIRCLEHALF		6
#define	MKNC_FLG_ELLIPSE		7
#define	MKNC_FLG_DEEP			8
#define	MKNC_FLG_HELICAL		9
#define	MKNC_FLG_DEEPFINISH		10
#define	MKNC_FLG_DRILLMATCH		11
#define	MKNC_FLG_DRILLCIRCLE	12
#define	MKNC_FLG_DRILLBREAK		13
#define	MKNC_FLG_LAYERCOMMENT	14
#define	MKNC_FLG_L0CYCLE		15

#define	MKNC_STR_LINEFORM		0
#define	MKNC_STR_EOB			1
#define	MKNC_STR_CUSTMOVE_B		2
#define	MKNC_STR_CUSTMOVE_A		3
#define	MKNC_STR_HEADER			4
#define	MKNC_STR_FOOTER			5

class CNCMakeOption
{
	CString	m_strInitFile;		// ����̧�ٖ�
	int		m_nOrderLength;		// �ő喽�ߒ�

// �؍����Ұ��ݒ���޲�۸ނ͂��F�B
friend class CMKNCSetup;
friend class CMKNCSetup1;
friend class CMKNCSetup2;
friend class CMKNCSetup3;
friend class CMKNCSetup4;
friend class CMKNCSetup5;
friend class CMKNCSetup6;
friend class CMKNCSetup8;

	// int�^��߼��
	union {
		struct {
			int		m_nSpindle,			// �厲��]���x
			// -----
					m_nLineAdd,			// �s�ԍ�����
					m_nG90,				// �ʒu�w��(G90 or G91)
					m_nZReturn,			// Z���̕��A(Initial or R)
					m_nDot,				// ���l�\�L(�����_ or 1/1000)
					m_nFDot,			// �e���Ұ��̐��l�\�L
					m_nCircleCode,		// �~�؍�(G2 or G3)
					m_nIJ,				// �~�ʕ�Ԃ�R��I/J/K
			// -----
					m_nMakeEnd,			// ���H�ςݐ[���̎w��
					m_nDeepSpindle,		// �[���d�グ��]��
					m_nDeepZProcess,	// �[���d�グ�����Z�����A
					m_nDeepAProcess,	// �[���̐؍�菇
					m_nDeepCProcess,	// �[���̐؍����
			// -----
					m_nDrillSpindle,	// ��������]��
					m_nDwell,			// �޳�َ���
					m_nDwellFormat,		// �޳�َ��Ԃ̕\�L
					m_nDrillZProcess,	// ��������Z�����A
					m_nDrillProcess,	// �������̎d��
					m_nDrillSort,		// �������̸�ٰ��ݸޏ���
					m_nDrillCircleProcess,	// �~�ް����܂ނƂ��̌���������
			// -----
					m_nMoveZ,			// �ړ�ڲԂ�Z��
			// -----
					m_nTolerance,		// ���e���𒴂����Ƃ��̓���
					m_nOptimaizeDrill;	// �����H�̊��
		};
		int			m_unNums[23];
	};
	// double�^��߼��
	union {
		struct {
			double	m_dFeed,			// �؍푗�葬�x
					m_dZFeed,			// Z����������Ƃ��̑��葬�x
					m_dZG0Stop,			// G0�œ�����Z�ʒu(R�_)
					m_dZCut,			// Z���̉����ʒu(�؂荞��)
					m_dG92[NCXYZ],		// G92��X/Y/Z
			// -----
					m_dEllipse,			// �ȉ~����
			// -----
					m_dMakeValue,		// ���H�ςݐ[���̵̾��or�Œ�Z�l
					m_dMakeFeed,		// ���H�ς�Z���葬�x
					m_dDeep,			// �[���̍ŏI�؂荞��
					m_dZStep,			// �[���؍�̂��߂̽ï��
					m_dDeepFeed,		// �[���d�グ����
			// -----
					m_dDrillFeed,		// ����������
					m_dDrillR,			// �����HR�_
					m_dDrillZ,			// �����H�؂荞��
					m_dDrillCircle,		// �����H�Ɍ����Ă�~�ް��̔��a
			// -----
					m_dTolerance,		// ������W�ƌ��Ȃ����e��
					m_dDrillMargin;		// ����ɑ΂��鋖�e��
		};
		double		m_udNums[19];
	};
	// BOOL�^��߼��
	union {
		struct {
			BOOL	m_bXrev,			// X�����]
					m_bYrev,			// Y�����]
					m_bLineAdd,			// �s�ԍ�
					m_bZeroCut,			// �����_�ȉ��̾�۶��
					m_bGclip,			// G���ޏȗ��`
					m_bDisableSpindle,	// S���Ұ��𐶐����Ȃ�
					m_bCircleHalf,		// �S�~�͕���
			// -----
					m_bEllipse,			// ���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�
			// -----
					m_bDeep,			// �[���؍���s��
					m_bHelical,			// �~�ް����ضِ؍�
					m_bDeepFinish,		// �d�グ��߼�ݓK�p��
			// -----
					m_bDrillMatch,		// �����H������W�͖���
					m_bDrillCircle,		// �~�ް��������H�ް��ƌ��Ȃ�
					m_bDrillBreak,		// �傫�����Ƃɺ��Ă𖄂ߍ���
			// -----
					m_bLayerComment,	// ڲԂ��Ƃɺ��Ă𖄂ߍ���
					m_bL0Cycle;			// �Œ軲�ْ���L0�o��
		};
		BOOL		m_ubFlags[16];
	};
	// CString�^��߼��
	CString		m_strOption[6];			// �s�ԍ�̫�ϯ�, EOB, ���шړ�����(�O��)
										// ����ͯ�ް�C����̯��

	BOOL	Convert();				// ���ް�ޮ݌݊��p
	void	InitialDefault(void);	// ��̫�Đݒ�
	CString	GetInsertSpace(int nLen) {
		CString	strResult(' ', m_nOrderLength - nLen);
		return strResult;
	}

public:
	CNCMakeOption(LPCTSTR);

	BOOL	ReadMakeOption(LPCTSTR);	// ����̧�ق̓ǂݍ���
	BOOL	SaveMakeOption(LPCTSTR = NULL);	// ����̧�ق̏����o��

	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
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

#ifdef _DEBUGOLD
	void	DbgDump(void) const;		// ��߼�ݕϐ��������
#endif
};
