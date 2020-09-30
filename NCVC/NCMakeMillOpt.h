// NCMakeMillOpt.h: NC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "NCMakeOption.h"

enum {
	MKNC_NUM_PROG = 0,			// ��۸��єԍ�
	MKNC_NUM_LINEADD,			// �s�ԍ�����
	MKNC_NUM_G90,				// �ʒu�w��(G90 or G91)
	MKNC_NUM_DOT,				// ���l�\�L(����3�� or ����4�� or 1/1000)
	MKNC_NUM_FDOT,				// �e���Ұ��̐��l�\�L
	MKNC_NUM_CIRCLECODE,		// �~�؍�(G2 or G3)
	MKNC_NUM_SPINDLE,			// �厲��]���x
	MKNC_NUM_ZRETURN,			// Z���̕��A(Initial or R)
	MKNC_NUM_IJ,				// �~�ʕ�Ԃ�R��I/J/K
	MKNC_NUM_MAKEEND,			// ���H�ςݐ[���̎w��
	MKNC_NUM_DEEPSPINDLE,		// �[���d�グ��]��
	MKNC_NUM_DEEPRETURN,		// �[���d�グ�����Z�����A
	MKNC_NUM_DEEPALL,			// �[���̐؍�菇
	MKNC_NUM_DEEPROUND,			// �[���̐؍����
	MKNC_NUM_DRILLSPINDLE,		// ��������]��
	MKNC_NUM_DWELLFORMAT,		// �޳�َ��Ԃ̕\�L
	MKNC_NUM_DRILLRETURN,		// �������̉��H����
	MKNC_NUM_DRILLPROCESS,		// �������̎d��
	MKNC_NUM_DRILLSORT,			// �������̸�ٰ��ݸޏ���
	MKNC_NUM_DRILLCIRCLEPROCESS,// �~�ް����܂ނƂ��̌���������
	MKNC_NUM_MOVEZ,				// �ړ�ڲԂ�Z��
	MKNC_NUM_TOLERANCE,			// ���e���𒴂����Ƃ��̓���
	MKNC_NUM_OPTIMAIZEDRILL,	// �����H�̊��
		MKNC_NUM_NUMS		// [23]
};
enum {
	MKNC_DBL_FEED = 0,			// �؍푗�葬�x
	MKNC_DBL_ZFEED,				// Z����������Ƃ��̑��葬�x
	MKNC_DBL_ZG0STOP,			// G0�œ�����Z�ʒu(R�_)
	MKNC_DBL_ZCUT,				// Z���̉����ʒu(�؂荞��)
	MKNC_DBL_G92X,				// G92��X/Y/Z
	MKNC_DBL_G92Y,
	MKNC_DBL_G92Z,
	MKNC_DBL_ELLIPSE,			// �ȉ~����
	MKNC_DBL_MAKEEND,			// ���H�ςݐ[���̵̾��or�Œ�Z�l
	MKNC_DBL_MAKEENDFEED,		// ���H�ς�Z���葬�x
	MKNC_DBL_DEEP,				// �[���̍ŏI�؂荞��
	MKNC_DBL_ZSTEP,				// �[���؍�̂��߂̽ï��
	MKNC_DBL_DEEPFEED,			// �[���d�グ����
	MKNC_DBL_DRILLFEED,			// ����������
	MKNC_DBL_DRILLR,			// �����HR�_
	MKNC_DBL_DRILLZ,			// �����H�؂荞��
	MKNC_DBL_DRILLQ,			// �[��Q�l
	MKNC_DBL_DWELL,				// �޳�َ���
	MKNC_DBL_DRILLCIRCLE,		// �����H�Ɍ����Ă�~�ް��̔��a
	MKNC_DBL_TOLERANCE,			// ������W�ƌ��Ȃ����e��
	MKNC_DBL_DRILLMARGIN,		// ����ɑ΂��鋖�e��
		MKNC_DBL_NUMS		// [21]
};
enum {
	MKNC_FLG_PROG = 0,			// O�ԍ��t�^
	MKNC_FLG_PROGAUTO,			// ����ъ��蓖��
	MKNC_FLG_LINEADD,			// �s�ԍ�
	MKNC_FLG_ZEROCUT,			// �����_�ȉ��̾�۶��
	MKNC_FLG_GCLIP,				// G���ޏȗ��`
	MKNC_FLG_ELLIPSE,			// ���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�
	MKNC_FLG_XREV,				// X�����]
	MKNC_FLG_YREV,				// Y�����]
	MKNC_FLG_DISABLESPINDLE,	// S���Ұ��𐶐����Ȃ�
	MKNC_FLG_CIRCLEHALF,		// �S�~�͕���
	MKNC_FLG_ZEROCUT_IJ,		// [I|J]0�͏ȗ�
	MKNC_FLG_DEEP,				// �[���؍���s��
	MKNC_FLG_HELICAL,			// �~�ް����ضِ؍�
	MKNC_FLG_DEEPFINISH,		// �d�グ��߼�ݓK�p��
	MKNC_FLG_DRILLMATCH,		// �����H������W�͖���
	MKNC_FLG_DRILLCIRCLE,		// �~�ް��������H�ް��ƌ��Ȃ�
	MKNC_FLG_DRILLBREAK,		// �傫�����Ƃɺ��Ă𖄂ߍ���
	MKNC_FLG_LAYERCOMMENT,		// ڲԂ��Ƃɺ��Ă𖄂ߍ���
	MKNC_FLG_L0CYCLE,			// �Œ軲�ْ���L0�o��
		MKNC_FLG_NUMS		// [19]
};
enum {
	MKNC_STR_LINEFORM = 0,		// �s�ԍ�̫�ϯ�
	MKNC_STR_EOB,				// EOB
	MKNC_STR_HEADER,			// ����ͯ�ް
	MKNC_STR_FOOTER,			// ����̯��
	MKNC_STR_CUSTMOVE_B,		// ���шړ�����(�O��)
	MKNC_STR_CUSTMOVE_A,
	MKNC_STR_PERLSCRIPT,		// ������Ɏ��s�����Perl������
		MKNC_STR_NUMS		// [7]
};
//
#define	MIL_I_PROG					m_pIntOpt[MKNC_NUM_PROG]
#define	MIL_I_LINEADD				m_pIntOpt[MKNC_NUM_LINEADD]
#define	MIL_I_G90					m_pIntOpt[MKNC_NUM_G90]
#define	MIL_I_DOT					m_pIntOpt[MKNC_NUM_DOT]
#define	MIL_I_FDOT					m_pIntOpt[MKNC_NUM_FDOT]
#define	MIL_I_CIRCLECODE			m_pIntOpt[MKNC_NUM_CIRCLECODE]
#define	MIL_I_SPINDLE				m_pIntOpt[MKNC_NUM_SPINDLE]
#define	MIL_I_ZRETURN				m_pIntOpt[MKNC_NUM_ZRETURN]
#define	MIL_I_IJ					m_pIntOpt[MKNC_NUM_IJ]
#define	MIL_I_MAKEEND				m_pIntOpt[MKNC_NUM_MAKEEND]
#define	MIL_I_DEEPSPINDLE			m_pIntOpt[MKNC_NUM_DEEPSPINDLE]
#define	MIL_I_DEEPRETURN			m_pIntOpt[MKNC_NUM_DEEPRETURN]
#define	MIL_I_DEEPALL				m_pIntOpt[MKNC_NUM_DEEPALL]
#define	MIL_I_DEEPROUND				m_pIntOpt[MKNC_NUM_DEEPROUND]
#define	MIL_I_DRILLSPINDLE			m_pIntOpt[MKNC_NUM_DRILLSPINDLE]
#define	MIL_I_DWELLFORMAT			m_pIntOpt[MKNC_NUM_DWELLFORMAT]
#define	MIL_I_DRILLRETURN			m_pIntOpt[MKNC_NUM_DRILLRETURN]
#define	MIL_I_DRILLPROCESS			m_pIntOpt[MKNC_NUM_DRILLPROCESS]
#define	MIL_I_DRILLSORT				m_pIntOpt[MKNC_NUM_DRILLSORT]
#define	MIL_I_DRILLCIRCLEPROCESS	m_pIntOpt[MKNC_NUM_DRILLCIRCLEPROCESS]
#define	MIL_I_MOVEZ					m_pIntOpt[MKNC_NUM_MOVEZ]
#define	MIL_I_TOLERANCE				m_pIntOpt[MKNC_NUM_TOLERANCE]
#define	MIL_I_OPTIMAIZEDRILL		m_pIntOpt[MKNC_NUM_OPTIMAIZEDRILL]
//
#define	MIL_D_FEED					m_pDblOpt[MKNC_DBL_FEED]
#define	MIL_D_ZFEED					m_pDblOpt[MKNC_DBL_ZFEED]
#define	MIL_D_ZG0STOP				m_pDblOpt[MKNC_DBL_ZG0STOP]
#define	MIL_D_ZCUT					m_pDblOpt[MKNC_DBL_ZCUT]
#define	MIL_D_G92X					m_pDblOpt[MKNC_DBL_G92X]
#define	MIL_D_G92Y					m_pDblOpt[MKNC_DBL_G92Y]
#define	MIL_D_G92Z					m_pDblOpt[MKNC_DBL_G92Z]
#define	MIL_D_ELLIPSE				m_pDblOpt[MKNC_DBL_ELLIPSE]
#define	MIL_D_MAKEEND				m_pDblOpt[MKNC_DBL_MAKEEND]
#define	MIL_D_MAKEENDFEED			m_pDblOpt[MKNC_DBL_MAKEENDFEED]
#define	MIL_D_DEEP					m_pDblOpt[MKNC_DBL_DEEP]
#define	MIL_D_ZSTEP					m_pDblOpt[MKNC_DBL_ZSTEP]
#define	MIL_D_DEEPFEED				m_pDblOpt[MKNC_DBL_DEEPFEED]
#define	MIL_D_DRILLFEED				m_pDblOpt[MKNC_DBL_DRILLFEED]
#define	MIL_D_DRILLR				m_pDblOpt[MKNC_DBL_DRILLR]
#define	MIL_D_DRILLZ				m_pDblOpt[MKNC_DBL_DRILLZ]
#define	MIL_D_DRILLQ				m_pDblOpt[MKNC_DBL_DRILLQ]
#define	MIL_D_DWELL					m_pDblOpt[MKNC_DBL_DWELL]
#define	MIL_D_DRILLCIRCLE			m_pDblOpt[MKNC_DBL_DRILLCIRCLE]
#define	MIL_D_TOLERANCE				m_pDblOpt[MKNC_DBL_TOLERANCE]
#define	MIL_D_DRILLMARGIN			m_pDblOpt[MKNC_DBL_DRILLMARGIN]
//
#define	MIL_F_PROG					m_pFlgOpt[MKNC_FLG_PROG]
#define	MIL_F_PROGAUTO				m_pFlgOpt[MKNC_FLG_PROGAUTO]
#define	MIL_F_LINEADD				m_pFlgOpt[MKNC_FLG_LINEADD]
#define	MIL_F_ZEROCUT				m_pFlgOpt[MKNC_FLG_ZEROCUT]
#define	MIL_F_GCLIP					m_pFlgOpt[MKNC_FLG_GCLIP]
#define	MIL_F_ELLIPSE				m_pFlgOpt[MKNC_FLG_ELLIPSE]
#define	MIL_F_XREV					m_pFlgOpt[MKNC_FLG_XREV]
#define	MIL_F_YREV					m_pFlgOpt[MKNC_FLG_YREV]
#define	MIL_F_DISABLESPINDLE		m_pFlgOpt[MKNC_FLG_DISABLESPINDLE]
#define	MIL_F_CIRCLEHALF			m_pFlgOpt[MKNC_FLG_CIRCLEHALF]
#define	MIL_F_ZEROCUT_IJ			m_pFlgOpt[MKNC_FLG_ZEROCUT_IJ]
#define	MIL_F_DEEP					m_pFlgOpt[MKNC_FLG_DEEP]
#define	MIL_F_HELICAL				m_pFlgOpt[MKNC_FLG_HELICAL]
#define	MIL_F_DEEPFINISH			m_pFlgOpt[MKNC_FLG_DEEPFINISH]
#define	MIL_F_DRILLMATCH			m_pFlgOpt[MKNC_FLG_DRILLMATCH]
#define	MIL_F_DRILLCIRCLE			m_pFlgOpt[MKNC_FLG_DRILLCIRCLE]
#define	MIL_F_DRILLBREAK			m_pFlgOpt[MKNC_FLG_DRILLBREAK]
#define	MIL_F_LAYERCOMMENT			m_pFlgOpt[MKNC_FLG_LAYERCOMMENT]
#define	MIL_F_L0CYCLE				m_pFlgOpt[MKNC_FLG_L0CYCLE]
//
#define	MIL_S_LINEFORM				m_strOption[MKNC_STR_LINEFORM]
#define	MIL_S_EOB					m_strOption[MKNC_STR_EOB]
#define	MIL_S_HEADER				m_strOption[MKNC_STR_HEADER]
#define	MIL_S_FOOTER				m_strOption[MKNC_STR_FOOTER]
#define	MIL_S_CUSTMOVE_B			m_strOption[MKNC_STR_CUSTMOVE_B]
#define	MIL_S_CUSTMOVE_A			m_strOption[MKNC_STR_CUSTMOVE_A]
#define	MIL_S_PERLSCRIPT			m_strOption[MKNC_STR_PERLSCRIPT]
//
class CNCMakeMillOpt : public CNCMakeOption
{
	friend class CMKNCSetup1;
	friend class CMKNCSetup2;
	friend class CMKNCSetup3;
	friend class CMKNCSetup4;
	friend class CMKNCSetup5;
	friend class CMKNCSetup6;
	friend class CMKNCSetup8;

	// �e�׽���ް�����������̂ŁA
	// union/struct�Z�͎g���Ȃ�
	// �ނ���C++�炵�����ިݸ�

	BOOL	Convert();						// ���ް�ޮ݌݊��p

protected:
	virtual	void	InitialDefault(void);
	virtual	BOOL	IsPathID(int);

public:
	CNCMakeMillOpt(LPCTSTR);

	virtual	CString	GetLineNoForm(void) const;

#ifdef _DEBUG
	virtual	void	DbgDump(void) const;	// ��߼�ݕϐ��������
#endif
};
