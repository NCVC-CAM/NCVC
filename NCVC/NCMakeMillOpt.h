// NCMakeMillOpt.h: NC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "NCMakeOption.h"

#define	MKNC_NUM_SPINDLE			0
#define	MKNC_NUM_PROG				1
#define	MKNC_NUM_LINEADD			2
#define	MKNC_NUM_G90				3
#define	MKNC_NUM_ZRETURN			4
#define	MKNC_NUM_DOT				5
#define	MKNC_NUM_FDOT				6
#define	MKNC_NUM_CIRCLECODE			7
#define	MKNC_NUM_IJ					8
#define	MKNC_NUM_MAKEEND			9
#define	MKNC_NUM_DEEPSPINDLE		10
#define	MKNC_NUM_DEEPRETURN			11
#define	MKNC_NUM_DEEPALL			12
#define	MKNC_NUM_DEEPROUND			13
#define	MKNC_NUM_DRILLSPINDLE		14
#define	MKNC_NUM_DWELL				15
#define	MKNC_NUM_DWELLFORMAT		16
#define	MKNC_NUM_DRILLRETURN		17
#define	MKNC_NUM_DRILLPROCESS		18
#define	MKNC_NUM_DRILLSORT			19
#define	MKNC_NUM_DRILLCIRCLEPROCESS	20
#define	MKNC_NUM_MOVEZ				21
#define	MKNC_NUM_TOLERANCE			22
#define	MKNC_NUM_OPTIMAIZEDRILL		23

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
#define	MKNC_FLG_PROG			2
#define	MKNC_FLG_PROGAUTO		3
#define	MKNC_FLG_LINEADD		4
#define	MKNC_FLG_ZEROCUT		5
#define	MKNC_FLG_GCLIP			6
#define	MKNC_FLG_DISABLESPINDLE	7
#define	MKNC_FLG_CIRCLEHALF		8
#define	MKNC_FLG_ELLIPSE		9
#define	MKNC_FLG_DEEP			10
#define	MKNC_FLG_HELICAL		11
#define	MKNC_FLG_DEEPFINISH		12
#define	MKNC_FLG_DRILLMATCH		13
#define	MKNC_FLG_DRILLCIRCLE	14
#define	MKNC_FLG_DRILLBREAK		15
#define	MKNC_FLG_LAYERCOMMENT	16
#define	MKNC_FLG_L0CYCLE		17

#define	MKNC_STR_LINEFORM		0
#define	MKNC_STR_EOB			1
#define	MKNC_STR_HEADER			2
#define	MKNC_STR_FOOTER			3
#define	MKNC_STR_CUSTMOVE_B		4
#define	MKNC_STR_CUSTMOVE_A		5

class CNCMakeMillOpt : public CNCMakeOption
{
// �؍����Ұ��ݒ���޲�۸ނ͂��F�B
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
					m_nProg,			// ��۸��єԍ�
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
					m_nDeepReturn,		// �[���d�グ�����Z�����A
					m_nDeepAll,			// �[���̐؍�菇
					m_nDeepRound,		// �[���̐؍����
			// -----
					m_nDrillSpindle,	// ��������]��
					m_nDwell,			// �޳�َ���
					m_nDwellFormat,		// �޳�َ��Ԃ̕\�L
					m_nDrillReturn,		// ��������Z�����A
					m_nDrillProcess,	// �������̎d��
					m_nDrillSort,		// �������̸�ٰ��ݸޏ���
					m_nDrillCircleProcess,	// �~�ް����܂ނƂ��̌���������
			// -----
					m_nMoveZ,			// �ړ�ڲԂ�Z��
			// -----
					m_nTolerance,		// ���e���𒴂����Ƃ��̓���
					m_nOptimaizeDrill;	// �����H�̊��
		};
		int			m_unNums[24];
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
					m_bProg,			// O�ԍ��t�^
					m_bProgAuto,		// ����ъ��蓖��
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
		BOOL		m_ubFlags[18];
	};
	// CString�^��߼�� -> ���̂��ް��׽��
		// �s�ԍ�̫�ϯ�, EOB, ����ͯ�ް�C����̯��
		// ���шړ�����(�O��)

	BOOL	Convert();						// ���ް�ޮ݌݊��p

public:
	CNCMakeMillOpt(LPCTSTR);

	BOOL	ReadMakeOption(LPCTSTR);
	BOOL	SaveMakeOption(LPCTSTR = NULL);	// ����̧�ق̏����o��

#ifdef _DEBUGOLD
	void	DbgDump(void) const;		// ��߼�ݕϐ��������
#endif
};
