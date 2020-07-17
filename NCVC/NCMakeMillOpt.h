// NCMakeMillOpt.h: NC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "NCMakeOption.h"

enum {
	MKNC_NUM_PROG = 0,
	MKNC_NUM_LINEADD,
	MKNC_NUM_G90,
	MKNC_NUM_DOT,
	MKNC_NUM_FDOT,
	MKNC_NUM_CIRCLECODE,
	// -- �����܂Ŕh��WireOption����
	MKNC_NUM_SPINDLE,
	MKNC_NUM_ZRETURN,
	MKNC_NUM_IJ,
	MKNC_NUM_MAKEEND,
	MKNC_NUM_DEEPSPINDLE,
	MKNC_NUM_DEEPRETURN,
	MKNC_NUM_DEEPALL,
	MKNC_NUM_DEEPROUND,
	MKNC_NUM_DRILLSPINDLE,
	MKNC_NUM_DWELL,
	MKNC_NUM_DWELLFORMAT,
	MKNC_NUM_DRILLRETURN,
	MKNC_NUM_DRILLPROCESS,
	MKNC_NUM_DRILLSORT,
	MKNC_NUM_DRILLCIRCLEPROCESS,
	MKNC_NUM_MOVEZ,
	MKNC_NUM_TOLERANCE,
	MKNC_NUM_OPTIMAIZEDRILL,
		MKNC_NUM_NUMS		// [24]
};
enum {
	MKNC_DBL_FEED = 0,
	MKNC_DBL_ZFEED,
	MKNC_DBL_ZG0STOP,
	MKNC_DBL_ZCUT,
	MKNC_DBL_G92X,
	MKNC_DBL_G92Y,
	MKNC_DBL_G92Z,
	MKNC_DBL_ELLIPSE,
	MKNC_DBL_MAKEEND,
	MKNC_DBL_MAKEENDFEED,
	MKNC_DBL_DEEP,
	MKNC_DBL_ZSTEP,
	MKNC_DBL_DEEPFEED,
	MKNC_DBL_DRILLFEED,
	MKNC_DBL_DRILLR,
	MKNC_DBL_DRILLZ,
	MKNC_DBL_DRILLCIRCLE,
	MKNC_DBL_TOLERANCE,
	MKNC_DBL_DRILLMARGIN,
		MKNC_DBL_NUMS		// [19]
};
enum {
	MKNC_FLG_PROG = 0,
	MKNC_FLG_PROGAUTO,
	MKNC_FLG_LINEADD,
	MKNC_FLG_ZEROCUT,
	MKNC_FLG_GCLIP,
	MKNC_FLG_ELLIPSE,
	// -- �����܂Ŕh��WireOption����
	MKNC_FLG_XREV,
	MKNC_FLG_YREV,
	MKNC_FLG_DISABLESPINDLE,
	MKNC_FLG_CIRCLEHALF,
	MKNC_FLG_ZEROCUT_IJ,
	MKNC_FLG_DEEP,
	MKNC_FLG_HELICAL,
	MKNC_FLG_DEEPFINISH,
	MKNC_FLG_DRILLMATCH,
	MKNC_FLG_DRILLCIRCLE,
	MKNC_FLG_DRILLBREAK,
	MKNC_FLG_LAYERCOMMENT,
	MKNC_FLG_L0CYCLE,
		MKNC_FLG_NUMS		// [19]
};
enum {
	MKNC_STR_LINEFORM = 0,
	MKNC_STR_EOB,
	MKNC_STR_HEADER,
	MKNC_STR_FOOTER,
	MKNC_STR_CUSTMOVE_B,
	MKNC_STR_CUSTMOVE_A,
		MKNC_STR_NUMS		// [6]
};

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
			// -----
			int		m_nProg,			// ��۸��єԍ�
					m_nLineAdd,			// �s�ԍ�����
					m_nG90,				// �ʒu�w��(G90 or G91)
					m_nDot,				// ���l�\�L(�����_ or 1/1000)
					m_nFDot,			// �e���Ұ��̐��l�\�L
					m_nCircleCode,		// �~�؍�(G2 or G3)
			//
					m_nSpindle,			// �厲��]���x
					m_nZReturn,			// Z���̕��A(Initial or R)
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
		int			m_unNums[MKNC_NUM_NUMS];
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
		double		m_udNums[MKNC_DBL_NUMS];
	};
	// BOOL�^��߼��
	union {
		struct {
			BOOL	m_bProg,			// O�ԍ��t�^
					m_bProgAuto,		// ����ъ��蓖��
					m_bLineAdd,			// �s�ԍ�
					m_bZeroCut,			// �����_�ȉ��̾�۶��
					m_bGclip,			// G���ޏȗ��`
					m_bEllipse,			// ���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�
			//
					m_bXrev,			// X�����]
					m_bYrev,			// Y�����]
					m_bDisableSpindle,	// S���Ұ��𐶐����Ȃ�
					m_bCircleHalf,		// �S�~�͕���
					m_bZeroCutIJ,		// [I|J]0�͏ȗ�
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
		BOOL		m_ubFlags[MKNC_FLG_NUMS];
	};
	// CString�^��߼�� -> ���̂��ް��׽��
		// �s�ԍ�̫�ϯ�, EOB, ����ͯ�ް�C����̯��
		// ���шړ�����(�O��)

	BOOL	Convert();						// ���ް�ޮ݌݊��p

public:
	CNCMakeMillOpt(LPCTSTR);

#ifdef _DEBUGOLD
	virtual	void	DbgDump(void) const;	// ��߼�ݕϐ��������
#endif
};
