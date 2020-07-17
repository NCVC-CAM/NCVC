// NCViewXY.h : ヘッダー ファイル
//

#pragma once

#include "NCViewBase.h"

class CNCViewXY : public CNCViewBase
{
protected:
	CNCViewXY();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCViewXY)

	virtual	void	SetGuideData(void);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CNCViewXY)
	public:
	virtual void OnInitialUpdate();
	protected:
	//}}AFX_VIRTUAL

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCViewXY)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
