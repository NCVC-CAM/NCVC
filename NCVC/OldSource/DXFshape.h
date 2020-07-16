// DXFshape.h: CDXFmap クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(__DXFSHAPE_H__)
#define __DXFSHAPE_H__

// CDXFworkingﾀｲﾌﾟ
enum	ENWORKINGTYPE {
	DIRECTION=0, OUTLINE=1, POCKET=2
};
// CDXFworkingﾌﾗｸﾞ
#define	DXFWORKFLG_AUTO				0x0001
#define	DXFWORKFLG_SELECT			0x1000
// DXFshapeﾌﾗｸﾞ
#define	DXFMAPFLG_CANNOTWORKING		0x0001
#define	DXFMAPFLG_CANNOTOUTLINE		0x0002
#define	DXFMAPFLG_CANNOTAUTOWORKING	0x000f
#define	DXFMAPFLG_DIRECTION			0x0010
#define	DXFMAPFLG_OUTLINE			0x0020
#define	DXFMAPFLG_POCKET			0x0040
#define	DXFMAPFLG_WORKING			0x00f0
#define	DXFMAPFLG_INSIDE			0x0100
#define	DXFMAPFLG_OUTSIDE			0x0200
#define	DXFMAPFLG_SELECT			0x1000
#define	DXFMAPFLG_MAKE				0x2000
#define	DXFMAPFLG_SEARCH			0x4000
// CDXFshape 所属集合
enum	DXFSHAPE_ASSEMBLE	{
	DXFSHAPE_OUTLINE=0, DXFSHAPE_LOCUS=1, DXFSHAPE_EXCLUDE=2
};

class CDXFdata;
class CDXFmap;
class CDXFchain;
class CDXFshape;

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータ加工指示のベースクラス
/////////////////////////////////////////////////////////////////////////////
class CDXFworking : public CObject
{
	ENWORKINGTYPE	m_enType;	// 指示ﾀｲﾌﾟ

protected:
	DWORD		m_dwFlags;		// 状態ﾌﾗｸﾞ
	CString		m_strWorking;	// 加工指示名
	CDXFshape*	m_pShape;		// 属する形状ｸﾗｽ
	CDXFdata*	m_pData;		// 選択ｵﾌﾞｼﾞｪｸﾄ

	CDXFworking(ENWORKINGTYPE, CDXFshape* = NULL, CDXFdata* = NULL, DWORD = 0);

public:
	ENWORKINGTYPE	GetWorkingType(void) const {
		return m_enType;
	}
	DWORD	GetWorkingFlag(void) const {
		return m_dwFlags;
	}
	void	SetSelectFlag(BOOL bSelect) {
		if ( bSelect )
			m_dwFlags |=  DXFWORKFLG_SELECT;
		else
			m_dwFlags &= ~DXFWORKFLG_SELECT;
	}
	BOOL	IsAutoWorking(void) const {
		return m_dwFlags & DXFWORKFLG_AUTO;
	}
	CString	GetWorkingName(void) const {
		return m_strWorking;
	}
	void	SetWorkingName(LPCTSTR lpszWorking) {	// from CDXFShapeView::OnEndLabelEdit()
		m_strWorking = lpszWorking;
	}
	CDXFshape*	GetParentMap(void) const {
		return m_pShape;
	}
	CDXFdata*	GetTargetObject(void) const {
		return m_pData;
	}

	virtual	void	DrawTuning(double) = 0;
	virtual	void	Draw(CDC*) const = 0;

protected:
	virtual	void	Serialize(CArchive&);	// 派生ｸﾗｽ用
	DECLARE_DYNAMIC(CDXFworking)
};

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「方向」加工指示クラス
class CDXFworkingDirection : public CDXFworking
{
	CPointD		m_ptArraw[3];	// 矢印座標
	CPoint		m_ptDraw[3];	// 矢印描画座標

protected:
	CDXFworkingDirection() : CDXFworking(DIRECTION) {}
public:
	CDXFworkingDirection(CDXFshape*, CDXFdata*, CPointD[]);

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingDirection)
};

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「輪郭」加工指示クラス
class CDXFworkingOutline : public CDXFworking
{
	CDXFarray	m_obOutline;	// 輪郭ｵﾌﾞｼﾞｪｸﾄ

protected:
	CDXFworkingOutline() : CDXFworking(OUTLINE) {}
public:
	CDXFworkingOutline(CDXFshape*, const CDXFarray&, DWORD = 0);
	virtual	~CDXFworkingOutline();

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingOutline)
};

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「ポケット」加工指示クラス
class CDXFworkingPocket : public CDXFworking
{
	CDXFarray	m_obPocket;		// ﾎﾟｹｯﾄｵﾌﾞｼﾞｪｸﾄ

protected:
	CDXFworkingPocket() : CDXFworking(POCKET) {}
public:
	CDXFworkingPocket(CDXFshape*, DWORD = 0);
	virtual	~CDXFworkingPocket();

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingPocket)
};

