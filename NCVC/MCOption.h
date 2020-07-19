// MCOption.h: 工作機械ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

// ModalGroup
enum {
	MODALGROUP0 = 0,	// G00～G03
	MODALGROUP1,		// G17～G19
	MODALGROUP2,		// G54～G59
	MODALGROUP3,		// G90，G91
	MODALGROUP4,		// G98，G99
		MODALGROUP		// [5]
};
enum {		// int型
	MC_INT_FDOT = MODALGROUP+NCXYZ,
	MC_INT_CORRECTTYPE,
	MC_INT_FORCEVIEWMODE,
		MC_INT_NUMS		// [11]
};
enum {		// float型
	MC_DBL_FEED = NCXYZ,
	MC_DBL_BLOCKWAIT,
	MC_DBL_DEFWIREDEPTH,
		MC_DBL_AAA		// [6](dummy)
};
#define	WORKOFFSET		6	// G54～G59
const	size_t	MC_DBL_NUMS = MC_DBL_AAA+WORKOFFSET*NCXYZ;	// 24
enum {		// BOOL型
	MC_FLG_OBS0 = 0,	// ｵﾌﾟｼｮﾅﾙﾌﾞﾛｯｸｽｷｯﾌﾟ
	MC_FLG_OBS1,
	MC_FLG_OBS2,
	MC_FLG_OBS3,
	MC_FLG_OBS4,
	MC_FLG_OBS5,
	MC_FLG_OBS6,
	MC_FLG_OBS7,
	MC_FLG_OBS8,
	MC_FLG_OBS9,
	MC_FLG_L0CYCLE,		// 固定ｻｲｸﾙのL0指定
		MC_FLG_NUMS		// [11]
};
enum {		// ﾏｸﾛ関連(CString型)
	MCMACROCODE = 0,	// 呼び出しｺｰﾄﾞ
	MCMACROFOLDER,		// ﾌｫﾙﾀﾞ
	MCMACROIF,			// I/F
	MCMACROARGV,		// 引数
		MCMACROSTRING	// [4]
};
enum {		// 置換用
	MCMACRORESULT = MCMACROSTRING,	// 出力結果
	MCMACHINEFILE,					// 現在の機械情報ﾌｧｲﾙ名
	MCCURRENTFOLDER,				// 現在のNCﾌｧｲﾙﾌｫﾙﾀﾞ
};
// 工具補正ﾀｲﾌﾟ
enum {
	MC_TYPE_A = 0,
	MC_TYPE_B
};
// 強制表示ﾓｰﾄﾞ(MC_INT_FORCEVIEWMODE)
enum {
	MC_VIEWMODE_MILL = 0,
	MC_VIEWMODE_LATHE,
	MC_VIEWMODE_WIRE
};

// 工具情報
enum {
	MCTOOL_T = 0,
	MCTOOL_NAME,
	MCTOOL_D,
	MCTOOL_H,
	MCTOOL_TYPE,
		MCTOOL_NUMS		// [5]
};
class CMCTOOLINFO
{
friend	class	CMCOption;
friend	class	CMCSetup3;

	BOOL	m_bDlgAdd, m_bDlgDel;
	int		m_nTool, m_nType;
	CString	m_strName;
	float	m_dToolD, m_dToolH;

public:
	CMCTOOLINFO(void) {
		ClearOption();
	}
	CMCTOOLINFO(int nTool, const CString& strName, float dToolD, float dToolH, int nType, 
			BOOL bDlgAdd = FALSE) {
		m_bDlgAdd	= bDlgAdd;
		m_bDlgDel	= FALSE;
		m_nTool		= nTool;
		m_strName	= strName;
		m_dToolD	= dToolD;
		m_dToolH	= dToolH;
		m_nType		= nType;
	}

