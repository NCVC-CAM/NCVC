// MCOption.h: �H��@�B��߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
// ModalGroup
#define	MODALGROUP	5
#define	MODALGROUP0	0	// G00�`G03
#define	MODALGROUP1	1	// G17�`G19
#define	MODALGROUP2	2	// G54�`G59
#define	MODALGROUP3	3	// G90�CG91
#define	MODALGROUP4	4	// G98�CG99
// G54�`G59
#define	WORKOFFSET		6
// ϸۊ֘A
#define	MCMACROSTRING		4
#define	MCMACROCODE			0	// �Ăяo������
#define	MCMACROFOLDER		1	// ̫���
#define	MCMACROIF			2	// I/F
#define	MCMACROARGV			3	// ����
#define	MCMACRORESULT		4	// �o�͌���
#define	MCMACHINEFILE		5	// ���݂̋@�B���̧�ٖ�
#define	MCCURRENTFOLDER		6	// ���݂�NÇ��̫���
// BOOL�^
#define	MC_FLG_L0CYCLE		0
// �H��␳����
#define	MC_TYPE_A			0
#define	MC_TYPE_B			1

// �H����
#define	MCTOOLINFOOPT	4	// MCTOOLINFO �̵�߼�݋�؂萔
class CMCTOOLINFO
{
friend	class	CMCOption;
friend	class	CMCSetup3;

	BOOL	m_bDlgAdd, m_bDlgDel;
	int		m_nTool;
	CString	m_strName;
	double	m_dToolD, m_dToolH;

public:
	CMCTOOLINFO(void) {
		ClearOption();
	}
	CMCTOOLINFO(int nTool, const CString& strName, double dToolD, double dToolH,
			BOOL bDlgAdd = FALSE) {
		m_bDlgAdd	= bDlgAdd;
		m_bDlgDel	= FALSE;
		m_nTool		= nTool;
		m_strName	= strName;
		m_dToolD	= dToolD;
		m_dToolH	= dToolH;
	}

	void	ClearOption(void) {
		m_bDlgAdd = m_bDlgDel = FALSE;
		m_nTool = 0;
		m_strName.Empty();
		m_dToolD = m_dToolH = 0;
	}
};

class CMCOption
{
friend	class	CMCSetup1;
friend	class	CMCSetup2;
friend	class	CMCSetup3;
friend	class	CMCSetup4;
friend	class	CMCSetup5;

	CStringList	m_strMCList;	// �@�B���̧�ٗ���

	// int�^��߼��
	union {
		struct {
			int		m_nModal[MODALGROUP],	// Ӱ��ِݒ�
					m_nG0Speed[NCXYZ],		// �ʒu����(G0)�ړ����x
					m_nFDot,				// �F�� 0:sec 1:msec
					m_nCorrectType;			// �␳����
		};
		int			m_unNums[10];
	};
	// double�^��߼��
	union {
		struct {
			double	m_dFeed,				// �ȗ����̐؍푬�x
					m_dInitialXYZ[NCXYZ],	// XYZ�����l
					m_dBlock,				// 1��ۯ���������
					m_dWorkOffset[WORKOFFSET][NCXYZ];	// G54�`G59
		};
		double		m_udNums[23];
	};
	// BOOL�^��߼��
	union {
		struct {
			BOOL	m_bL0Cycle;		// �Œ軲�ْ���L0����
		};
		BOOL		m_ubFlgs[1];
	};
	// CString�^��߼��
	CString		m_strMCname,	// �@�B��
				m_strMacroOpt[MCMACROSTRING];	// ϸۊ֌W
	// �H�����߼��
	CTypedPtrList<CPtrList, CMCTOOLINFO*>	m_ltTool;	// CMCTOOLINFO�^���i�[

	void	Convert(void);			// ڼ޽�؂���̧�ق� & ���ް�ޮ݂�ڼ޽�؂�����
	void	ConvertWorkOffset(size_t, LPCTSTR);
	BOOL	AddMCListHistory(LPCTSTR);	// �����X�V

public:
	CMCOption();
	~CMCOption();
	BOOL	ReadMCoption(LPCTSTR, BOOL = TRUE);
	BOOL	SaveMCoption(LPCTSTR);

	BOOL	GetFlag(size_t n) const {		// �׸޵�߼��
		ASSERT( n>=0 && n<SIZEOF(m_ubFlgs) );
		return m_ubFlgs[n];
	}
	const	CStringList*	GetMCList(void) {
		return &m_strMCList;
	}
	CString	GetMCHeadFileName(void) const {
		CString	strResult;
		if ( !m_strMCList.IsEmpty() )
			strResult = m_strMCList.GetHead();
		return strResult;
	}
	int		GetModalSetting(size_t n) const {
		ASSERT( n>=0 && n<MODALGROUP );
		return m_nModal[n];
	}
	int		GetG0Speed(size_t n) const {
		ASSERT( n>=0 && n<NCXYZ );
		return m_nG0Speed[n];
	}
	BOOL	IsZeroG0Speed(void) const {
		if ( GetG0Speed(NCA_X)==0 || GetG0Speed(NCA_Y)==0 || GetG0Speed(NCA_Z)==0 )
			return TRUE;
		return FALSE;
	}
	double	GetBlockTime(void) const {
		return m_dBlock;
	}
	int		GetFDot(void) const {
		return m_nFDot;
	}
	double	GetFeed(void) const {
		return m_dFeed;
	}
	double	GetInitialXYZ(size_t n) const {
		ASSERT( n>=0 && n<NCXYZ );
		return m_dInitialXYZ[n];
	}
	CPoint3D	GetWorkOffset(size_t n) const {
		ASSERT( n>=0 && n<WORKOFFSET );
		return	CPoint3D(m_dWorkOffset[n][NCA_X], m_dWorkOffset[n][NCA_Y], m_dWorkOffset[n][NCA_Z]);
	}
	CString	GetMacroStr(int n) const {
		ASSERT( n>=0 && n<SIZEOF(m_strMacroOpt) );
		return m_strMacroOpt[n];
	}
	BOOL	IsMacroSearch(void) const {
		return !(m_strMacroOpt[MCMACROCODE].IsEmpty() | m_strMacroOpt[MCMACROIF].IsEmpty());
	}
	CString	MakeMacroCommand(int) const;
	CString	GetDefaultOption(void) const;	// from MCSetup4.cpp

	int		GetCorrectType(void) const {
		return m_nCorrectType;
	}
	boost::optional<double>	GetToolD(int) const;
	boost::optional<double>	GetToolH(int) const;
	BOOL	AddTool(int, double, BOOL);	// from NCDoc.cpp(G10)
	void	ReductionTools(BOOL);

	void	AddMCHistory_ComboBox(CComboBox&);
};
