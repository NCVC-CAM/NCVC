// DXFshape.h: CDXFmap �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

// CDXFworking�׸�
#define	DXFWORKFLG_AUTO				0x0001
#define	DXFWORKFLG_SCAN				0x0002
#define	DXFWORKFLG_SELECT		0x00010000
// DXFshape�׸�
#define	DXFMAPFLG_CANNOTWORKING		0x0001
#define	DXFMAPFLG_EDGE				0x0002
#define	DXFMAPFLG_INTERSEC			0x0004
#define	DXFMAPFLG_CANNOTAUTOWORKING	0x000f
#define	DXFMAPFLG_DIRECTION			0x0010
#define	DXFMAPFLG_START				0x0020
#define	DXFMAPFLG_OUTLINE			0x0040
#define	DXFMAPFLG_POCKET			0x0080
#define	DXFMAPFLG_WORKING			0x00f0
#define	DXFMAPFLG_INSIDE			0x0100
#define	DXFMAPFLG_OUTSIDE			0x0200
#define	DXFMAPFLG_SEPARATE			0x0800
#define	DXFMAPFLG_REVERSE			0x1000
#define	DXFMAPFLG_NEXTPOS			0x2000
#define	DXFMAPFLG_SELECT		0x00010000
#define	DXFMAPFLG_MAKE			0x00020000
#define	DXFMAPFLG_SEARCH		0x00040000
// CDXFworking����
enum	ENWORKINGTYPE {
	WORK_DIRECTION=0, WORK_START, WORK_OUTLINE, WORK_POCKET
};
// CDXFshape �����W��
enum	DXFSHAPE_ASSEMBLE {
	DXFSHAPE_OUTLINE=0, DXFSHAPE_LOCUS, DXFSHAPE_EXCLUDE
};
// CDXFshape �W������
enum {
	DXFSHAPETYPE_CHAIN=0, DXFSHAPETYPE_MAP
};

class CLayerData;
class CDXFdata;
class CDXFmap;
class CDXFchain;
class CDXFshape;

CDXFdata*	CreateDxfOffsetObject	//	TH_AutoWorkingSet.cpp ������Q��
	(const CDXFdata*, const CPointF&, const CPointF&, int = 0, float = 0);
CDXFdata*	CreateDxfLatheObject
	(const CDXFdata*, float);

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̍��W�}�b�v�{�A���W�c�N���X
/////////////////////////////////////////////////////////////////////////////
typedef	CMap<CPointF, CPointF&, CDXFarray*, CDXFarray*&>	CMapPointToDXFarray;
class CDXFmap : public CMapPointToDXFarray
{
	BOOL	m_bNativePointKey;
public:
	CDXFmap();
	virtual	~CDXFmap();
#ifdef _DEBUG
	virtual	void	DbgDump(void) const;
#endif
	static	float	ms_dTolerance;	// ������W�ƌ��Ȃ����e��

	void	SetPointMap(CDXFdata*);			// CMap�ɍ��W�ް��o�^
	void	SetMakePointMap(CDXFdata*);		// �@�V�@(Make�p)
	DWORD	GetMapTypeFlag(void) const;
	boost::tuple<BOOL, CDXFarray*, CPointF>	IsEulerRequirement(const CPointF&) const;
	BOOL	IsNativeKey(void) const {
		return m_bNativePointKey;
	}
	BOOL	IsAllSearchFlg(void) const;
	BOOL	IsAllMakeFlg(void) const;
	void	AllMapObject_ClearSearchFlg(BOOL = TRUE) const;
	void	AllMapObject_ClearMakeFlg(void) const;
	BOOL	CopyToChain(CDXFchain*);
	void	Append(const CDXFmap*);
	void	Append(const CDXFchain*);
	//
	INT_PTR	GetObjectCount(void) const;
	float	GetSelectObjectFromShape(const CPointF&, const CRectF* = NULL, CDXFdata** = NULL);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*) const;
	void	OrgTuning(void);

	// ���ްײ��
	void	RemoveAll();	// DestructElements()���g���Ȃ��Ȃ����̂�
							// RemoveAll()���ްײ�ނ������ذ���h��

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFmap)
};
namespace boost { namespace range_detail_microsoft {
/*
    template< >
    struct customization< ::CDXFmap > :
    #if !defined(BOOST_RANGE_MFC_NO_CPAIR)
        mfc_cpair_map_functions
    #else
        mfc_map_functions
    #endif
    {
        template< class X >
        struct meta
        {
    #if !defined(BOOST_RANGE_MFC_NO_CPAIR)
            typedef typename X::CPair pair_t;

            typedef mfc_cpair_map_iterator<X, pair_t> mutable_iterator;
            typedef mfc_cpair_map_iterator<X const, pair_t const> const_iterator;
    #else
            typedef CPointF key_t;
            typedef CDXFarray* mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
    #endif            
        };
    };
*/
} }
/*
BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CDXFmap
)
*/
/////////////////////////////////////////////////////////////////////////////
// CDXFmap(CMap)���߰Ă����۰������߰�֐�
template<> AFX_INLINE
UINT AFXAPI HashKey(CPointF& ptKey)
{
	// �O�����̒藝���L�[��(�Q�悾���ł͐��l���傫���Ȃ肷��)
	// ���x����������̂ŁC�P�̈ʂ��ۂ�
//	return (UINT)(ptKey.hypot() / 10.0) * 10;
	// ���Ԃ�������̂ŊȒP��
//	return (UINT)GAPCALC(ptKey) >> 4;	// x^2 + y^2
	// �����ɈႤ���W�ŃL�[�l���ς��Ȃ��悤��
	return (UINT)GAPCALC(ptKey.RoundUp()) >> 4;	// x^2 + y^2
}

