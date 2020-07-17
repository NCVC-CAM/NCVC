// NCMakeWireOpt.h: ܲԕ��d���H�@�pNC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {		// -- ��{MillOption����
	MKWI_NUM_PROG = 0,
	MKWI_NUM_LINEADD,
	MKWI_NUM_G90,
	MKWI_NUM_DOT,
	MKWI_NUM_FDOT,
	MKWI_NUM_CIRCLECODE,
		MKWI_NUM_NUMS		// [6]
};
enum {
	MKWI_DBL_DEPTH = 0,
	MKWI_DBL_TAPER,
	MKWI_DBL_FEED,
	MKWI_DBL_G92X,
	MKWI_DBL_G92Y,
	MKWI_DBL_AWFCIRCLE_LO,
	MKWI_DBL_AWFCIRCLE_HI,
	MKWI_DBL_ELLIPSE,
		MKWI_DBL_NUMS		// [8]
};
enum {		// -- ��{MillOption����
	MKWI_FLG_PROG = 0,
	MKWI_FLG_PROGAUTO,
	MKWI_FLG_LINEADD,
	MKWI_FLG_ZEROCUT,
	MKWI_FLG_GCLIP,
	MKWI_FLG_ELLIPSE,
	// --
	MKWI_FLG_AWFSTART,
	MKWI_FLG_AWFEND,
		MKWI_FLG_NUMS		// [8]
};
enum {
	MKWI_STR_LINEFORM = 0,
	MKWI_STR_EOB,
	MKWI_STR_HEADER,
	MKWI_STR_FOOTER,
	MKWI_STR_TAPERMODE,
	MKWI_STR_AWFCNT,
	MKWI_STR_AWFCUT,
		MKWI_STR_NUMS		// [7]
};

class CNCMakeWireOpt : public CNCMakeOption
{
// �؍����Ұ��ݒ���޲�۸ނ͂��F�B
friend class CMKWISetup1;
friend class CMKWISetup2;
friend class CMKNCSetup2;
friend class CMKNCSetup6;

	// int�^��߼��
	union {
		struct {
			int		m_nProg,			// ��۸��єԍ�
					m_nLineAdd,			// �s�ԍ�����
					m_nG90,				// �ʒu�w��(G90 or G91)
					m_nDot,				// ���l�\�L(�����_ or 1/1000)
					m_nFDot,			// �e���Ұ��̐��l�\�L
					m_nCircleCode;		// �~�؍�(G2 or G3)
		};
		int			m_unNums[MKWI_NUM_NUMS];
	};
	// double�^��߼��
	union {
		struct {
			double	m_dDepth,			// ܰ�����
					m_dTaper,			// ð�ߊp�x[deg]
					m_dFeed,			// �؍푗��
					m_dG92X,			// G92
					m_dG92Y,
					m_dAWFcircleLo,		// AWF�����Ώۉ~
					m_dAWFcircleHi,		// AWF�����Ώۉ~
			// -----
					m_dEllipse;			// �ȉ~����
		};
		double		m_udNums[MKWI_DBL_NUMS];
	};
	// BOOL�^��߼��
	union {
		struct {
			BOOL	m_bProg,			// O�ԍ��t�^
					m_bProgAuto,		// ����ъ��蓖��
					m_bLineAdd,			// �s�ԍ�
					m_bZeroCut,			// �����_�ȉ��̾�۶��
					m_bGclip,			// G���ޏȗ��`
			// -----
					m_bEllipse,			// ���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�
			//
					m_bAWFstart,		// ���H�O����
					m_bAWFend;			// ���H��ؒf
		};
		BOOL		m_ubFlags[MKWI_FLG_NUMS];
	};
	// CString�^��߼�� -> ���̂��ް��׽��
		// �s�ԍ�̫�ϯ�, EOB, ����ͯ�ް�C����̯��,
		// TaperMode, AWF��������, AWF�ؒf����

public:
	CNCMakeWireOpt(LPCTSTR);

	BOOL	IsAWFcircle(double r) {
		return m_dAWFcircleLo<=r && r<=m_dAWFcircleHi;
	}

#ifdef _DEBUGOLD
	virtual	void	DbgDump(void) const;	// ��߼�ݕϐ��������
#endif
};
