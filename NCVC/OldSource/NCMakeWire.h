// NCMakeWire.h: CNCMakeWire クラスのインターフェイス
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"

class CNCMakeWire : public CNCMakeMill
{
public:
	// 切削ﾃﾞｰﾀ
	CNCMakeWire(const CDXFdata*, double);
	// XYのG[0|1]移動
	CNCMakeWire(int, const CPointD& pt, double, double);
	// 任意の文字列ｺｰﾄﾞ
	CNCMakeWire(const CString&);

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化(TH_MakeWire.cpp)
	static	void	SetStaticOption(const CNCMakeWireOpt*);	// TH_MakeWire.cpp
};