	void	ClearOption(void) {
		m_bDlgAdd = m_bDlgDel = FALSE;
		m_nTool = m_nType = 0;
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
friend	class	COBSdlg;

	CStringList	m_strMCList;	// 機械情報ﾌｧｲﾙ履歴

	// int型ｵﾌﾟｼｮﾝ
	union {
		struct {
			int		m_nModal[MODALGROUP],	// ﾓｰﾀﾞﾙ設定
					m_nG0Speed[NCXYZ],		// 位置決め(G0)移動速度
					m_nFDot,				// 認識 0:sec 1:msec
					m_nCorrectType,			// 補正ﾀｲﾌﾟ
					m_nForceViewMode;		// 強制表示ﾓｰﾄﾞ
		};
		int			m_unNums[MC_INT_NUMS];
	};
	// float型ｵﾌﾟｼｮﾝ
	union {
		struct {
			float	m_dInitialXYZ[NCXYZ],	// XYZ初期値
					m_dFeed,				// 省略時の切削速度
					m_dBlock,				// 1ﾌﾞﾛｯｸ処理時間
					m_dDefWireDepth,		// ﾜｲﾔ加工機のﾃﾞﾌｫﾙﾄ厚さ
					m_dWorkOffset[WORKOFFSET][NCXYZ];	// G54～G59
		};
		float		m_udNums[MC_DBL_NUMS];
	};
	// BOOL型ｵﾌﾟｼｮﾝ
	union {
		struct {
			BOOL	m_bOBS[10],		// ｵﾌﾟｼｮﾅﾙﾌﾞﾛｯｸｽｷｯﾌﾟ
					m_bL0Cycle;		// 固定ｻｲｸﾙ中のL0動作
		};
		BOOL		m_ubFlgs[MC_FLG_NUMS];
	};
	// CString型ｵﾌﾟｼｮﾝ
	CString		m_strMCname,	// 機械名
				m_strAutoBreak,	// 自動ﾌﾞﾚｲｸ設定ｺｰﾄﾞ
				m_strMacroOpt[MCMACROSTRING];	// ﾏｸﾛ関係
	// 工具情報ｵﾌﾟｼｮﾝ
	CTypedPtrList<CPtrList, CMCTOOLINFO*>	m_ltTool;	// CMCTOOLINFO型を格納

	void	Convert(void);			// ﾚｼﾞｽﾄﾘからﾌｧｲﾙへ & 旧ﾊﾞｰｼﾞｮﾝのﾚｼﾞｽﾄﾘを消去
	void	ConvertWorkOffset(size_t, LPCTSTR);
	BOOL	AddMCListHistory(LPCTSTR);	// 履歴更新

public:
	CMCOption();
	~CMCOption();
	BOOL	ReadMCoption(LPCTSTR, BOOL = TRUE);
	BOOL	SaveMCoption(LPCTSTR);

	BOOL	GetFlag(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_ubFlgs) );
		return m_ubFlgs[n];
	}
	int		GetInt(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	float	GetDbl(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_udNums) );
		return m_udNums[n];
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
	float	GetInitialXYZ(size_t n) const {
		ASSERT( n>=0 && n<NCXYZ );
		return m_dInitialXYZ[n];
	}
	CPoint3F	GetWorkOffset(size_t n) const {
		ASSERT( n>=0 && n<WORKOFFSET );
		return	CPoint3F(m_dWorkOffset[n][NCA_X], m_dWorkOffset[n][NCA_Y], m_dWorkOffset[n][NCA_Z]);
	}
	CString	GetMacroStr(int n) const {
		ASSERT( n>=0 && n<SIZEOF(m_strMacroOpt) );
		return m_strMacroOpt[n];
	}
	CString	MakeMacroCommand(int) const;
	CString	GetDefaultOption(void) const;	// from MCSetup4.cpp
	CString	GetAutoBreakStr(void) const {
		return m_strAutoBreak;
	}
	boost::optional<float>	GetToolD(int) const;
	boost::optional<float>	GetToolH(int) const;
	int						GetMillType(int) const;
	BOOL	AddTool(int, float, BOOL);	// from NCDoc.cpp(G10)
	void	ReductionTools(BOOL);

	void	AddMCHistory_ComboBox(CComboBox&);
};
