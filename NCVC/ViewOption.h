// ViewOption.h: �\���n��߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

struct	PENSTYLE
{
	LPCTSTR	lpszPenName;	// �޲�۸�(�����ޯ��)�o�^�p����
	int		nPenType;		// CPen�׽�ւ���ݽ���
	// --- �ȉ�DXF�p
	LPCTSTR	lpszDXFname;	// ���햼
	LPCTSTR	lpszDXFpattern;	// ���������(����3)
	int		nDXFdash;		// �ޯ���̒������ڐ�(����73==����49�̐�)
	float	dDXFpattern;	// ����ݑS�̒���(����40)
	float	dDXFdash[6];	// �ޯ������(����49)
	// --- �ȉ�OpenGL�p
	GLushort	nGLpattern;	// for glLineStipple
};
#define	MAXPENSTYLE		5

#define	VIEWUPDATE_REDRAW			0x0001
#define	VIEWUPDATE_DISPLAYLIST		0x0002
#define	VIEWUPDATE_LIGHT			0x0004
#define	VIEWUPDATE_BOXEL			0x0008
#define	VIEWUPDATE_TEXTURE			0x0010
#define	VIEWUPDATE_ALL				0x001f

enum {
	NCVIEWFLG_TRACEMARKER = 0,
	NCVIEWFLG_DRAWREVISE,
	NCVIEWFLG_DRAWCIRCLECENTER,
	NCVIEWFLG_GUIDESCALE,
	NCVIEWFLG_GUIDELENGTH,
	NCVIEWFLG_SOLIDVIEW,
	NCVIEWFLG_WIREPATH,
	NCVIEWFLG_DRAGRENDER,
	NCVIEWFLG_TEXTURE,
	NCVIEWFLG_LATHESLIT,
	NCVIEWFLG_NOACTIVETRACEGL,
	NCVIEWFLG_TOOLTRACE,
		NCVIEWFLG_NUMS		// [12]
};
enum {
	COMCOL_RECT = 0,
	COMCOL_SELECT,
		COMCOL_NUMS			// [2]
};
enum {
	NCCOL_BACKGROUND1 = 0,
	NCCOL_BACKGROUND2,
	NCCOL_PANE,
	NCCOL_GUIDEX, NCCOL_GUIDEY, NCCOL_GUIDEZ,
	NCCOL_G0, NCCOL_G1, NCCOL_G1Z,
	NCCOL_CYCLE,
	NCCOL_CENTERCIRCLE,
	NCCOL_WORK,
	NCCOL_MAXCUT,
	NCCOL_CORRECT,
	NCCOL_GL_WRK,
	NCCOL_GL_CUT,
	NCCOL_GL_MILL,
		NCCOL_NUMS			// [17]
};
enum {
	NCINFOCOL_BACKGROUND1 = 0,
	NCINFOCOL_BACKGROUND2,
	NCINFOCOL_TEXT,
		NCINFOCOL_NUMS		// [3]
};
enum {
	DXFCOL_BACKGROUND1 = 0,
	DXFCOL_BACKGROUND2,
	DXFCOL_ORIGIN,
	DXFCOL_CUTTER,
	DXFCOL_START,
	DXFCOL_MOVE,
	DXFCOL_TEXT,
	DXFCOL_OUTLINE,
	DXFCOL_WORK,
		DXFCOL_NUMS			// [9]
};
enum {
	// NCCOLLINE_GUIDE_[X|Y|Z] �� NCA_[X|Y|Z] �ő�p
	NCCOLLINE_G0 = NCXYZ,
	NCCOLLINE_G1,
	NCCOLLINE_G1Z,
	NCCOLLINE_CYCLE,
	NCCOLLINE_WORK,
	NCCOLLINE_MAXCUT,
		NCCOLLINE_NUMS		// [9]
};
const size_t	DXFCOLLINE_NUMS = 6;

class CViewOption
{
friend	class	CViewSetup1;
friend	class	CViewSetup2;
friend	class	CViewSetup3;
friend	class	CViewSetup4;
friend	class	CViewSetup5;
friend	class	CViewSetup6;
friend	class	CNCViewGL;		// OpenGL��߰ď󋵂ɂ�����׸ނ�����OFF

	DWORD	m_dwUpdateFlg;		// ViewSetup�ɂ�钼�O�̍X�V��

	BOOL	m_bMouseWheel;				// �g��k����ϳ�β�ق��g����
	union {
		struct {
			BOOL	m_bTraceMarker,		// �ڰ����̌��݈ʒu�\��
					m_bDrawRevise,		// �␳�O�ް��̕`��
					m_bDrawCircleCenter,// �~�ʕ�Ԃ̒��S��`��
					m_bScale,			// TRUE:�޲�ނɖڐ�
					m_bGuide,			// TRUE:�g�嗦�ɓ���
					m_bSolidView,		// OpenGL�د�ޕ\��
					m_bWirePath,		// ܲ��߽�\��
					m_bDragRender,		// ��ׯ�ޒ��������ݸ�
					m_bTexture,			// ø����̓\��t��
					m_bLatheSlit,		// ���Ղ̒f�ʕ\��
					m_bNoActiveTraceGL,	// ��è�ނł��ڰ�
					m_bToolTrace;		// �ڰ����ɍH���\��
		};
		BOOL		m_bNCFlag[NCVIEWFLG_NUMS];
	};
	COLORREF	m_colView[COMCOL_NUMS],	// �ޭ��̊e�F
				m_colNCView[NCCOL_NUMS],
				m_colNCInfoView[NCINFOCOL_NUMS],
				m_colDXFView[DXFCOL_NUMS],
				m_colCustom[16];		// �F�̐ݒ��޲�۸�(CColorDialog)
	int			m_nLineType[2],			// ����
				m_nNCLineType[NCCOLLINE_NUMS],
				m_nDXFLineType[DXFCOLLINE_NUMS],
				m_nWheelType,			// 0->��O:�g��,��:�k�� 1->�t
				m_nTraceSpeed[3],		// 0:����, 1:����, 2:�ᑬ
				m_nMillType,			// 0:�����, 1:�ް�, 2:�ʎ��
				m_nForceView01[4],		// 4��-1:����,�E��,����,�E��
				m_nForceView02[4];		// 4��-2:����,����,����,�E
	float		m_dGuide[NCXYZ],		// �޲�ގ��̒���
				m_dDefaultEndmill;		// ��̫�Ĵ����ٌa(���a)
	LOGFONT		m_lfFont[2];			// NC/DXF�Ŏg�p����̫�ď��
	CString		m_strTexture;			// ø����摜̧��

	void	AllDefaultSetting(void);

public:
	CViewOption();
	BOOL		SaveViewOption(void);	// ڼ޽�؂ւ̕ۑ�

	BOOL		IsMouseWheel(void) const {
		return m_bMouseWheel;
	}
	int			GetWheelType(void) const {
		return m_nWheelType;
	}
	BOOL		GetNCViewFlg(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_bNCFlag) );
		return m_bNCFlag[a];
	}
	int			GetTraceSpeed(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nTraceSpeed) );
		return m_nTraceSpeed[a];
	}
	COLORREF	GetDrawColor(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_colView) );
		return m_colView[a];
	}
	COLORREF	GetNcDrawColor(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_colNCView) );
		return m_colNCView[a];
	}
	COLORREF	GetNcInfoDrawColor(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_colNCInfoView) );
		return m_colNCInfoView[a];
	}
	COLORREF	GetDxfDrawColor(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_colDXFView) );
		return m_colDXFView[a];
	}
	int			GetDrawType(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nLineType) );
		return m_nLineType[a];
	}
	int			GetNcDrawType(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nNCLineType) );
		return m_nNCLineType[a];
	}
	int			GetDxfDrawType(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_nDXFLineType) );
		return m_nDXFLineType[a];
	}
	float	GetGuideLength(size_t a) const {
		ASSERT( a>=0 && a<SIZEOF(m_dGuide) );
		return m_dGuide[a];
	}
	float	GetDefaultEndmill(void) const {
		return m_dDefaultEndmill;
	}
	int		GetDefaultEndmillType(void) const {
		return m_nMillType;
	}
	const	LPLOGFONT	GetLogFont(DOCTYPE enType) {
		return &m_lfFont[enType];
	}
	CString	GetTextureFile(void) const {
		return m_strTexture;
	}
	int		GetForceView01(int n) const {
		return m_nForceView01[n]>=0 && m_nForceView01[n]<SIZEOF(m_nForceView01) ?
			m_nForceView01[n] : 0;
	}
	int		GetForceView02(int n) const {
		return m_nForceView02[n]>=0 && m_nForceView02[n]<SIZEOF(m_nForceView02) ?
			m_nForceView02[n] : 0;
	}

	// ����߰āC���߰�
	BOOL	Export(LPCTSTR);
	void	Inport(LPCTSTR);
};
