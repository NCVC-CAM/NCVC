// NCViewYZ.h : ヘッダー ファイル
//

#pragma once

#include "NCViewBase.h"

class CNCViewYZ : public CNCViewBase
{
protected:
	CNCViewYZ();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCViewYZ)

	virtual	CRectD	ConvertRect(const CRect3D& rc) {
		CRectD	rcResult(rc.top, rc.low, rc.bottom, rc.high);
		return rcResult;
	}
	virtual	boost::tuple<size_t, size_t>	GetPlaneAxis(void) {
		size_t	x = NCA_Y, y = NCA_Z;
		return boost::make_tuple(x, y);
	}
	virtual	void	SetGuideData(void);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CNCViewYZ)
	public:
	virtual void OnInitialUpdate();
	protected:
	//}}AFX_VIRTUAL

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCViewYZ)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
