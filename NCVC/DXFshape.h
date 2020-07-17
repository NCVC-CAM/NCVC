// DXFshape.h: CDXFmap �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

// CDXFworking�׸�
#define	DXFWORKFLG_AUTO				0x0001
#define	DXFWORKFLG_SCAN				0x0002
#define	DXFWORKFLG_SELECT			0x1000
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
#define	DXFMAPFLG_SELECT			0x1000
#define	DXFMAPFLG_MAKE				0x2000
#define	DXFMAPFLG_SEARCH			0x4000
// CDXFworking����
enum	ENWORKINGTYPE {
	WORK_DIRECTION=0, WORK_START=1, WORK_OUTLINE=2, WORK_POCKET=3
};
// CDXFshape �����W��
enum	DXFSHAPE_ASSEMBLE {
	DXFSHAPE_OUTLINE=0, DXFSHAPE_LOCUS=1, DXFSHAPE_EXCLUDE=2
};
// CDXFshape �W������
#define	DXFSHAPETYPE_CHAIN			0
#define	DXFSHAPETYPE_MAP			1

class CDXFdata;
class CDXFmap;
class CDXFchain;
class CDXFshape;

CDXFdata*	CreateDxfOffsetObject	//	TH_AutoWorkingSet.cpp ������Q��
	(const CDXFdata*, const CPointD&, const CPointD&, int = 0, double = 0);
