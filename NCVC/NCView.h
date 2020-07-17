// NCView.h : CNCView クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "NCViewBase.h"

class CNCView : public CNCViewBase
{
	// ﾃﾞｰﾀ矩形
	CPointD		m_ptdMaxRect[2][4];		// 最大切削矩形[Zmin/Zmax][矩形]
	CPoint		m_ptDrawMaxRect[2][4];	// 最大切削矩形の描画用座標
	// ﾜｰｸ矩形
	CPointD		m_ptdWorkRect[2][4];	// ﾜｰｸ矩形[Zmin/Zmax][矩形] ４角(from CNCDoc::m_rcWork)
	CPoint		m_ptDrawWorkRect[2][4];	// ﾜｰｸ矩形の描画用座標
	// ﾜｰｸ円柱
	CPointD		m_ptdWorkCylinder[2][ARCCOUNT];
	CPoint		m_ptDrawWorkCylinder[2][ARCCOUNT];

protected: // シリアライズ機能のみから作成します。
	CNCView();
	DECLARE_DYNCREATE(CNCView)

	virtual	void	SetGuideData(void);
	virtual	void	SetDataMaxRect(void);
	virtual	void	SetWorkRect(void);
	virtual	void	SetWorkCylinder(void);
	virtual	void	ConvertMaxRect(void);
	virtual	void	ConvertWorkRect(void);
	virtual	void	ConvertWorkCylinder(void);
	virtual	void	DrawMaxRect(CDC*);
	virtual	void	DrawWorkRect(CDC*);
	virtual	void	DrawWorkCylinder(CDC*);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CNCView)
	public:
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);  // このビューを描画する際にオーバーライドされます。
	protected:
	//}}AFX_VIRTUAL

// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCView)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
