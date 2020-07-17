// NCMakeWireOpt.h: ܲԕ��d���H�@�pNC������߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {
	MKWI_NUM_PROG = 0,			// ��۸��єԍ�
	MKWI_NUM_LINEADD,			// �s�ԍ�����
	MKWI_NUM_G90,				// �ʒu�w��(G90 or G91)
	MKWI_NUM_DOT,				// ���l�\�L(�����_ or 1/1000)
	MKWI_NUM_FDOT,				// �e���Ұ��̐��l�\�L
	MKWI_NUM_CIRCLECODE,		// �~�؍�(G2 or G3)
		MKWI_NUM_NUMS		// [6]
};
enum {
	MKWI_DBL_DEPTH = 0,			// ܰ�����
	MKWI_DBL_TAPER,				// ð�ߊp�x[deg]
	MKWI_DBL_FEED,				// �؍푗��
	MKWI_DBL_G92X,				// G92
	MKWI_DBL_G92Y,
	MKWI_DBL_AWFCIRCLE_LO,		// AWF�����Ώۉ~
	MKWI_DBL_AWFCIRCLE_HI,
	MKWI_DBL_ELLIPSE,			// �ȉ~����
		MKWI_DBL_NUMS		// [8]
};
enum {
	MKWI_FLG_PROG = 0,			// O�ԍ��t�^
	MKWI_FLG_PROGAUTO,			// ����ъ��蓖��
	MKWI_FLG_LINEADD,			// �s�ԍ�
	MKWI_FLG_ZEROCUT,			// �����_�ȉ��̾�۶��
	MKWI_FLG_GCLIP,				// G���ޏȗ��`
	MKWI_FLG_ELLIPSE,			// ���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�
	MKWI_FLG_AWFSTART,			// ���H�O����
	MKWI_FLG_AWFEND,			// ���H��ؒf
		MKWI_FLG_NUMS		// [8]
};
enum {
	MKWI_STR_LINEFORM = 0,		// �s�ԍ�̫�ϯ�
	MKWI_STR_EOB,				// EOB
	MKWI_STR_HEADER,			// ����ͯ�ް
	MKWI_STR_FOOTER,			// ����̯��
	MKWI_STR_TAPERMODE,			// TaperMode
	MKWI_STR_AWFCNT,			// AWF��������
	MKWI_STR_AWFCUT,			// AWF�ؒf����
		MKWI_STR_NUMS		// [7]
};
//
#define	WIR_I_PROG			m_pIntOpt[MKWI_NUM_PROG]
#define	WIR_I_LINEADD		m_pIntOpt[MKWI_NUM_LINEADD]
#define	WIR_I_G90			m_pIntOpt[MKWI_NUM_G90]
#define	WIR_I_DOT			m_pIntOpt[MKWI_NUM_DOT]
#define	WIR_I_FDOT			m_pIntOpt[MKWI_NUM_FDOT]
#define	WIR_I_CIRCLECODE	m_pIntOpt[MKWI_NUM_CIRCLECODE]
//
#define	WIR_D_DEPTH			m_pDblOpt[MKWI_DBL_DEPTH]
#define	WIR_D_TAPER			m_pDblOpt[MKWI_DBL_TAPER]
#define	WIR_D_FEED			m_pDblOpt[MKWI_DBL_FEED]
#define	WIR_D_G92X			m_pDblOpt[MKWI_DBL_G92X]
#define	WIR_D_G92Y			m_pDblOpt[MKWI_DBL_G92Y]
#define	WIR_D_AWFCIRCLE_LO	m_pDblOpt[MKWI_DBL_AWFCIRCLE_LO]
#define	WIR_D_AWFCIRCLE_HI	m_pDblOpt[MKWI_DBL_AWFCIRCLE_HI]
#define	WIR_D_ELLIPSE		m_pDblOpt[MKWI_DBL_ELLIPSE]
//
#define	WIR_F_PROG			m_pFlgOpt[MKWI_FLG_PROG]
#define	WIR_F_PROGAUTO		m_pFlgOpt[MKWI_FLG_PROGAUTO]
#define	WIR_F_LINEADD		m_pFlgOpt[MKWI_FLG_LINEADD]
#define	WIR_F_ZEROCUT		m_pFlgOpt[MKWI_FLG_ZEROCUT]
#define	WIR_F_GCLIP			m_pFlgOpt[MKWI_FLG_GCLIP]
#define	WIR_F_ELLIPSE		m_pFlgOpt[MKWI_FLG_ELLIPSE]
#define	WIR_F_AWFSTART		m_pFlgOpt[MKWI_FLG_AWFSTART]
#define	WIR_F_AWFEND		m_pFlgOpt[MKWI_FLG_AWFEND]
//
class CNCMakeWireOpt : public CNCMakeOption
{
	friend class CMKWISetup1;
	friend class CMKWISetup2;
	friend class CMKNCSetup2;
	friend class CMKNCSetup6;

	// �e�׽���ް�����������̂ŁA
	// union/struct�Z�͎g���Ȃ�
	// �ނ���C++�炵�����ިݸ�

protected:
	virtual	void	InitialDefault(void);
	virtual	BOOL	IsPathID(int);

public:
	CNCMakeWireOpt(LPCTSTR);

	BOOL	IsAWFcircle(double r) {
		return WIR_D_AWFCIRCLE_LO<=r && r<=WIR_D_AWFCIRCLE_HI;
	}

#ifdef _DEBUG
	virtual	void	DbgDump(void) const;	// ��߼�ݕϐ��������
#endif
};