template<> AFX_INLINE
BOOL AFXAPI CompareElements(const CPointF* lpt1, const CPointF* lpt2)
{
	return lpt1->hypot(lpt2) < CDXFmap::ms_dTolerance;
}

#ifdef _DEBUG
template<> AFX_INLINE
void AFXAPI DumpElements(CDumpContext& dc, const CPointF* lpt, INT_PTR nCount)
{
	dc << "key x=" << lpt->x << " y=" << lpt->y;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̗֊s�W�c�N���X
/////////////////////////////////////////////////////////////////////////////
class CDXFchain : public CDXFlist
{
	DWORD		m_dwFlags;
	CRect3F		m_rcMax;

	POSITION	(CDXFchain::*m_pfnGetFirstPos)(void) const;
	CDXFdata*&	(CDXFchain::*m_pfnGetFirstData)();
	CDXFdata*&	(CDXFchain::*m_pfnGetData)(POSITION&);

public:
	CDXFchain(DWORD = 0);
	virtual	~CDXFchain();

	void	SetChainFlag(DWORD dwFlags) {
		m_dwFlags |=  dwFlags;
	}
	DWORD	GetChainFlag(void) const {
		return m_dwFlags;
	}
	void	ClearSideFlg(void) {
		m_dwFlags &= ~(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE);
	}
	BOOL	IsMakeFlg(void) const {
		return m_dwFlags & DXFMAPFLG_MAKE;
	}
	BOOL	IsSearchFlg(void) const {
		return m_dwFlags & DXFMAPFLG_SEARCH;
	}
	void	SetMakeFlags(void);
	CRect3F	GetMaxRect(void) const {
		return m_rcMax;
	}
	void	SetMaxRect(const CRect3F& rc) {
		m_rcMax |= rc;
	}
	void	SetMaxRect(const CDXFdata* pData) {
		SetMaxRect(pData->GetMaxRect());
	}
	void	ClearMaxRect(void) {
		m_rcMax.SetRectMinimum();
	}
	void	ReverseMakePt(void);
	void	ReverseNativePt(void);
	void	CopyToMap(CDXFmap*);
	BOOL	IsLoop(void) const;
	BOOL	IsPointInPolygon(const CPointF&) const;
	//
	POSITION	SetLoopFunc(const CDXFdata*, BOOL, BOOL);
	POSITION	GetFirstPosition(void) {
		return (this->*m_pfnGetFirstPos)();
	}
	CDXFdata*&	GetFirstData(void) {
		return (this->*m_pfnGetFirstData)();
	}
	CDXFdata*&	GetSeqData(POSITION& pos) {
		return (this->*m_pfnGetData)(pos);
	}
	//
	INT_PTR	GetObjectCount(void) const;
	float	GetLength(void) const;
	void	AllChainObject_ClearSearchFlg(void);
	float	GetSelectObjectFromShape(const CPointF&, const CRectF* = NULL, CDXFdata** = NULL);
	void	SetVectorPoint(POSITION, CVPointF&, float);
	void	SetVectorPoint(POSITION, CVPointF&, size_t);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*) const;
	void	OrgTuning(void);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFchain)
};
namespace boost { namespace range_detail_microsoft {

