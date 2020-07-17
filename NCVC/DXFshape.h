// DXFshape.h: CDXFmap �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

// CDXFworking�׸�
#define	DXFWORKFLG_AUTO				0x0001
#define	DXFWORKFLG_SELECT			0x1000
// DXFshape�׸�
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
// CDXFworking����
enum	ENWORKINGTYPE {
	WORK_DIRECTION=0, WORK_START=1, WORK_OUTLINE=2, WORK_POCKET=3
};
// CDXFshape �����W��
enum	DXFSHAPE_ASSEMBLE	{
	DXFSHAPE_OUTLINE=0, DXFSHAPE_LOCUS=1, DXFSHAPE_EXCLUDE=2
};
// �����`�󏈗�����
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
// �c�w�e�f�[�^�̍��W�}�b�v�{�A���W�c�N���X
/////////////////////////////////////////////////////////////////////////////
typedef	CMap<CPointD, CPointD&, CDXFarray*, CDXFarray*&>	CMapPointToDXFarray;
class CDXFmap : public CMapPointToDXFarray
{
	CDXFdata*	GetFirstObject(void) const;

public:
	CDXFmap();
	virtual	~CDXFmap();

	static	double	ms_dTolerance;	// ������W�ƌ��Ȃ����e��

	void	SetPointMap(CDXFdata*);			// CMap�ɍ��W�ް��o�^
	void	SetMakePointMap(CDXFdata*);		// �@�V�@(Make�p)
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
	virtual	void	DrawTuning(double);
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
	virtual	void	DrawTuning(double);
	virtual	void	Draw(CDC*) const;

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CDXFworkingStart)
};

/////////////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�́u�֊s�v���H�w���N���X
class CDXFworkingOutline : public CDXFworking
{
	CDXFchain	m_ltOutline;	// �֊s��޼ު��

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
// �c�w�e�f�[�^�́u�|�P�b�g�v���H�w���N���X
class CDXFworkingPocket : public CDXFworking
{
	CDXFarray	m_obPocket;		// �߹�ĵ�޼ު��

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
// �c�w�e�f�[�^�̌`��W���N���X
//		���W�}�b�v�Ɨ֊s�W�c�̐e���N���X
/////////////////////////////////////////////////////////////////////////////
class CDXFshape : public CObject
{
	DWORD		m_dwFlags;
	DXFSHAPE_ASSEMBLE	m_enAssemble;	// �����W��
	CRect3D		m_rcMax;		// ��޼ު�čő��`
	CString		m_strShape;		// �`��
	boost::variant<CDXFchain*, CDXFmap*>	m_vShape;	// CDXFchain* or CDXFmap*
	CDXFworkingList	m_ltWork;	// ���H�w���ް�
	double		m_dOffset;		// �`��̾�Ēl
	int			m_nInOut;		// �֊s��޼ު�Ă̕���
	BOOL		m_bAcute;		// �s�p�ۂ�
	HTREEITEM	m_hTree;		// �o�^����Ă����ذ�ޭ������(���s�����I�ݒ�)
	int			m_nSerialSeq;	// ���݂��ذ��

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
