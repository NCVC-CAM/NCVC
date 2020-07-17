// CustomControl.h : ヘッダー ファイル
//

#pragma once

/*
	ｲﾝﾌﾟﾚｽ発行
	VisualC++5 ﾊﾟﾜﾌﾙﾃｸﾆｯｸ大全集
	Scott Stanfield/Ralph Arvesen 著
	羽山 博 監訳
*/

/////////////////////////////////////////////////////////////////////////////
// CIntEdit ウィンドウ

class CIntEdit : public CEdit
{
// コンストラクション
public:
	CIntEdit();

// アトリビュート
public:

// オペレーション
public:
	operator int();
	CIntEdit& operator =(int);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CIntEdit)
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CIntEdit();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CIntEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CFloatEdit ウィンドウ

class CFloatEdit : public CEdit
{
	BOOL	m_bIntFormat;	// 小数点以下がないとき，整数ﾌｫｰﾏｯﾄ

// コンストラクション
public:
	CFloatEdit(BOOL = FALSE);

// アトリビュート
public:

// オペレーション
public:
	operator float();
	CFloatEdit& operator =(float);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CFloatEdit)
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CFloatEdit();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CFloatEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CColComboBox ウィンドウ

class CColComboBox : public CComboBox
{
// コンストラクション
public:
	CColComboBox();

// アトリビュート
public:

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CColComboBox)
	public:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CColComboBox();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CColComboBox)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

COLORREF	ConvertSTRtoRGB(LPCTSTR);	// 文字列を色情報に変換
CString		ConvertRGBtoSTR(COLORREF);	// その逆