	template< >
	struct customization< ::CDXFchain > :
		list_functions
	{
		template< class X >
		struct meta
		{
            typedef list_iterator<X, ::CDXFdata *> mutable_iterator;
    #if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
            typedef list_iterator<X const, ::CDXFdata const *> const_iterator;
    #else
            typedef list_iterator<X const, ::CDXFdata const * const, ::CDXFdata const * const> const_iterator;
    #endif
		};
	};

/*
    template< >
    struct customization< ::CTypedPtrList<CObList, CDXFdata*> > :
        list_functions
    {
        template< class X >
        struct meta
        {
            typedef typename remove_pointer<CDXFdata*>::type val_t;

            // not l-value
            typedef list_iterator<X, val_t * const, val_t * const> mutable_iterator;
            typedef list_iterator<X const, val_t const * const, val_t const * const> const_iterator;
        };
    };
*/
} }

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
	boost::range_detail_microsoft::using_type_as_tag,
	BOOST_PP_NIL, CDXFchain
)

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^���H�w���̃x�[�X�N���X
/////////////////////////////////////////////////////////////////////////////
class CDXFworking : public CObject
{
	ENWORKINGTYPE	m_enType;	// �w������

protected:
	DWORD		m_dwFlags;		// ����׸�
	CString		m_strWorking;	// ���H�w����
	CDXFshape*	m_pShape;		// ������`��׽
	CDXFdata*	m_pData;		// �I���޼ު��

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
	BOOL	IsScanWorking(void) const {
		return m_dwFlags & DXFWORKFLG_SCAN;		// CDXFworkingOutline �̂�
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

	virtual	void	DrawTuning(float) = 0;
	virtual	void	Draw(CDC*) const = 0;

	virtual	void	Serialize(CArchive&);
	DECLARE_DYNAMIC(CDXFworking)
};
typedef	CTypedPtrList<CObList, CDXFworking*>	CDXFworkingList;

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�����v���H�w���N���X
class CDXFworkingDirection : public CDXFworking
{
	CPointF		m_ptStart,		// �n�_���W�i�~�p�j
				m_ptArraw[3];	// �����W�i�I�_�j
	CPoint		m_ptDraw[3];	// ���`����W

protected:
	CDXFworkingDirection() : CDXFworking(WORK_DIRECTION) {}
public:
	CDXFworkingDirection(CDXFshape*, CDXFdata*, CPointF, CPointF[]);

	CPointF	GetStartPoint(void) const {
		return m_ptStart;
	}
	CPointF	GetArrowPoint(void) const {
		return m_ptArraw[1];	// ��󒆐S=>��޼ު�ďI�_
	}
	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingDirection)
};

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�J�n�ʒu�v�w���N���X
class CDXFworkingStart : public CDXFworking
{
	CPointF		m_ptStart;		// �n�_���W
	CRect		m_rcDraw;		// �`���`

protected:
	CDXFworkingStart() : CDXFworking(WORK_START) {}
public:
	CDXFworkingStart(CDXFshape*, CDXFdata*, CPointF);

	CPointF	GetStartPoint(void) const {
		return m_ptStart;
	}
	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingStart)
};

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�֊s�v���H�w���N���X
typedef CSortArray<CObArray, CDXFchain*>	COutlineData;
class CDXFworkingOutline : public CDXFworking
{
	COutlineData	m_obOutline;
	CStringArray	m_obMergeHandle;	// �����������ꂽ�W��
	float			m_dOffset;			// �֊s��޼ު�Ă����ꂽ�Ƃ��̵̾�Ēl
	CRect3F			m_rcMax;
	void	SeparateAdd_Construct(const CDXFchain*);

protected:
	CDXFworkingOutline() : CDXFworking(WORK_OUTLINE) {
		m_rcMax.SetRectMinimum();
	}
public:
	CDXFworkingOutline(CDXFshape*, const CDXFchain*, float, DWORD = 0);
	virtual	~CDXFworkingOutline();

