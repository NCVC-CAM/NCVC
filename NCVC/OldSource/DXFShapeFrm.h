// DXFShapeFrm.h : ヘッダー ファイル
//

#if !defined(AFX_DXFSHAPEFRM_H__8C692F0D_FA3F_4973_903E_75A9A7E0FE53__INCLUDED_)
#define AFX_DXFSHAPEFRM_H__8C692F0D_FA3F_4973_903E_75A9A7E0FE53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeFrm フレーム

class CDXFShapeFrm : public CFrameWnd
{
	CToolBar	m_wndToolBar;

protected:
	CDXFShapeFrm();           // 動的生成に使用されるプロテクト コンストラクタ。
	DECLARE_DYNCREATE(CDXFShapeFrm)

// アトリビュート
public:

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDXFShapeFrm)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	virtual ~CDXFShapeFrm();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CDXFShapeFrm)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DXFSHAPEFRM_H__8C692F0D_FA3F_4973_903E_75A9A7E0FE53__INCLUDED_)
