// NCMakeLathe.h: CNCMakeLathe クラスのインターフェイス
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeBase.h"

enum	TWOMOVEMODE {
	ZXMOVE, XZMOVE
};

class CNCMakeLathe : public CNCMakeBase
{
	// 座標値設定
//	static	CString	GetValString(int, double, BOOL);
	virtual	CString	GetValString(int, double, BOOL);

public:
	// 切削ﾃﾞｰﾀ
	CNCMakeLathe(const CDXFdata*);
	// 指定位置に直線移動
	CNCMakeLathe(const CPointD&);
	// 指定位置に２軸移動
	CNCMakeLathe(TWOMOVEMODE, const CPointD&);
	// X|Z軸の変化
	CNCMakeLathe(int, int, double);
	// 任意の文字列ｺｰﾄﾞ
	CNCMakeLathe(const CString&);

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化(TH_MakeLathe.cpp)
	static	void	SetStaticOption(const CNCMakeLatheOpt*);

	// TH_MakeLathe.cpp から参照
	static	CString	MakeSpindle(void);
};
