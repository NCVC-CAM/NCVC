// DXFshape.h: CDXFmap クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

// CDXFworkingﾌﾗｸﾞ
#define	DXFWORKFLG_AUTO				0x0001
#define	DXFWORKFLG_SELECT			0x1000
// DXFshapeﾌﾗｸﾞ
#define	DXFMAPFLG_CANNOTWORKING		0x0001
#define	DXFMAPFLG_CANNOTOUTLINE		0x0002
#define	DXFMAPFLG_CANNOTAUTOWORKING	0x000f
#define	DXFMAPFLG_DIRECTION			0x0010
#define	DXFMAPFLG_START				0x0020
#define	DXFMAPFLG_OUTLINE			0x0040
#define	DXFMAPFLG_POCKET			0x0080
#define	DXFMAPFLG_WORKING			0x00f0
#define	DXFMAPFLG_INSIDE			0x0100
#define	DXFMAPFLG_OUTSIDE			0x0200
#define	DXFMAPFLG_SELECT			0x1000
#define	DXFMAPFLG_MAKE				0x2000
#define	DXFMAPFLG_SEARCH			0x4000
// CDXFworkingﾀｲﾌﾟ
enum	ENWORKINGTYPE {
	WORK_DIRECTION=0, WORK_START=1, WORK_OUTLINE=2, WORK_POCKET=3
};
// CDXFshape 所属集合
enum	DXFSHAPE_ASSEMBLE	{
	DXFSHAPE_OUTLINE=0, DXFSHAPE_LOCUS=1, DXFSHAPE_EXCLUDE=2
};
// 自動形状処理ﾀｲﾌﾟ
enum	ENAUTOWORKINGTYPE {
	AUTOOUTLINE=0,		AUTOPOCKET=1,
	AUTOALLINSIDE=2,	AUTOALLOUTSIDE=3,
	AUTORECALCWORKING=4
};

class CDXFdata;
class CDXFmap;
class CDXFchain;
class CDXFshape;

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
	DWORD	GetMapTypeFlag(void) const;
	boost::tuple<BOOL, CDXFarray*, CPointD>	IsEulerRequirement(const CPointD&) const;
	BOOL	IsAllSearchFlg(void) const;
	void	AllMapObject_ClearSearchFlg(BOOL = TRUE) const;
	void	AllMapObject_ClearMakeFlg() const;
	void	CopyToChain(CDXFchain*);
	void	Append(const CDXFmap*);
	void	Append(const CDXFchain*);
	//
	int		GetObjectCount(void) const;
	double	GetSelectObjectFromShape(const CPointD&, const CRectD* = NULL, CDXFdata** = NULL);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*) const;
	void	OrgTuning(void);

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
	DWORD		m_dwFlags;
	CRect3D		m_rcMax;

public:
	CDXFchain();
	virtual	~CDXFchain();

	void	SetChainFlag(DWORD dwFlags) {
		m_dwFlags |=  dwFlags;
	}
	BOOL	IsMakeFlg(void) const {
		return m_dwFlags & DXFMAPFLG_MAKE;
	}
	CRect3D	GetMaxRect(void) const {
		return m_rcMax;
	}
	void	SetMaxRect(const CDXFdata* pData) {
		m_rcMax |= pData->GetMaxRect();
	}
	void	ClearMaxRect(void) {
		m_rcMax.SetRectMinimum();
	}
	void	ReversPoint(void);
	void	CopyToMap(CDXFmap*);
	//
	int		GetObjectCount(void) const;
	double	GetSelectObjectFromShape(const CPointD&, const CRectD* = NULL, CDXFdata** = NULL);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*) const;
	void	OrgTuning(void);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFchain)
};

/////////////////////////////////////////////////////////////////////////////

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

	virtual	void	Serialize(CArchive&);
	DECLARE_DYNAMIC(CDXFworking)
};

typedef	CTypedPtrList<CObList, CDXFworking*>	CDXFworkingList;

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「方向」加工指示クラス
class CDXFworkingDirection : public CDXFworking
{
	CPointD		m_ptStart,		// 始点座標（円用）
				m_ptArraw[3];	// 矢印座標（終点）
	CPoint		m_ptDraw[3];	// 矢印描画座標

protected:
	CDXFworkingDirection() : CDXFworking(WORK_DIRECTION) {}
public:
	CDXFworkingDirection(CDXFshape*, CDXFdata*, CPointD, CPointD[]);