	float		GetOutlineOffset(void) const {
		return m_dOffset;
	}
	INT_PTR		GetOutlineSize(void) const {
		return m_obOutline.GetSize();
	}
	CDXFchain*	GetOutlineObject(INT_PTR n) const {
		return m_obOutline[n];
	}
	CRect3F		GetMaxRect(void) const {
		return m_rcMax;
	}
	void	SeparateModify(void);	// from TH_AutoWorkingSet.cpp::SeparateOutline_Thread()
	void	SetMergeHandle(const CString&);
	INT_PTR	GetMergeHandleSize(void) const {
		return m_obMergeHandle.GetSize();
	}
	CString	GetMergeHandle(INT_PTR n) const {
		return m_obMergeHandle[n];
	}

	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingOutline)
};
typedef	CTypedPtrList<CObList, CDXFworkingOutline*>	COutlineList;

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�|�P�b�g�v���H�w���N���X
typedef CSortArray<CObArray, CDXFworkingOutline*>	COutlineWorkArray;
class CDXFworkingPocket : public CDXFworking
{
	COutlineWorkArray	m_obPocket;		// �����̗֊s��޼ު�Ă��Ǘ�

protected:
	CDXFworkingPocket() : CDXFworking(WORK_POCKET) {}
public:
	CDXFworkingPocket(CDXFshape*, DWORD = 0);
	virtual	~CDXFworkingPocket();

	virtual	void	DrawTuning(float);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingPocket)
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̌`��W���N���X
//		���W�}�b�v�Ɨ֊s�W�c�̐e���N���X
/////////////////////////////////////////////////////////////////////////////
class CDXFshape : public CObject
{
	DWORD		m_dwFlags;
	DXFSHAPE_ASSEMBLE	m_enAssemble;	// �����W��
	CLayerData*	m_pParentLayer;			// �ް��̑�����ڲԏ��
	CRect3F		m_rcMax;				// ��޼ު�čő��`
	CString		m_strShape,				// �`��(�ύX��)
				m_strShapeHandle;		// �`��W���׽���ʗp(�ύX�s��)
	boost::variant<CDXFchain*, CDXFmap*>	m_vShape;	// CDXFchain* or CDXFmap*
	CDXFworkingList	m_ltWork;			// ���H�w���ް�(Outline����)
	COutlineList	m_ltOutline;		// �֊s�ް�
	COutlineData	m_obLathe;			// ���՗p�֊s�ް�
	float		m_dOffset;				// �`�����̫�ĵ̾�Ēl
	int			m_nInOut;				// �֊s��޼ު�Ă̕���
	BOOL		m_bAcute;				// �s�p�ۂ�
	HTREEITEM	m_hTree;				// �o�^����Ă����ذ�ޭ������(���s�����I�ݒ�)
	int			m_nSerialSeq;			// ���݂��ذ��

	void	Constructor(DXFSHAPE_ASSEMBLE, LPCTSTR, DWORD, CLayerData*);
	void	SetDetailInfo(CDXFchain*);
	void	SetDetailInfo(CDXFmap*);
	BOOL	CreateOutlineTempObject_polyline(const CDXFpolyline*, BOOL, float,
				CDXFchain*, CTypedPtrArray<CPtrArray, CDXFlist*>&);
	BOOL	SeparateOutlineIntersection(CDXFchain*, CTypedPtrArray<CPtrArray, CDXFlist*>&, BOOL = FALSE);
	BOOL	CheckSeparateChain(CDXFlist*, const float);

