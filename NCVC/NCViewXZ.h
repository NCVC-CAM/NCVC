// NCViewXZ.h : ヘッダー ファイル
//

#pragma once

#include "NCViewBase.h"

class CNCViewXZ : public CNCViewBase
{
protected:
	CNCViewXZ();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCViewXZ)

	virtual	CRectF	ConvertRect(const CRect3F& rc) {
		CRectF	rcResult(rc.left, rc.low, rc.right, rc.high);
		return rcResult;
	}
	virtual	boost::tuple<size_t, size_t>	GetPlaneAxis(void) {
		size_t	x = NCA_X, y = NCA_Z;
		return boost::make_tuple(x, y);
	}
	virtual	void	SetGuideData(void);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CNCViewXZ)
	public:
	virtual void OnInitialUpdate();
	protected:
	//}}AFX_VIRTUAL

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCViewXZ)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