	CPointD	GetStartPoint(void) const {
		return m_ptStart;
	}
	CPointD	GetArrowPoint(void) const {
		return m_ptArraw[1];	// 矢印中心=>ｵﾌﾞｼﾞｪｸﾄ終点
	}
	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingDirection)
};

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「開始位置」指示クラス
class CDXFworkingStart : public CDXFworking
{
	CPointD		m_ptStart;		// 始点座標
	CRect		m_rcDraw;		// 描画矩形

protected:
	CDXFworkingStart() : CDXFworking(WORK_START) {}
public:
	CDXFworkingStart(CDXFshape*, CDXFdata*, CPointD);

	CPointD	GetStartPoint(void) const {
		return m_ptStart;
	}
	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingStart)
};

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「輪郭」加工指示クラス
class CDXFworkingOutline : public CDXFworking
{
	CDXFchain	m_ltOutline;	// 輪郭ｵﾌﾞｼﾞｪｸﾄ

protected:
	CDXFworkingOutline() : CDXFworking(WORK_OUTLINE) {}
public:
	CDXFworkingOutline(CDXFshape*, const CDXFchain*, DWORD = 0);
	virtual	~CDXFworkingOutline();

	CDXFchain*	GetOutlineChain(void) {
		return &m_ltOutline;
	}
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
	CDXFworkingPocket() : CDXFworking(WORK_POCKET) {}
public:
	CDXFworkingPocket(CDXFshape*, DWORD = 0);
	virtual	~CDXFworkingPocket();

	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingPocket)
};

/////////////////////////////////////////////////////////////////////////////

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
	double		m_dOffset;		// 形状ｵﾌｾｯﾄ値
	int			m_nInOut;		// 輪郭ｵﾌﾞｼﾞｪｸﾄの方向
	BOOL		m_bAcute;		// 鋭角丸め
	HTREEITEM	m_hTree;		// 登録されているﾂﾘｰﾋﾞｭｰﾊﾝﾄﾞﾙ(実行時動的設定)
	int			m_nSerialSeq;	// 現在のﾂﾘｰ順

	void	Constructor(DXFSHAPE_ASSEMBLE, LPCTSTR, DWORD);
	void	SetDetailInfo(CDXFchain*);
	void	SetDetailInfo(CDXFmap*);
	BOOL	ChangeCreate_MapToChain(CDXFmap*);
	BOOL	ChangeCreate_ChainToMap(CDXFchain*);
	CDXFdata*	CreateOutlineTempObject_new(const CDXFdata*, const CPointD&, const CPointD&, int) const;
	BOOL	SeparateOutlineIntersection(CDXFchain*, CTypedPtrArrayEx<CPtrArray, CDXFlist*>&, BOOL = FALSE);
	BOOL	CheckSeparateChain(CDXFlist*);
	BOOL	CheckIntersectionCircle(const CPointD&);
	void	RemoveExceptDirection(void);

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
	int		GetSerializeSeq(void) const {
		return m_nSerialSeq;
	}
	void	SetSerializeSeq(int nSeq) {
		m_nSerialSeq = nSeq;
	}
	double	GetOffset(void) const {
		return m_dOffset;
	}
	void	SetOffset(double dOffset) {
		m_dOffset = dOffset;
	}
	BOOL	GetAcuteRound(void) const {
		return m_bAcute;
	}
	void	SetAcuteRound(BOOL bAcute) {
		m_bAcute = bAcute;
	}
	int		GetInOutFlag(void) const {
		return m_nInOut;
	}
	CDXFworkingList*	GetWorkList(void) {
		return &m_ltWork;
	}
	BOOL	AddWorkingData(CDXFworking*, int = -1);
	BOOL	DelWorkingData(CDXFworking*, CDXFshape* = NULL);
	boost::tuple<CDXFworking*, CDXFdata*> GetDirectionObject(void) const;
	boost::tuple<CDXFworking*, CDXFdata*> GetStartObject(void) const;
	CDXFchain*	GetOutlineObject(void) const;
	BOOL	LinkObject(void);
	BOOL	LinkShape(CDXFshape*);
	//
	BOOL	CreateOutlineTempObject(BOOL, CDXFchain*);
	//
	void	AllChangeFactor(double) const;
	void	DrawWorking(CDC*) const;
	// CDXFchain* or CDXFmap* で振り分け処理
	int		GetObjectCount(void) const;
	double	GetSelectObjectFromShape(const CPointD&, const CRectD* = NULL, CDXFdata** = NULL);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*) const;
	void	OrgTuning(void);

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

struct OrgTuning_Visitor : boost::static_visitor<>
{
	template<typename T>
	void operator()(T p) const { p->OrgTuning(); }
};
