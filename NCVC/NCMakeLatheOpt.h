// NCMakeLatheOpt.h: ���՗pNC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {
	MKLA_NUM_SPINDLE = 0,
	MKLA_NUM_MARGIN,
	MKLA_NUM_PROG,
	MKLA_NUM_LINEADD,
	MKLA_NUM_G90,
	MKLA_NUM_DOT,
	MKLA_NUM_FDOT,
	MKLA_NUM_CIRCLECODE,
	MKLA_NUM_IJ,
		MKLA_NUM_NUMS		// [9]
};
enum {
	MKLA_DBL_FEED = 0,
	MKLA_DBL_XFEED,
	MKLA_DBL_CUT,
	MKLA_DBL_PULL_Z,
	MKLA_DBL_PULL_X,
	MKLA_DBL_MARGIN,
	MKLA_DBL_ELLIPSE,
		MKLA_DBL_NUMS		// [7]
};
enum {
	MKLA_FLG_PROG = 0,
	MKLA_FLG_PROGAUTO,
	MKLA_FLG_LINEADD,
	MKLA_FLG_ZEROCUT,
	MKLA_FLG_GCLIP,
	MKLA_FLG_DISABLESPINDLE,
	MKLA_FLG_CIRCLEHALF,
	MKLA_FLG_ZEROCUT_IJ,
	MKLA_FLG_ELLIPSE,
		MKLA_FLG_NUMS		// [9]
};
enum {
	MKLA_STR_LINEFORM = 0,
	MKLA_STR_EOB,
	MKLA_STR_HEADER,
	MKLA_STR_FOOTER,
		MKLA_STR_NUMS		// [4]
};

class CNCMakeLatheOpt : public CNCMakeOption
{
// �؍����Ұ��ݒ���޲�۸ނ͂��F�B
friend class CMKLASetup1;
friend class CMKNCSetup2;
friend class CMKNCSetup6;

	// int�^��߼��
	union {
		struct {
			int		m_nSpindle,			// �厲��]���x
					m_nMargin,			// �d�グ��
			// -----
					m_nProg,			// ��۸��єԍ�
					m_nLineAdd,			// �s�ԍ�����
					m_nG90,				// �ʒu�w��(G90 or G91)
					m_nDot,				// ���l�\�L(�����_ or 1/1000)
					m_nFDot,			// �e���Ұ��̐��l�\�L
					m_nCircleCode,		// �~�؍�(G2 or G3)
					m_nIJ;				// �~�ʕ�Ԃ�R��I/J/K
		};
		int			m_unNums[MKLA_NUM_NUMS];
	};
	// double�^��߼��
	union {
		struct {
			double	m_dFeed,			// �؍푗��(Z)
					m_dXFeed,			// �؍푗��(X)
					m_dCut,				// �؂荞��(���a�l)
					m_dPullZ,			// ������Z
					m_dPullX,			// ������X(���a�l)
					m_dMargin,			// �d�グ��(���a�l)
			// -----
					m_dEllipse;			// �ȉ~����
		};
		double		m_udNums[MKLA_DBL_NUMS];
	};
	// BOOL�^��߼��
	union {
		struct {
			BOOL	m_bProg,			// O�ԍ��t�^
					m_bProgAuto,		// ����ъ��蓖��
					m_bLineAdd,			// �s�ԍ�
					m_bZeroCut,			// �����_�ȉ��̾�۶��
					m_bGclip,			// G���ޏȗ��`
					m_bDisableSpindle,	// S���Ұ��𐶐����Ȃ�
					m_bCircleHalf,		// �S�~�͕���
					m_bZeroCutIJ,		// [I|J]0�͏ȗ�
			// -----
					m_bEllipse;			// ���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�
		};
		BOOL		m_ubFlags[MKLA_FLG_NUMS];
	};
	// CString�^��߼�� -> ���̂��ް��׽��
		// �s�ԍ�̫�ϯ�, EOB, ����ͯ�ް�C����̯��

public:
	CNCMakeLatheOpt(LPCTSTR);

#ifdef _DEBUGOLD
	virtual	void	DbgDump(void) const;	// ��߼�ݕϐ��������
#endif
};
