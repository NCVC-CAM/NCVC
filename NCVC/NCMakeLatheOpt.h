// NCMakeLatheOpt.h: ���՗pNC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

#define	MKLA_NUM_SPINDLE		0
#define	MKLA_NUM_MARGIN			1
#define	MKLA_NUM_PROG			2
#define	MKLA_NUM_LINEADD		3
#define	MKLA_NUM_G90			4
#define	MKLA_NUM_DOT			5
#define	MKLA_NUM_FDOT			6
#define	MKLA_NUM_CIRCLECODE		7
#define	MKLA_NUM_IJ				8

#define	MKLA_DBL_FEED			0
#define	MKLA_DBL_XFEED			1
#define	MKLA_DBL_CUT			2
#define	MKLA_DBL_PULL_Z			3
#define	MKLA_DBL_PULL_X			4
#define	MKLA_DBL_MARGIN			5
#define	MKLA_DBL_ELLIPSE		6

#define	MKLA_FLG_PROG			0
#define	MKLA_FLG_PROGAUTO		1
#define	MKLA_FLG_LINEADD		2
#define	MKLA_FLG_ZEROCUT		3
#define	MKLA_FLG_GCLIP			4
#define	MKLA_FLG_DISABLESPINDLE	5
#define	MKLA_FLG_CIRCLEHALF		6
#define	MKLA_FLG_ELLIPSE		7

#define	MKLA_STR_LINEFORM		0
#define	MKLA_STR_EOB			1
#define	MKLA_STR_HEADER			2
#define	MKLA_STR_FOOTER			3

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
		int			m_unNums[9];
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
		double		m_udNums[7];
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
			// -----
					m_bEllipse;			// ���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�
		};
		BOOL		m_ubFlags[8];
	};
	// CString�^��߼�� -> ���̂��ް��׽��
		// �s�ԍ�̫�ϯ�, EOB, ����ͯ�ް�C����̯��

public:
	CNCMakeLatheOpt(LPCTSTR);

	BOOL	ReadMakeOption(LPCTSTR);
	BOOL	SaveMakeOption(LPCTSTR = NULL);	// ����̧�ق̏����o��

#ifdef _DEBUGOLD
	void	DbgDump(void) const;		// ��߼�ݕϐ��������
#endif
};
