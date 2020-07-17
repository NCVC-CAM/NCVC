// NCMakeWireOpt.h: ܲԕ��d���H�@�pNC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

// -- ��{MillOption����
#define	MKWI_NUM_PROG			0
#define	MKWI_NUM_LINEADD		1
#define	MKWI_NUM_G90			2
#define	MKWI_NUM_DOT			3
#define	MKWI_NUM_FDOT			4
#define	MKWI_NUM_CIRCLECODE		5
// --

#define	MKWI_DBL_DEPTH			0
#define	MKWI_DBL_TAPER			1
#define	MKWI_DBL_FEED			2
#define	MKWI_DBL_G92X			3
#define	MKWI_DBL_G92Y			4
#define	MKWI_DBL_AWFCIRCLE_LO	5
#define	MKWI_DBL_AWFCIRCLE_HI	6
#define	MKWI_DBL_ELLIPSE		7

// -- ��{MillOption����
#define	MKWI_FLG_PROG			0
#define	MKWI_FLG_PROGAUTO		1
#define	MKWI_FLG_LINEADD		2
#define	MKWI_FLG_ZEROCUT		3
#define	MKWI_FLG_GCLIP			4
#define	MKWI_FLG_ELLIPSE		5
// --
#define	MKWI_FLG_AWFSTART		6
#define	MKWI_FLG_AWFEND			7

#define	MKWI_STR_LINEFORM		0
#define	MKWI_STR_EOB			1
#define	MKWI_STR_HEADER			2
#define	MKWI_STR_FOOTER			3
#define	MKWI_STR_TAPERMODE		4
#define	MKWI_STR_AWFCNT			5
#define	MKWI_STR_AWFCUT			6

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
		int			m_unNums[6];
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
		double		m_udNums[8];
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
		BOOL		m_ubFlags[8];
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