protected:
	CDXFshape();	// Serialize
public:
	CDXFshape(DXFSHAPE_ASSEMBLE, LPCTSTR, DWORD, CLayerData*, CDXFchain*);
	CDXFshape(DXFSHAPE_ASSEMBLE, LPCTSTR, DWORD, CLayerData*, CDXFmap*);
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
	CLayerData*	GetParentLayer(void) const {
		return m_pParentLayer;
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
	BOOL	IsSideFlg(void) const {
		return m_dwFlags & (DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE);
	}
	CString	GetShapeHandle(void) const {
		return m_strShapeHandle;
	}
	CString	GetShapeName(void) const {
		return m_strShape;
	}
	void	SetShapeName(LPCTSTR lpszShape) {	// from CDXFShapeView::OnEndLabelEdit()
		m_strShape = lpszShape;
	}
	CRect3F	GetMaxRect(void) const {
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
	float	GetOffset(void) const {
		return m_dOffset;
	}
	void	SetOffset(float dOffset) {
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
	COutlineList*		GetOutlineList(void) {
		return &m_ltOutline;
	}
	COutlineData*		GetLatheList(void) {
		return &m_obLathe;
	}
	BOOL	IsOutlineList(void) const {
		return !m_ltOutline.IsEmpty();
	}
	CDXFworkingOutline*	GetOutlineLastObj(void) {
		return m_ltOutline.IsEmpty() ? NULL : m_ltOutline.GetTail();
	}
	BOOL	AddWorkingData(CDXFworking*);
	BOOL	DelWorkingData(CDXFworking*, CDXFshape* = NULL);
	BOOL	AddOutlineData(CDXFworkingOutline*, int);
	BOOL	DelOutlineData(CDXFworkingOutline* = NULL);
	boost::tuple<CDXFworking*, CDXFdata*>  GetDirectionObject(void) const;
	boost::tuple<CDXFworking*, CDXFdata*>  GetStartObject(void) const;
	POSITION	GetFirstChainPosition(void);		// TH_MakeXXX.cpp
	BOOL	LinkObject(void);
	BOOL	LinkShape(CDXFshape*);
	BOOL	ChangeCreate_MapToChain(CDXFmap* = NULL);
	BOOL	ChangeCreate_ChainToMap(CDXFchain* = NULL);
	//
	BOOL	CreateOutlineTempObject(BOOL, CDXFchain*, float = 0.0f);
	BOOL	CheckIntersectionCircle(const CPointF&, const float dOffset);
	BOOL	CreateScanLine_X(CDXFchain*);
	BOOL	CreateScanLine_Y(CDXFchain*);
	BOOL	CreateScanLine_Outline(CDXFchain*);
	BOOL	CreateScanLine_ScrollCircle(CDXFchain*);
	void	CreateScanLine_Lathe(int, float);
	void	CrearScanLine_Lathe(void);
	//
	void	AllChangeFactor(float) const;
	void	DrawWorking(CDC*) const;
	// CDXFchain* or CDXFmap* �ŐU�蕪������
	INT_PTR	GetObjectCount(void) const;
	float	GetSelectObjectFromShape(const CPointF&, const CRectF* = NULL, CDXFdata** = NULL);
	void	SetShapeSwitch(BOOL);
	void	RemoveObject(const CDXFdata*);
	void	DrawShape(CDC*) const;
	void	OrgTuning(void);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFshape)
};

typedef CSortArray<CObArray, CDXFshape*>		CShapeArray;

/////////////////////////////////////////////////////////////////////////////
// Visitor�W��(����������� apply_visitor �ł͔����)

struct GetObjectCount_Visitor : boost::static_visitor<INT_PTR>
{
	template<typename T>
	INT_PTR operator()(T p) const { return p->GetObjectCount(); }
};

struct OrgTuning_Visitor : boost::static_visitor<>
{
	template<typename T>
	void operator()(T p) const { p->OrgTuning(); }
};
