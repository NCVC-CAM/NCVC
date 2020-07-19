// NCMakeLathe.h: CNCMakeLathe クラスのインターフェイス
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeBase.h"
#include "NCMakeLatheOpt.h"

enum	TWOMOVEMODE {
	ZXMOVE, XZMOVE
};

class CNCMakeLathe : public CNCMakeBase
{
	// 座標値設定(PFNGETVALSTRING型に合わせるための第3引数)
	static	CString	GetValString(int, float, BOOL = FALSE);

public:
	// 空ｵﾌﾞｼﾞｪｸﾄ
	CNCMakeLathe(void);
	// 切削ﾃﾞｰﾀ
	CNCMakeLathe(const CDXFdata*, float);
	// 指定位置に直線移動
	CNCMakeLathe(const CPointF&);
	// 指定位置に２軸移動
	CNCMakeLathe(TWOMOVEMODE, const CPointF&, float);
	// X|Z軸の変化
	CNCMakeLathe(int, int, float, float);
	// 任意の文字列ｺｰﾄﾞ
	CNCMakeLathe(const CString&);

	// 端面加工生成
	void	CreateEndFace(const CPointF&);
	// 下穴加工生成
	void	CreatePilotHole(void);

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化(TH_MakeLathe.cpp)
	static	void	SetStaticOption(const CNCMakeLatheOpt*);

	// TH_MakeLathe.cpp から参照
	static	CString	MakeSpindle(int);
};