typedef	CTypedPtrList<CObList, CDXFworking*>	CDXFworkingList;

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの座標マップ＋連結集団クラス
/////////////////////////////////////////////////////////////////////////////
typedef	CMap<CPointD, CPointD&, CDXFarray*, CDXFarray*&>	CMapPointToDXFarray;
class CDXFmap : public CMapPointToDXFarray
{
	CDXFdata*	GetFirstObject(void) const;

public:
	CDXFmap();
	virtual	~CDXFmap();

	static	double	ms_dTolerance;	// 同一座標と見なす許容差

	void	SetPointMap(CDXFdata*);			// CMapに座標ﾃﾞｰﾀ登録
	void	SetMakePointMap(CDXFdata*);		// 　〃　(Make用)
	DWORD	GetShapeFlag(void);
	boost::tuple<BOOL, CDXFarray*, CPointD>	IsEulerRequirement(const CPointD&) const;
	BOOL	IsAllSearchFlg(void) const;
	void	AllMapObject_ClearSearchFlg(BOOL = TRUE) const;
	void	AllMapObject_ClearMakeFlg() const;
	void	Copy_ToChain(CDXFchain*);
	void	Append(const CDXFmap*);
	void	Append(const CDXFchain*);
	//
	int		GetObjectCount(void);
	boost::tuple<CDXFdata*, double> GetSelectViewGap(const CPointD&, const CRectD&);
	boost::tuple<CDXFdata*, double>	GetSelectMakeGap(const CPointD&, BOOL = TRUE);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*);

	// ｵｰﾊﾞｰﾗｲﾄﾞ
	void	RemoveAll();	// DestructElements()が使えなくなったので
							// RemoveAll()をｵｰﾊﾞｰﾗｲﾄﾞしてﾒﾓﾘﾘｰｸを防ぐ

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFmap)
};

/////////////////////////////////////////////////////////////////////////////
// CDXFmap(CMap)をｻﾎﾟｰﾄするｸﾞﾛｰﾊﾞﾙﾍﾙﾊﾟｰ関数
template<> AFX_INLINE
UINT AFXAPI HashKey(CPointD& ptKey)
{
	// 三平方の定理をキーに(２乗だけでは数値が大きくなりすぎ)
	// 精度が高すぎるので，１の位をｾﾞﾛに
//	return (UINT)(ptKey.hypot() / 10.0) * 10;
	// 時間がかかるので簡単に
//	return (UINT)GAPCALC(ptKey) >> 4;	// x^2 + y^2
	// 微妙に違う座標でキー値が変わらないように
	return (UINT)GAPCALC(ptKey.RoundUp()) >> 4;	// x^2 + y^2
}

template<> AFX_INLINE
BOOL AFXAPI CompareElements(const CPointD* lpt1, const CPointD* lpt2)
{
	return ( lpt1->IsMatchPoint(lpt2) ||
				_hypot(lpt1->x - lpt2->x, lpt1->y - lpt2->y) < CDXFmap::ms_dTolerance );
}

#ifdef _DEBUG
template<> AFX_INLINE
void AFXAPI DumpElements(CDumpContext& dc, const CPointD* lpt, INT_PTR nCount)
{
	dc << "key x=" << lpt->x << " y=" << lpt->y;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの輪郭集団クラス
/////////////////////////////////////////////////////////////////////////////
class CDXFchain : public CDXFlist
{
public:
	CDXFchain();
	virtual	~CDXFchain();

	DWORD	GetShapeFlag(void);
	void	Copy_ToMap(CDXFmap*);
	//
	int		GetObjectCount(void);
	boost::tuple<CDXFdata*, double> GetSelectViewGap(const CPointD&, const CRectD&);
	boost::tuple<CDXFdata*, double>	GetSelectMakeGap(const CPointD&, BOOL = TRUE);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFchain)
};

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの形状集合クラス
//		座標マップと輪郭集団の親分クラス
/////////////////////////////////////////////////////////////////////////////
class CDXFshape : public CObject
{
	DWORD		m_dwFlags;
	DXFSHAPE_ASSEMBLE	m_enAssemble;	// 所属集合
	CRect3D		m_rcMax;		// ｵﾌﾞｼﾞｪｸﾄ最大矩形
	CString		m_strShape;		// 形状名
	boost::variant<CDXFchain*, CDXFmap*>	m_vShape;	// CDXFchain* or CDXFmap*
	CDXFworkingList	m_ltWork;	// 加工指示ﾃﾞｰﾀ
	HTREEITEM	m_hTree;		// 登録されているﾂﾘｰﾋﾞｭｰﾊﾝﾄﾞﾙ

