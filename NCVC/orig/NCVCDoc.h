// NCVCDoc.h : CNCVCDoc クラスのインターフェイス
//


#pragma once


class CNCVCDoc : public CDocument
{
protected: // シリアル化からのみ作成します。
	CNCVCDoc();
	DECLARE_DYNCREATE(CNCVCDoc)

// 属性
public:

// 操作
public:

// オーバーライド
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// 実装
public:
	virtual ~CNCVCDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成された、メッセージ割り当て関数
protected:
	DECLARE_MESSAGE_MAP()
};