CDXFdata*	CreateDxfLatheObject
	(const CDXFdata*, double);

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̍��W�}�b�v�{�A���W�c�N���X
/////////////////////////////////////////////////////////////////////////////
typedef	CMap<CPointD, CPointD&, CDXFarray*, CDXFarray*&>	CMapPointToDXFarray;
class CDXFmap : public CMapPointToDXFarray
{
public:
	CDXFmap();
	virtual	~CDXFmap();

	static	double	ms_dTolerance;	// ������W�ƌ��Ȃ����e��

	void	SetPointMap(CDXFdata*);			// CMap�ɍ��W�ް��o�^
	void	SetMakePointMap(CDXFdata*);		// �@�V�@(Make�p)
	DWORD	GetMapTypeFlag(void) const;
	boost::tuple<BOOL, CDXFarray*, CPointD>	IsEulerRequirement(const CPointD&) const;
	BOOL	IsAllSearchFlg(void) const;
	BOOL	IsAllMakeFlg(void) const;
	void	AllMapObject_ClearSearchFlg(BOOL = TRUE) const;
	void	AllMapObject_ClearMakeFlg(void) const;
	BOOL	CopyToChain(CDXFchain*);
	void	Append(const CDXFmap*);
	void	Append(const CDXFchain*);
	//
	int		GetObjectCount(void) const;
	double	GetSelectObjectFromShape(const CPointD&, const CRectD* = NULL, CDXFdata** = NULL);
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

/////////////////////////////////////////////////////////////////////////////
// CDXFmap(CMap)���߰Ă����۰������߰�֐�
template<> AFX_INLINE
UINT AFXAPI HashKey(CPointD& ptKey)
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
// �c�w�e�f�[�^�̗֊s�W�c�N���X
/////////////////////////////////////////////////////////////////////////////
class CDXFchain : public CDXFlist
{
	DWORD		m_dwFlags;
	CRect3D		m_rcMax;

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
	CRect3D	GetMaxRect(void) const {
		return m_rcMax;
	}
	void	SetMaxRect(const CRect3D& rc) {
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
	BOOL	IsPointInPolygon(const CPointD&) const;
	//
	int		GetObjectCount(void) const;
	void	AllChainObject_ClearSearchFlg(void);
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

	virtual	void	DrawTuning(const double) = 0;
	virtual	void	Draw(CDC*) const = 0;

	virtual	void	Serialize(CArchive&);
	DECLARE_DYNAMIC(CDXFworking)
};

typedef	CTypedPtrList<CObList, CDXFworking*>	CDXFworkingList;

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�����v���H�w���N���X
class CDXFworkingDirection : public CDXFworking
{
	CPointD		m_ptStart,		// �n�_���W�i�~�p�j
				m_ptArraw[3];	// �����W�i�I�_�j
	CPoint		m_ptDraw[3];	// ���`����W

protected:
	CDXFworkingDirection() : CDXFworking(WORK_DIRECTION) {}
public:
	CDXFworkingDirection(CDXFshape*, CDXFdata*, CPointD, CPointD[]);

	CPointD	GetStartPoint(void) const {
		return m_ptStart;
	}
	CPointD	GetArrowPoint(void) const {
		return m_ptArraw[1];	// ��󒆐S=>��޼ު�ďI�_
	}
	virtual	void	DrawTuning(const double);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingDirection)
};

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�J�n�ʒu�v�w���N���X
class CDXFworkingStart : public CDXFworking
{
	CPointD		m_ptStart;		// �n�_���W
	CRect		m_rcDraw;		// �`���`

protected:
	CDXFworkingStart() : CDXFworking(WORK_START) {}
public:
	CDXFworkingStart(CDXFshape*, CDXFdata*, CPointD);

	CPointD	GetStartPoint(void) const {
		return m_ptStart;
	}
	virtual	void	DrawTuning(const double);
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
	double			m_dOffset;			// �֊s��޼ު�Ă����ꂽ�Ƃ��̵̾�Ēl
	CRect3D			m_rcMax;
	void	SeparateAdd_Construct(const CDXFchain*);

protected:
	CDXFworkingOutline() : CDXFworking(WORK_OUTLINE) {
		m_rcMax.SetRectMinimum();
	}
public:
	CDXFworkingOutline(CDXFshape*, const CDXFchain*, const double, DWORD = 0);
	virtual	~CDXFworkingOutline();

	double		GetOutlineOffset(void) const {
		return m_dOffset;
	}
	int			GetOutlineSize(void) const {
		return m_obOutline.GetSize();
	}
	CDXFchain*	GetOutlineObject(int n) const {
		return m_obOutline[n];
	}
	CRect3D		GetMaxRect(void) const {
		return m_rcMax;
	}
	void	SeparateModify(void);	// from TH_AutoWorkingSet.cpp::SeparateOutline_Thread()
	void	SetMergeHandle(const CString&);
	int		GetMergeHandleSize(void) const {
		return m_obMergeHandle.GetSize();
	}
	CString	GetMergeHandle(int n) const {
		return m_obMergeHandle[n];
	}

	virtual	void	DrawTuning(const double);
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

	virtual	void	DrawTuning(const double);
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
	CRect3D		m_rcMax;				// ��޼ު�čő��`
	CString		m_strShape,				// �`��(�ύX��)
				m_strShapeHandle;		// �`��W���׽���ʗp(�ύX�s��)
	boost::variant<CDXFchain*, CDXFmap*>	m_vShape;	// CDXFchain* or CDXFmap*
	CDXFworkingList	m_ltWork;			// ���H�w���ް�(Outline����)
	COutlineList	m_ltOutline;		// �֊s�ް�
	COutlineData	m_obLathe;			// ���՗p�֊s�ް�
	double		m_dOffset;				// �`�����̫�ĵ̾�Ēl
	int			m_nInOut;				// �֊s��޼ު�Ă̕���
	BOOL		m_bAcute;				// �s�p�ۂ�
	HTREEITEM	m_hTree;				// �o�^����Ă����ذ�ޭ������(���s�����I�ݒ�)
	int			m_nSerialSeq;			// ���݂��ذ��

	void	Constructor(DXFSHAPE_ASSEMBLE, LPCTSTR, DWORD);
	void	SetDetailInfo(CDXFchain*);
	void	SetDetailInfo(CDXFmap*);
	BOOL	CreateOutlineTempObject_polyline(const CDXFpolyline*, BOOL, double,
				CDXFchain*, CTypedPtrArrayEx<CPtrArray, CDXFlist*>&);
	BOOL	SeparateOutlineIntersection(CDXFchain*, CTypedPtrArrayEx<CPtrArray, CDXFlist*>&, BOOL = FALSE);
	BOOL	CheckSeparateChain(CDXFlist*, const double);

protected:
	CDXFshape();	// Serialize
public:
	CDXFshape(DXFSHAPE_ASSEMBLE, LPCTSTR, DWORD, CDXFchain*);
	CDXFshape(DXFSHAPE_ASSEMBLE, LPCTSTR, DWORD, CDXFmap*);
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
	BOOL	LinkObject(void);
	BOOL	LinkShape(CDXFshape*);
	BOOL	ChangeCreate_MapToChain(CDXFmap* = NULL);
	BOOL	ChangeCreate_ChainToMap(CDXFchain* = NULL);
	//
	BOOL	CreateOutlineTempObject(BOOL, CDXFchain*, double = 0.0);
	BOOL	CheckIntersectionCircle(const CPointD&, const double dOffset);
	BOOL	CreateScanLine_X(CDXFchain*);
	BOOL	CreateScanLine_Y(CDXFchain*);
	BOOL	CreateScanLine_Outline(CDXFchain*);
	BOOL	CreateScanLine_ScrollCircle(CDXFchain*);
	void	CreateScanLine_Lathe(int, double);
	void	CrearScanLine_Lathe(void);
	//
	void	AllChangeFactor(double) const;
	void	DrawWorking(CDC*) const;
	// CDXFchain* or CDXFmap* �ŐU�蕪������
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
// Visitor�W��(����������� apply_visitor �ł͔����)

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
