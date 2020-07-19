// NCMakeLatheOpt.h: ���՗pNC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {
	MKLA_NUM_PROG = 0,			// ��۸��єԍ�
	MKLA_NUM_LINEADD,			// �s�ԍ�����
	MKLA_NUM_G90,				// �ʒu�w��(G90 or G91)
	MKLA_NUM_DOT,				// ���l�\�L(�����_ or 1/1000)
	MKLA_NUM_FDOT,				// �e���Ұ��̐��l�\�L
	MKLA_NUM_CIRCLECODE,		// �~�؍�(G2 or G3)
	MKLA_NUM_IJ,				// �~�ʕ�Ԃ�R��I/J/K
	MKLA_NUM_E_SPINDLE,			// �[�ʎ厲��]��
	MKLA_NUM_I_SPINDLE,			// ���a�厲��]��
	MKLA_NUM_O_SPINDLE,			// �O�a�厲��]��
	MKLA_NUM_I_MARGIN,			// ���a�d�グ��
	MKLA_NUM_O_MARGIN,			// �O�a�d�グ��
		MKLA_NUM_NUMS		// [12]
};
enum {
	MKLA_DBL_O_FEED = 0,		// �O�a�؍푗��(Z)
	MKLA_DBL_O_FEEDX,			// �O�a�؍푗��(X)
	MKLA_DBL_O_CUT,				// �O�a�؂荞��(���a�l)
	MKLA_DBL_O_PULLZ,			// �O�a������Z
	MKLA_DBL_O_PULLX,			// �O�a������X(���a�l)
	MKLA_DBL_O_MARGIN,			// �O�a�d�グ��(���a�l)
	MKLA_DBL_ELLIPSE,			// �ȉ~����
	MKLA_DBL_E_FEED,			// �[�ʐ؍푗��
	MKLA_DBL_E_CUT,				// �[�ʐ؂荞��
	MKLA_DBL_E_STEP,			// �[�ʐ؂荞�ݽï��
	MKLA_DBL_E_PULLZ,			// �[�ʈ�����Z
	MKLA_DBL_E_PULLX,			// �[�ʈ�����X
	MKLA_DBL_DRILLZ,			// �����؂荞��
	MKLA_DBL_DRILLR,			// R�_
	MKLA_DBL_DRILLQ,			// Q�l
	MKLA_DBL_DRILLD,			// �߂�l
	MKLA_DBL_DWELL,				// �޳�َ���(Make�̊֌W��float)
	MKLA_DBL_HOLE,				// ������������
	MKLA_DBL_I_FEED,			// ���a�؍푗��(Z)
	MKLA_DBL_I_FEEDX,			// ���a�؍푗��(X)
	MKLA_DBL_I_CUT,				// ���a�؂荞��(���a�l)
	MKLA_DBL_I_PULLZ,			// ���a������Z
	MKLA_DBL_I_PULLX,			// ���a������X(���a�l)
	MKLA_DBL_I_MARGIN,			// ���a�d�グ��(���a�l)
		MKLA_DBL_NUMS		// [24]
};
enum {
	MKLA_FLG_PROG = 0,			// O�ԍ��t�^
	MKLA_FLG_PROGAUTO,			// ����ъ��蓖��
	MKLA_FLG_LINEADD,			// �s�ԍ�
	MKLA_FLG_ZEROCUT,			// �����_�ȉ��̾�۶��
	MKLA_FLG_GCLIP,				// G���ޏȗ��`
	MKLA_FLG_DISABLESPINDLE,	// S���Ұ��𐶐����Ȃ�
	MKLA_FLG_CIRCLEHALF,		// �S�~�͕���
	MKLA_FLG_ZEROCUT_IJ,		// [I|J]0�͏ȗ�
	MKLA_FLG_ELLIPSE,			// ���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�
	MKLA_FLG_ENDFACE,			// �[�ʏ������s��
	MKLA_FLG_CYCLE,				// �������Œ�T�C�N����
		MKLA_FLG_NUMS		// [11]
};
enum {
	MKLA_STR_LINEFORM = 0,		// �s�ԍ�̫�ϯ�
	MKLA_STR_EOB,				// EOB
	MKLA_STR_HEADER,			// ����ͯ�ް
	MKLA_STR_FOOTER,			// ����̯��
	MKLA_STR_DRILL,				// ��������
	MKLA_STR_DRILLSPINDLE,		// �����厲��]���x
	MKLA_STR_DRILLFEED,			// �������葬�x
	MKLA_STR_D_CUSTOM,			// �������Ѻ���
	MKLA_STR_E_CUSTOM,			// �[�ʶ��Ѻ���
	MKLA_STR_I_CUSTOM,			// ���a���Ѻ���
	MKLA_STR_O_CUSTOM,			// �O�a���Ѻ���
		MKLA_STR_NUMS		// [11]
};
//
#define	LTH_I_PROG				m_pIntOpt[MKLA_NUM_PROG]
#define	LTH_I_LINEADD			m_pIntOpt[MKLA_NUM_LINEADD]
#define	LTH_I_G90				m_pIntOpt[MKLA_NUM_G90]
#define	LTH_I_DOT				m_pIntOpt[MKLA_NUM_DOT]
#define	LTH_I_FDOT				m_pIntOpt[MKLA_NUM_FDOT]
#define	LTH_I_CIRCLECODE		m_pIntOpt[MKLA_NUM_CIRCLECODE]
#define	LTH_I_IJ				m_pIntOpt[MKLA_NUM_IJ]
#define	LTH_I_E_SPINDLE			m_pIntOpt[MKLA_NUM_E_SPINDLE]
#define	LTH_I_I_SPINDLE			m_pIntOpt[MKLA_NUM_I_SPINDLE]
#define	LTH_I_O_SPINDLE			m_pIntOpt[MKLA_NUM_O_SPINDLE]
#define	LTH_I_I_MARGIN			m_pIntOpt[MKLA_NUM_I_MARGIN]
#define	LTH_I_O_MARGIN			m_pIntOpt[MKLA_NUM_O_MARGIN]
//
#define	LTH_D_O_FEED			m_pDblOpt[MKLA_DBL_O_FEED]
#define	LTH_D_O_FEEDX			m_pDblOpt[MKLA_DBL_O_FEEDX]
#define	LTH_D_O_CUT				m_pDblOpt[MKLA_DBL_O_CUT]
#define	LTH_D_O_PULLZ			m_pDblOpt[MKLA_DBL_O_PULLZ]
#define	LTH_D_O_PULLX			m_pDblOpt[MKLA_DBL_O_PULLX]
#define	LTH_D_O_MARGIN			m_pDblOpt[MKLA_DBL_O_MARGIN]
#define	LTH_D_ELLIPSE			m_pDblOpt[MKLA_DBL_ELLIPSE]
#define	LTH_D_E_FEED			m_pDblOpt[MKLA_DBL_E_FEED]
#define	LTH_D_E_CUT				m_pDblOpt[MKLA_DBL_E_CUT]
#define	LTH_D_E_STEP			m_pDblOpt[MKLA_DBL_E_STEP]
#define	LTH_D_E_PULLZ			m_pDblOpt[MKLA_DBL_E_PULLZ]
#define	LTH_D_E_PULLX			m_pDblOpt[MKLA_DBL_E_PULLX]
#define	LTH_D_DRILLZ			m_pDblOpt[MKLA_DBL_DRILLZ]
#define	LTH_D_DRILLR			m_pDblOpt[MKLA_DBL_DRILLR]
#define	LTH_D_DRILLQ			m_pDblOpt[MKLA_DBL_DRILLQ]
#define	LTH_D_DRILLD			m_pDblOpt[MKLA_DBL_DRILLD]
#define	LTH_D_DWELL				m_pDblOpt[MKLA_DBL_DWELL]
#define	LTH_D_HOLE				m_pDblOpt[MKLA_DBL_HOLE]
#define	LTH_D_I_FEED			m_pDblOpt[MKLA_DBL_I_FEED]
#define	LTH_D_I_FEEDX			m_pDblOpt[MKLA_DBL_I_FEEDX]
#define	LTH_D_I_CUT				m_pDblOpt[MKLA_DBL_I_CUT]
#define	LTH_D_I_PULLZ			m_pDblOpt[MKLA_DBL_I_PULLZ]
#define	LTH_D_I_PULLX			m_pDblOpt[MKLA_DBL_I_PULLX]
#define	LTH_D_I_MARGIN			m_pDblOpt[MKLA_DBL_I_MARGIN]
//
#define	LTH_F_PROG				m_pFlgOpt[MKLA_FLG_PROG]
#define	LTH_F_PROGAUTO			m_pFlgOpt[MKLA_FLG_PROGAUTO]
#define	LTH_F_LINEADD			m_pFlgOpt[MKLA_FLG_LINEADD]
#define	LTH_F_ZEROCUT			m_pFlgOpt[MKLA_FLG_ZEROCUT]
#define	LTH_F_GCLIP				m_pFlgOpt[MKLA_FLG_GCLIP]
#define	LTH_F_DISABLESPINDLE	m_pFlgOpt[MKLA_FLG_DISABLESPINDLE]
#define	LTH_F_CIRCLEHALF		m_pFlgOpt[MKLA_FLG_CIRCLEHALF]
#define	LTH_F_ZEROCUT_IJ		m_pFlgOpt[MKLA_FLG_ZEROCUT_IJ]
#define	LTH_F_ELLIPSE			m_pFlgOpt[MKLA_FLG_ELLIPSE]
#define	LTH_F_ENDFACE			m_pFlgOpt[MKLA_FLG_ENDFACE]
#define	LTH_F_CYCLE				m_pFlgOpt[MKLA_FLG_CYCLE]
//
#define	LTH_S_LINEFORM			m_strOption[MKLA_STR_LINEFORM]
#define	LTH_S_EOB				m_strOption[MKLA_STR_EOB]
#define	LTH_S_HEADER			m_strOption[MKLA_STR_HEADER]
#define	LTH_S_FOOTER			m_strOption[MKLA_STR_FOOTER]
#define	LTH_S_DRILL				m_strOption[MKLA_STR_DRILL]
#define	LTH_S_DRILLSPINDLE		m_strOption[MKLA_STR_DRILLSPINDLE]
#define	LTH_S_DRILLFEED			m_strOption[MKLA_STR_DRILLFEED]
#define	LTH_S_D_CUSTOM			m_strOption[MKLA_STR_D_CUSTOM]
#define	LTH_S_E_CUSTOM			m_strOption[MKLA_STR_E_CUSTOM]
#define	LTH_S_I_CUSTOM			m_strOption[MKLA_STR_I_CUSTOM]
#define	LTH_S_O_CUSTOM			m_strOption[MKLA_STR_O_CUSTOM]
//
struct LATHEDRILLINFO
{
	float	d;	// �a
	int		s;	// ��]��
	float	f;	// ���葬�x
};
typedef	std::vector<LATHEDRILLINFO>		VLATHEDRILLINFO;
//
class CNCMakeLatheOpt : public CNCMakeOption
{
	friend class CMKLASetup0;
	friend class CMKLASetup1;
	friend class CMKLASetup2;
	friend class CMKLASetup3;
	friend class CMKLASetup4;
	friend class CMKNCSetup2;
	friend class CMKNCSetup6;

	// �e�׽���ް�����������̂ŁA
	// union/struct�Z�͎g���Ȃ�
	// �ނ���C++�炵�����ިݸ�

protected:
	virtual	void	InitialDefault(void);
	virtual	BOOL	IsPathID(int);

public:
	CNCMakeLatheOpt(LPCTSTR);

	BOOL	GetDrillInfo(VLATHEDRILLINFO&) const;
	virtual	CString	GetLineNoForm(void) const;

#ifdef _DEBUG
	virtual	void	DbgDump(void) const;	// ��߼�ݕϐ��������
#endif
};