	void	Constructor(DXFSHAPE_ASSEMBLE, LPCTSTR, DWORD);
	void	SetDetailInfo(CDXFchain*);
	void	SetDetailInfo(CDXFmap*);
	BOOL	ChangeCreate_MapToChain(CDXFmap*);
	BOOL	ChangeCreate_ChainToMap(CDXFchain*);

protected:
	CDXFshape();	// Serialize
public:
	CDXFshape(DXFSHAPE_ASSEMBLE, LPCTSTR lpszShape, DWORD, CDXFchain*);
	CDXFshape(DXFSHAPE_ASSEMBLE, LPCTSTR lpszShape, DWORD, CDXFmap*);
	virtual	~CDXFshape();

	DXFSHAPE_ASSEMBLE	GetShapeAssemble(void) const {
		return m_enAssemble;
	}
	void	SetShapeAssemble(DXFSHAPE_ASSEMBLE enAssemble) {
		m_enAssemble = enAssemble;
	}
	int		GetShapeType(void) const {
		return m_vShape.which();
	}
	CDXFchain*	GetShapeChain(void) const {
		return m_vShape.which()==0 ? boost::get<CDXFchain*>(m_vShape) : NULL;
	}
	CDXFmap*	GetShapeMap(void) const {
		return m_vShape.which()==1 ? boost::get<CDXFmap*>(m_vShape) : NULL;
	}
	DWORD	GetShapeFlag(void) const {
		return m_dwFlags;
	}
	void	SetShapeFlag(DWORD dwFlags) {
		m_dwFlags |=  dwFlags;
	}
	void	ClearSideFlg(void) {
		m_dwFlags &= ~(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE);
	}
	void	ClearMakeFlg(void) {
		m_dwFlags &= ~(DXFMAPFLG_MAKE|DXFMAPFLG_SEARCH);
	}
	BOOL	IsMakeFlg(void) const {
		return m_dwFlags & DXFMAPFLG_MAKE;
	}
	BOOL	IsSearchFlg(void) const {
		return m_dwFlags & DXFMAPFLG_SEARCH;
	}
	CString	GetShapeName(void) const {
		return m_strShape;
	}
	void	SetShapeName(LPCTSTR lpszShape) {	// from CDXFShapeView::OnEndLabelEdit()
		m_strShape = lpszShape;
	}
	CRect3D	GetMaxRect(void) const {
		return m_rcMax;
	}
	HTREEITEM	GetTreeHandle(void) const {
		return m_hTree;
	}
	void	SetTreeHandle(HTREEITEM hTree) {
		m_hTree = hTree;
	}
	CDXFworkingList*	GetWorkList(void) {
		return &m_ltWork;
	}
	BOOL	AddWorkingData(CDXFworking*);
	BOOL	DelWorkingData(CDXFworking*, CDXFshape* = NULL);
	boost::tuple<CDXFworking*, CDXFdata*> GetDirectionObject(void) const;
	void	RemoveExceptDirection(void);
	BOOL	LinkObject(void);
	BOOL	LinkShape(CDXFshape*);
	//
	void	AllChangeFactor(double);
	void	DrawWorking(CDC*);
	// CDXFchain* or CDXFmap* で振り分け処理
	int		GetObjectCount(void) const;
	boost::tuple<CDXFdata*, double> GetSelectViewGap(const CPointD&, const CRectD&);
	boost::tuple<CDXFdata*, double>	GetSelectMakeGap(const CPointD&, BOOL = TRUE);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*);

protected:
	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFshape)
};

typedef CSortArray<CObArray, CDXFshape*>		CShapeArray;

/////////////////////////////////////////////////////////////////////////////
// Visitor集合(引数があると apply_visitor では非効率)

struct GetObjectCount_Visitor : boost::static_visitor<int>
{
	template<typename T>
	int operator()(T p) const { return p->GetObjectCount(); }
};

#endif
