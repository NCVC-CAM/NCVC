// NCMakeLatheOpt.h: ���՗pNC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {
	MKLA_NUM_SPINDLE = 0,		// �厲��]���x
	MKLA_NUM_MARGIN,			// �d�グ��
	MKLA_NUM_PROG,				// ��۸��єԍ�
	MKLA_NUM_LINEADD,			// �s�ԍ�����
	MKLA_NUM_G90,				// �ʒu�w��(G90 or G91)
	MKLA_NUM_DOT,				// ���l�\�L(�����_ or 1/1000)
	MKLA_NUM_FDOT,				// �e���Ұ��̐��l�\�L
	MKLA_NUM_CIRCLECODE,		// �~�؍�(G2 or G3)
	MKLA_NUM_IJ,				// �~�ʕ�Ԃ�R��I/J/K
		MKLA_NUM_NUMS		// [9]
};
enum {
	MKLA_DBL_FEED = 0,			// �؍푗��(Z)
	MKLA_DBL_XFEED,				// �؍푗��(X)
	MKLA_DBL_CUT,				// �؂荞��(���a�l)
	MKLA_DBL_PULL_Z,			// ������Z
	MKLA_DBL_PULL_X,			// ������X(���a�l)
	MKLA_DBL_MARGIN,			// �d�グ��(���a�l)
	MKLA_DBL_ELLIPSE,			// �ȉ~����
		MKLA_DBL_NUMS		// [7]
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
		MKLA_FLG_NUMS		// [9]
};
enum {
	MKLA_STR_LINEFORM = 0,		// �s�ԍ�̫�ϯ�
	MKLA_STR_EOB,				// EOB
	MKLA_STR_HEADER,			// ����ͯ�ް
	MKLA_STR_FOOTER,			// ����̯��
		MKLA_STR_NUMS		// [4]
};
//
#define	LTH_I_SPINDLE			m_pIntOpt[MKLA_NUM_SPINDLE]
#define	LTH_I_MARGIN			m_pIntOpt[MKLA_NUM_MARGIN]
#define	LTH_I_PROG				m_pIntOpt[MKLA_NUM_PROG]
#define	LTH_I_LINEADD			m_pIntOpt[MKLA_NUM_LINEADD]
#define	LTH_I_G90				m_pIntOpt[MKLA_NUM_G90]
#define	LTH_I_DOT				m_pIntOpt[MKLA_NUM_DOT]
#define	LTH_I_FDOT				m_pIntOpt[MKLA_NUM_FDOT]
#define	LTH_I_CIRCLECODE		m_pIntOpt[MKLA_NUM_CIRCLECODE]
#define	LTH_I_IJ				m_pIntOpt[MKLA_NUM_IJ]
//
#define	LTH_D_FEED				m_pDblOpt[MKLA_DBL_FEED]
#define	LTH_D_XFEED				m_pDblOpt[MKLA_DBL_XFEED]
#define	LTH_D_CUT				m_pDblOpt[MKLA_DBL_CUT]
#define	LTH_D_PULL_Z			m_pDblOpt[MKLA_DBL_PULL_Z]
#define	LTH_D_PULL_X			m_pDblOpt[MKLA_DBL_PULL_X]
#define	LTH_D_MARGIN			m_pDblOpt[MKLA_DBL_MARGIN]
#define	LTH_D_ELLIPSE			m_pDblOpt[MKLA_DBL_ELLIPSE]
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
//
class CNCMakeLatheOpt : public CNCMakeOption
{
	friend class CMKLASetup1;
	friend class CMKNCSetup2;
	friend class CMKNCSetup6;

	// �e�׽���ް�����������̂ŁA
	// union/struct�Z�͎g���Ȃ�
	// �ނ���C++�炵�����ިݸ�

protected:
	virtual	void	InitialDefault(void);

public:
	CNCMakeLatheOpt(LPCTSTR);

#ifdef _DEBUG
	virtual	void	DbgDump(void) const;	// ��߼�ݕϐ��������
#endif
};
