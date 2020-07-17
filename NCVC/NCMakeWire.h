// NCMakeWire.h: CNCMakeWire クラスのインターフェイス
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"

class CNCMakeWire : public CNCMakeMill
{
public:
	// 切削ﾃﾞｰﾀ
	CNCMakeWire(const CDXFdata*, float);
	// 上下異形状の切削ﾃﾞｰﾀ
	CNCMakeWire(const CDXFdata*, const CDXFdata*, float);
	// 上下異形状を微細線分で生成
	CNCMakeWire(const CVPointF&, const CVPointF&, float);
	// XYのG[0|1]移動
	CNCMakeWire(int, const CPointF&, float, float);
	// XY/UVのG01移動
	CNCMakeWire(const CPointF&, const CPointF&, float);
	// 任意の文字列ｺｰﾄﾞ
	CNCMakeWire(const CString&);

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化(TH_MakeWire.cpp)
	static	void	SetStaticOption(const CNCMakeWireOpt*);	// TH_MakeWire.cpp
};
