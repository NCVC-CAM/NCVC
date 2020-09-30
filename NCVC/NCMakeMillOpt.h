// NCMakeMillOpt.h: NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "NCMakeOption.h"

enum {
	MKNC_NUM_PROG = 0,			// ﾌﾟﾛｸﾞﾗﾑ番号
	MKNC_NUM_LINEADD,			// 行番号増加
	MKNC_NUM_G90,				// 位置指定(G90 or G91)
	MKNC_NUM_DOT,				// 数値表記(小数3位 or 小数4位 or 1/1000)
	MKNC_NUM_FDOT,				// Ｆﾊﾟﾗﾒｰﾀの数値表記
	MKNC_NUM_CIRCLECODE,		// 円切削(G2 or G3)
	MKNC_NUM_SPINDLE,			// 主軸回転速度
	MKNC_NUM_ZRETURN,			// Z軸の復帰(Initial or R)
	MKNC_NUM_IJ,				// 円弧補間にRかI/J/K
	MKNC_NUM_MAKEEND,			// 加工済み深さの指示
	MKNC_NUM_DEEPSPINDLE,		// 深彫仕上げ回転数
	MKNC_NUM_DEEPRETURN,		// 深彫仕上げからのZ軸復帰
	MKNC_NUM_DEEPALL,			// 深彫の切削手順
	MKNC_NUM_DEEPROUND,			// 深彫の切削方向
	MKNC_NUM_DRILLSPINDLE,		// 穴あけ回転数
	MKNC_NUM_DWELLFORMAT,		// ﾄﾞｳｪﾙ時間の表記
	MKNC_NUM_DRILLRETURN,		// 穴あけの加工ｺｰﾄﾞ
	MKNC_NUM_DRILLPROCESS,		// 穴あけの仕方
	MKNC_NUM_DRILLSORT,			// 穴あけのｸﾞﾙｰﾋﾟﾝｸﾞ順序
	MKNC_NUM_DRILLCIRCLEPROCESS,// 円ﾃﾞｰﾀを含むときの穴あけ順序
	MKNC_NUM_MOVEZ,				// 移動ﾚｲﾔのZ軸
	MKNC_NUM_TOLERANCE,			// 許容差を超えたときの動作
	MKNC_NUM_OPTIMAIZEDRILL,	// 穴加工の基準軸
		MKNC_NUM_NUMS		// [23]
};
enum {
	MKNC_DBL_FEED = 0,			// 切削送り速度
	MKNC_DBL_ZFEED,				// Z軸を下げるときの送り速度
	MKNC_DBL_ZG0STOP,			// G0で動かすZ位置(R点)
	MKNC_DBL_ZCUT,				// Z軸の下げ位置(切り込み)
	MKNC_DBL_G92X,				// G92のX/Y/Z
	MKNC_DBL_G92Y,
	MKNC_DBL_G92Z,
	MKNC_DBL_ELLIPSE,			// 楕円公差
	MKNC_DBL_MAKEEND,			// 加工済み深さのｵﾌｾｯﾄor固定Z値
	MKNC_DBL_MAKEENDFEED,		// 加工済みZ送り速度
	MKNC_DBL_DEEP,				// 深彫の最終切り込み
	MKNC_DBL_ZSTEP,				// 深彫切削のためのｽﾃｯﾌﾟ
	MKNC_DBL_DEEPFEED,			// 深彫仕上げ送り
	MKNC_DBL_DRILLFEED,			// 穴あけ送り
	MKNC_DBL_DRILLR,			// 穴加工R点
	MKNC_DBL_DRILLZ,			// 穴加工切り込み
	MKNC_DBL_DRILLQ,			// 深穴Q値
	MKNC_DBL_DWELL,				// ﾄﾞｳｪﾙ時間
	MKNC_DBL_DRILLCIRCLE,		// 穴加工に見立てる円ﾃﾞｰﾀの半径
	MKNC_DBL_TOLERANCE,			// 同一座標と見なす許容差
	MKNC_DBL_DRILLMARGIN,		// 基準軸に対する許容差
		MKNC_DBL_NUMS		// [21]
};
enum {
	MKNC_FLG_PROG = 0,			// O番号付与
	MKNC_FLG_PROGAUTO,			// ﾗﾝﾀﾞﾑ割り当て
	MKNC_FLG_LINEADD,			// 行番号
	MKNC_FLG_ZEROCUT,			// 小数点以下のｾﾞﾛｶｯﾄ
	MKNC_FLG_GCLIP,				// Gｺｰﾄﾞ省略形
	MKNC_FLG_ELLIPSE,			// 長径と短径が等しい楕円は円とみなす
	MKNC_FLG_XREV,				// X軸反転
	MKNC_FLG_YREV,				// Y軸反転
	MKNC_FLG_DISABLESPINDLE,	// Sﾊﾟﾗﾒｰﾀを生成しない
	MKNC_FLG_CIRCLEHALF,		// 全円は分割
	MKNC_FLG_ZEROCUT_IJ,		// [I|J]0は省略
	MKNC_FLG_DEEP,				// 深彫切削を行う
	MKNC_FLG_HELICAL,			// 円ﾃﾞｰﾀをﾍﾘｶﾙ切削
	MKNC_FLG_DEEPFINISH,		// 仕上げｵﾌﾟｼｮﾝ適用か
	MKNC_FLG_DRILLMATCH,		// 穴加工同一座標は無視
	MKNC_FLG_DRILLCIRCLE,		// 円ﾃﾞｰﾀも穴加工ﾃﾞｰﾀと見なす
	MKNC_FLG_DRILLBREAK,		// 大きさごとにｺﾒﾝﾄを埋め込む
	MKNC_FLG_LAYERCOMMENT,		// ﾚｲﾔごとにｺﾒﾝﾄを埋め込む
	MKNC_FLG_L0CYCLE,			// 固定ｻｲｸﾙ中はL0出力
		MKNC_FLG_NUMS		// [19]
};
enum {
	MKNC_STR_LINEFORM = 0,		// 行番号ﾌｫｰﾏｯﾄ
	MKNC_STR_EOB,				// EOB
	MKNC_STR_HEADER,			// ｶｽﾀﾑﾍｯﾀﾞｰ
	MKNC_STR_FOOTER,			// ｶｽﾀﾑﾌｯﾀｰ
	MKNC_STR_CUSTMOVE_B,		// ｶｽﾀﾑ移動ｺｰﾄﾞ(前後)
	MKNC_STR_CUSTMOVE_A,
	MKNC_STR_PERLSCRIPT,		// 生成後に実行されるPerlｽｸﾘﾌﾟﾄ
		MKNC_STR_NUMS		// [7]
};
//
#define	MIL_I_PROG					m_pIntOpt[MKNC_NUM_PROG]
#define	MIL_I_LINEADD				m_pIntOpt[MKNC_NUM_LINEADD]
#define	MIL_I_G90					m_pIntOpt[MKNC_NUM_G90]
#define	MIL_I_DOT					m_pIntOpt[MKNC_NUM_DOT]
#define	MIL_I_FDOT					m_pIntOpt[MKNC_NUM_FDOT]
#define	MIL_I_CIRCLECODE			m_pIntOpt[MKNC_NUM_CIRCLECODE]
#define	MIL_I_SPINDLE				m_pIntOpt[MKNC_NUM_SPINDLE]
#define	MIL_I_ZRETURN				m_pIntOpt[MKNC_NUM_ZRETURN]
#define	MIL_I_IJ					m_pIntOpt[MKNC_NUM_IJ]
#define	MIL_I_MAKEEND				m_pIntOpt[MKNC_NUM_MAKEEND]
#define	MIL_I_DEEPSPINDLE			m_pIntOpt[MKNC_NUM_DEEPSPINDLE]
#define	MIL_I_DEEPRETURN			m_pIntOpt[MKNC_NUM_DEEPRETURN]
#define	MIL_I_DEEPALL				m_pIntOpt[MKNC_NUM_DEEPALL]
#define	MIL_I_DEEPROUND				m_pIntOpt[MKNC_NUM_DEEPROUND]
#define	MIL_I_DRILLSPINDLE			m_pIntOpt[MKNC_NUM_DRILLSPINDLE]
#define	MIL_I_DWELLFORMAT			m_pIntOpt[MKNC_NUM_DWELLFORMAT]
#define	MIL_I_DRILLRETURN			m_pIntOpt[MKNC_NUM_DRILLRETURN]
#define	MIL_I_DRILLPROCESS			m_pIntOpt[MKNC_NUM_DRILLPROCESS]
#define	MIL_I_DRILLSORT				m_pIntOpt[MKNC_NUM_DRILLSORT]
#define	MIL_I_DRILLCIRCLEPROCESS	m_pIntOpt[MKNC_NUM_DRILLCIRCLEPROCESS]
#define	MIL_I_MOVEZ					m_pIntOpt[MKNC_NUM_MOVEZ]
#define	MIL_I_TOLERANCE				m_pIntOpt[MKNC_NUM_TOLERANCE]
#define	MIL_I_OPTIMAIZEDRILL		m_pIntOpt[MKNC_NUM_OPTIMAIZEDRILL]
//
#define	MIL_D_FEED					m_pDblOpt[MKNC_DBL_FEED]
#define	MIL_D_ZFEED					m_pDblOpt[MKNC_DBL_ZFEED]
#define	MIL_D_ZG0STOP				m_pDblOpt[MKNC_DBL_ZG0STOP]
#define	MIL_D_ZCUT					m_pDblOpt[MKNC_DBL_ZCUT]
#define	MIL_D_G92X					m_pDblOpt[MKNC_DBL_G92X]
#define	MIL_D_G92Y					m_pDblOpt[MKNC_DBL_G92Y]
#define	MIL_D_G92Z					m_pDblOpt[MKNC_DBL_G92Z]
#define	MIL_D_ELLIPSE				m_pDblOpt[MKNC_DBL_ELLIPSE]
#define	MIL_D_MAKEEND				m_pDblOpt[MKNC_DBL_MAKEEND]
#define	MIL_D_MAKEENDFEED			m_pDblOpt[MKNC_DBL_MAKEENDFEED]
#define	MIL_D_DEEP					m_pDblOpt[MKNC_DBL_DEEP]
#define	MIL_D_ZSTEP					m_pDblOpt[MKNC_DBL_ZSTEP]
#define	MIL_D_DEEPFEED				m_pDblOpt[MKNC_DBL_DEEPFEED]
#define	MIL_D_DRILLFEED				m_pDblOpt[MKNC_DBL_DRILLFEED]
#define	MIL_D_DRILLR				m_pDblOpt[MKNC_DBL_DRILLR]
#define	MIL_D_DRILLZ				m_pDblOpt[MKNC_DBL_DRILLZ]
#define	MIL_D_DRILLQ				m_pDblOpt[MKNC_DBL_DRILLQ]
#define	MIL_D_DWELL					m_pDblOpt[MKNC_DBL_DWELL]
#define	MIL_D_DRILLCIRCLE			m_pDblOpt[MKNC_DBL_DRILLCIRCLE]
#define	MIL_D_TOLERANCE				m_pDblOpt[MKNC_DBL_TOLERANCE]
#define	MIL_D_DRILLMARGIN			m_pDblOpt[MKNC_DBL_DRILLMARGIN]
//
#define	MIL_F_PROG					m_pFlgOpt[MKNC_FLG_PROG]
#define	MIL_F_PROGAUTO				m_pFlgOpt[MKNC_FLG_PROGAUTO]
#define	MIL_F_LINEADD				m_pFlgOpt[MKNC_FLG_LINEADD]
#define	MIL_F_ZEROCUT				m_pFlgOpt[MKNC_FLG_ZEROCUT]
#define	MIL_F_GCLIP					m_pFlgOpt[MKNC_FLG_GCLIP]
#define	MIL_F_ELLIPSE				m_pFlgOpt[MKNC_FLG_ELLIPSE]
#define	MIL_F_XREV					m_pFlgOpt[MKNC_FLG_XREV]
#define	MIL_F_YREV					m_pFlgOpt[MKNC_FLG_YREV]
#define	MIL_F_DISABLESPINDLE		m_pFlgOpt[MKNC_FLG_DISABLESPINDLE]
#define	MIL_F_CIRCLEHALF			m_pFlgOpt[MKNC_FLG_CIRCLEHALF]
#define	MIL_F_ZEROCUT_IJ			m_pFlgOpt[MKNC_FLG_ZEROCUT_IJ]
#define	MIL_F_DEEP					m_pFlgOpt[MKNC_FLG_DEEP]
#define	MIL_F_HELICAL				m_pFlgOpt[MKNC_FLG_HELICAL]
#define	MIL_F_DEEPFINISH			m_pFlgOpt[MKNC_FLG_DEEPFINISH]
#define	MIL_F_DRILLMATCH			m_pFlgOpt[MKNC_FLG_DRILLMATCH]
#define	MIL_F_DRILLCIRCLE			m_pFlgOpt[MKNC_FLG_DRILLCIRCLE]
#define	MIL_F_DRILLBREAK			m_pFlgOpt[MKNC_FLG_DRILLBREAK]
#define	MIL_F_LAYERCOMMENT			m_pFlgOpt[MKNC_FLG_LAYERCOMMENT]
#define	MIL_F_L0CYCLE				m_pFlgOpt[MKNC_FLG_L0CYCLE]
//
#define	MIL_S_LINEFORM				m_strOption[MKNC_STR_LINEFORM]
#define	MIL_S_EOB					m_strOption[MKNC_STR_EOB]
#define	MIL_S_HEADER				m_strOption[MKNC_STR_HEADER]
#define	MIL_S_FOOTER				m_strOption[MKNC_STR_FOOTER]
#define	MIL_S_CUSTMOVE_B			m_strOption[MKNC_STR_CUSTMOVE_B]
#define	MIL_S_CUSTMOVE_A			m_strOption[MKNC_STR_CUSTMOVE_A]
#define	MIL_S_PERLSCRIPT			m_strOption[MKNC_STR_PERLSCRIPT]
//
class CNCMakeMillOpt : public CNCMakeOption
{
	friend class CMKNCSetup1;
	friend class CMKNCSetup2;
	friend class CMKNCSetup3;
	friend class CMKNCSetup4;
	friend class CMKNCSetup5;
	friend class CMKNCSetup6;
	friend class CMKNCSetup8;

	// 親ｸﾗｽにﾃﾞｰﾀを持たせるので、
	// union/struct技は使えない
	// むしろC++らしくｺｰﾃﾞｨﾝｸﾞ

	BOOL	Convert();						// 旧ﾊﾞｰｼﾞｮﾝ互換用

protected:
	virtual	void	InitialDefault(void);
	virtual	BOOL	IsPathID(int);

public:
	CNCMakeMillOpt(LPCTSTR);

	virtual	CString	GetLineNoForm(void) const;

#ifdef _DEBUG
	virtual	void	DbgDump(void) const;	// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
