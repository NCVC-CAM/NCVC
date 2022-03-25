// TH_MakeNurbs.cpp
//		NURBS�ȖʗpNC����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"		// NCMakeBase.h�p
#include "3dModelDoc.h"
#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::string;
using namespace boost;

enum MAKEMODE
{
	ROUGH, CONTOUR
};

// ��۰��ٕϐ���`
static	CThreadDlg*		g_pParent;
static	C3dModelDoc*	g_pDoc;
static	CNCMakeMillOpt*	g_pMakeOpt;
static	MAKEMODE		g_enMode;
static	int				g_nMode;
static	float			g_dZoffset;		// ���[�N�̍��������_�ɂ���Ƃ��̃I�t�Z�b�g
static	int				g_nFase;

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)
#define	Get3dDbl(a)	g_pDoc->Get3dOption()->Get3dDbl(a)
#define	Get3dFlg(a)	g_pDoc->Get3dOption()->Get3dFlg(a)

// NC�����ɕK�v���ް��Q
static	CTypedPtrArrayEx<CPtrArray, CNCMakeMill*>	g_obMakeData;	// ���H�ް�

// �T�u�֐��֐�
static	void	InitialVariable(void);			// �ϐ�������
static	void	SetStaticOption(void);			// �ÓI�ϐ��̏�����
static	BOOL	MakeNurbs_RoughFunc(void);		// �r���H�̐������[�v
static	BOOL	MakeNurbs_ContourFunc(void);	// �d�グ�������̐������[�v
static	BOOL	OutputNurbsCode(void);			// NC���ނ̏o��
// �r���H�p�T�u
static	tuple<ptrdiff_t, int>		SearchFirstPoint(const VVCoord&);
static	tuple<ptrdiff_t, int>		SearchNearEdge(const VVCoord&);
// �d�グ�������p�T�u
static	tuple<ptrdiff_t, ptrdiff_t>	SearchNearGroup(const VVCoord&);
static	tuple<ptrdiff_t, double>	SearchNearPoint(const VCoord&);
static	void	MakeLoopCoord(VCoord&, size_t, int=-1);

// ͯ�ް,̯�ް���̽�߼�ٺ��ސ���
static	void	AddCustomNurbsCode(int);

// �C���ް��̐���
static inline	void _AddMakeNurbsStr(const CString& strData)
{
	CNCMakeMill*	pNCD = new CNCMakeMill(strData);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// G00��Z���ړ�
static inline	void _AddMoveG00Z(double z)
{
	CNCMakeMill*	pNCD = new CNCMakeMill(0, (float)z, 0.0f);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// G01��Z���ړ�
static inline	void _AddMakeG01Zcut(double z)
{
	z -= g_dZoffset;
	CNCMakeMill* pNCD = new CNCMakeMill(1, (float)z, GetDbl(MKNC_DBL_ZFEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// pt�܂ňړ����Cpt.z+R�_�܂�Z�����~
static inline	void _AddMovePoint(const CPoint3D& pt)
{
	CNCMakeMill*	pNCD;
	// �؍�|�C���g�܂ňړ�
	pNCD = new CNCMakeMill(0, pt.GetXY(), 0.0f);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	// pt.z+R�_�܂�G00Z���ړ�
	_AddMoveG00Z(pt.z - g_dZoffset + GetDbl(MKNC_DBL_ZG0STOP));
}
// Coord���W�̐���
static inline	void _AddMakeCoord(const Coord& c)
{
	CPoint3D		pt(c);
	pt.z -= g_dZoffset;
	CNCMakeMill* pNCD = new CNCMakeMill(pt, GetDbl(MKNC_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

// ��޽گ�ފ֐�
static	CCriticalSection	g_csMakeAfter;	// MakeNurbs_AfterThread()�گ��ۯ���޼ު��
static	UINT	MakeNurbs_AfterThread(LPVOID);	// ��n���گ��

//////////////////////////////////////////////////////////////////////
// NURBS�ȖʗpNC�����گ��
//////////////////////////////////////////////////////////////////////

UINT MakeNurbs_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeNurbs_Thread() Start\n");
#endif
	int		nResult = IDCANCEL;

	// ��۰��ٕϐ�������
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc = static_cast<C3dModelDoc *>(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);

	// �������\��
	g_nFase = 0;
	SendFaseMessage(g_pParent, g_nFase, -1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;
	// �������[�h�̌���
	if ( !g_pDoc->GetRoughCoord().empty() ) {
		g_enMode = ROUGH;
	}
	else if ( !g_pDoc->GetContourCoord().empty() ) {
		g_enMode = CONTOUR;
	}

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		// NC������߼�ݵ�޼ު�Ă̐����Ƶ�߼�݂̓ǂݍ���
		g_pMakeOpt = new CNCMakeMillOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKENURBS)->GetHead());
		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		InitialVariable();
		// �������Ƃɕω��������Ұ���ݒ�
		SetStaticOption();
		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
		// �������蓖��
		g_obMakeData.SetSize(0, 2048);
		// �����J�n
		BOOL bResult = FALSE;
		switch ( g_enMode ) {
		case ROUGH:
			// �r���H�������[�v
			bResult = MakeNurbs_RoughFunc();
			break;
		case CONTOUR:
			// �d�グ�������[�v
			bResult = MakeNurbs_ContourFunc();
			break;
		}
		if ( bResult )
			bResult = OutputNurbsCode();

		// �߂�l���
		if ( bResult && IsThread() )
			nResult = IDOK;

#ifdef _DEBUG
		printf("MakeNurbs_Thread All Over!!!\n");
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	// �I������
	_dp.SetDecimal3();
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��

	// ��������NC���ނ̏����گ��(�D��x��������)
	AfxBeginThread(MakeNurbs_AfterThread, NULL,
		THREAD_PRIORITY_IDLE);

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	CNCMakeMill::InitialVariable();		// CNCMakeBase::InitialVariable()
}

void SetStaticOption(void)
{
	// ���W�l�̐���
	switch ( g_enMode ) {
	case ROUGH:
		g_dZoffset = Get3dFlg(D3_FLG_ROUGH_ZORIGIN) ? Get3dDbl(D3_DBL_WORKHEIGHT) : 0.0f;
		break;
	case CONTOUR:
		g_dZoffset = Get3dFlg(D3_FLG_CONTOUR_ZORIGIN) ? Get3dDbl(D3_DBL_CONTOUR_ZMAX) : 0.0f;
		break;
	default:
		g_dZoffset = 0.0f;
	}

	// ABS, INC �֌W�Ȃ� G92�l�ŏ�����
	for ( int i=0; i<NCXYZ; i++ )
		CNCMakeMill::ms_xyz[i] = RoundUp(GetDbl(MKNC_DBL_G92X+i));

	// ������߼�݂ɂ��ÓI�ϐ��̏�����
	CNCMakeMill::SetStaticOption(g_pMakeOpt);
}

BOOL OutputNurbsCode(void)
{
	CString	strPath, strFile,
			strNCFile(g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_pParent, g_nFase, g_obMakeData.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		UINT	nOpenFlg = CFile::modeCreate | CFile::modeWrite |
			CFile::shareExclusive | CFile::typeText | CFile::osSequentialScan;
		CStdioFile	fp(strNCFile, nOpenFlg);
		for ( INT_PTR i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
			g_obMakeData[i]->WriteGcode(fp);
			SetProgressPos(g_pParent, i+1);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	SetProgressPos(g_pParent, g_obMakeData.GetSize());
	return IsThread();
}

//////////////////////////////////////////////////////////////////////
//	�r���H�������[�v
//////////////////////////////////////////////////////////////////////

BOOL MakeNurbs_RoughFunc(void)
{
	VVVCoord::iterator	itv;
	VVVCoord&	vvv = g_pDoc->GetRoughCoord();
	INT_PTR		maxcnt = 0, cnt = 0;
	ptrdiff_t	idx;
	int			r;	// 0:����, 1:�t��

	// Coord::dmy �̃N���A�D�����ς݃t���O�Ƃ��Ďg�p
	for ( itv=vvv.begin(); itv!=vvv.end(); ++itv ) {
		maxcnt += itv->size();			// ���W�W��==������
		for ( auto it2=itv->begin(); it2!=itv->end(); ++it2 ) {
			for ( auto it3=it2->begin(); it3!=it2->end(); ++it3 ) {
				it3->dmy = 0.0;			// �����ς݃t���O�̃N���A
			}
		}
	}

	// �t�F�[�Y�X�V
	SendFaseMessage(g_pParent, g_nFase, maxcnt);

	// G����ͯ��(�J�n����)
	AddCustomNurbsCode(MKNC_STR_HEADER);

	// �ŏ��̊J�n�|�C���g������
	// �ŏ����� front �� back ����̌���
	itv = vvv.begin();
	tie(idx, r) = SearchFirstPoint(*itv);
	if ( idx < 0 ) {
		return FALSE;
	}
	MakeLoopCoord(itv->operator[](idx), 0, r);
	SetProgressPos(g_pParent, ++cnt);

	// �X�L�������W�̐���
	while ( IsThread() ) {
		tie(idx, r) = SearchNearEdge(*itv);
		if ( idx < 0 ) {
			if ( ++itv == vvv.end() ) {
				// �p�X�������[�v�I��
				break;
			}
			continue;
		}
		MakeLoopCoord(itv->operator[](idx), 0, r);
		SetProgressPos(g_pParent, ++cnt);
	}

	// Z�����C�j�V�����_�ɕ��A
	_AddMoveG00Z(GetDbl(MKNC_DBL_G92Z));

	// G����̯��(�I������)
	AddCustomNurbsCode(MKNC_STR_FOOTER);

	return IsThread();
}

//////////////////////////////////////////////////////////////////////
//	�d�グ�������̐������[�v
//////////////////////////////////////////////////////////////////////

BOOL MakeNurbs_ContourFunc(void)
{
	VVVCoord::iterator	itv;
	VVVCoord&	vvv = g_pDoc->GetContourCoord();
	std::vector<size_t>		vLayer;		// �K�w���Ƃ̎c�O���[�v��
	size_t		layer = 0;				// ���ݏ������̊K�w
	INT_PTR		maxcnt = 0, cnt = 0;
	ptrdiff_t	grp, idx;
	CPointD		pt;

	// Coord::dmy �̃N���A�D�����ς݃t���O�Ƃ��Ďg�p
	for ( itv=vvv.begin(); itv!=vvv.end(); ++itv ) {
		maxcnt += itv->size();			// ���W�W��==������
		vLayer.push_back(itv->size());	// �K�w���Ƃ̍��W�O���[�v���ŏ�����
		for ( auto it2=itv->begin(); it2!=itv->end(); ++it2 ) {
			for ( auto it3=it2->begin(); it3!=it2->end(); ++it3 ) {
				it3->dmy = 0.0;			// �����ς݃t���O�̃N���A
			}
		}
	}
	
	// �t�F�[�Y�X�V
	SendFaseMessage(g_pParent, g_nFase, maxcnt);

	// G����ͯ��(�J�n����)
	AddCustomNurbsCode(MKNC_STR_HEADER);

	// �ŏ��̊K�w����J�n�ʒu������
	// �G���[�ōŏ��̊K�w�ɗL���ȃf�[�^���Ȃ��ꍇ������̂�
	// �f�[�^�����ł���Ƃ���܂Ń��[�v
	for ( itv=vvv.begin(); itv!=vvv.end() && IsThread(); ++itv ) {
		tie(grp, idx) = SearchNearGroup(*itv);
		if ( grp >= 0 ) {
			MakeLoopCoord(itv->operator[](grp), idx);
			layer = std::distance(vvv.begin(), itv);
			vLayer[layer]--;
			break;
		}
	}
	if ( itv == vvv.end() ) {
		return FALSE;	
	}

	// �p�X�������[�v
	while ( IsThread() ) {
		SetProgressPos(g_pParent, ++cnt);
		// ���̊K�w��
		layer++;
		if ( layer >= vvv.size() ) {
			// �܂��c���Ă���K�w����
			auto f = find_if(vLayer, lambda::_1>0);
			if ( f == vLayer.end() ) {
				// �p�X�������[�v�I��
				break;
			}
			layer = std::distance(vLayer.begin(), f);
		}
		// ���̊K�w�ŋ߂����W�O���[�v������
		tie(grp, idx) = SearchNearGroup(vvv[layer]);
		if ( grp < 0 ) {
			ASSERT(TRUE);	// �Ȃ��͂����Ȃ�
			break;
		}
		//
		pt.x = vvv[layer][grp][idx].x - CNCMakeMill::ms_xyz[NCA_X];
		pt.y = vvv[layer][grp][idx].y - CNCMakeMill::ms_xyz[NCA_Y];
		if ( hypot(pt.x, pt.y) > Get3dDbl(D3_DBL_CONTOUR_SHIFT)*2.5 ) {
			// �K�w�V�t�g��*2.5���傫���ړ��̏ꍇ��
			// R�_�܂ŏ㏸�i�Փˉ���j
			_AddMoveG00Z(GetDbl(MKNC_DBL_ZG0STOP));
		}
		else {
			// �ŏ��̈ړ���
			// ���݈ʒu�{R�_�����㏸
			_AddMoveG00Z(CNCMakeMill::ms_xyz[NCA_Z] + GetDbl(MKNC_DBL_ZG0STOP));
		}
		// ���̊K�w�Ő���
		MakeLoopCoord(vvv[layer][grp], idx);
		// ���̊K�w�̃O���[�v�������炷
		vLayer[layer]--;
	}

	// Z�����C�j�V�����_�ɕ��A
	_AddMoveG00Z(GetDbl(MKNC_DBL_G92Z));

	// G����̯��(�I������)
	AddCustomNurbsCode(MKNC_STR_FOOTER);

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

tuple<ptrdiff_t, int>	SearchFirstPoint(const VVCoord& vv)
{
	ptrdiff_t	idx;
	int			r;
	// �ŏ��ƍŌ�̓_
	CPoint3D	pt0( vv.front().front() ),
				pt1( vv.front().back()  ),
				pt2( vv.back().front()  ),
				pt3( vv.back().back()   );
	CPointD		ptNow(CNCMakeMill::ms_xyz[NCA_X], CNCMakeMill::ms_xyz[NCA_Y]);
	pt0 -= ptNow;
	pt1 -= ptNow;
	pt2 -= ptNow;
	pt3 -= ptNow;
	// �����v�Z
	// �Œ蒷�z��Ȃ̂� vector �ł͂Ȃ� array
	boost::array<double, 4>	v = {
		pt0.x*pt0.x + pt0.y*pt0.y,
		pt1.x*pt1.x + pt1.y*pt1.y,
		pt2.x*pt2.x + pt2.y*pt2.y,
		pt3.x*pt3.x + pt3.y*pt3.y
	};
	// ��ԏ������v�f
	auto	it  = boost::range::min_element(v);
	switch ( std::distance(v.begin(), it) ) {
	case 0:		// pt0
		idx = 0;
		r = 0;		// ����
		break;
	case 1:		// pt1
		idx = 0;
		r = 1;		// �t��
		break;
	case 2:		// pt2
		idx = vv.size()-1;
		r = 0;
		break;
	case 3:		// pt3
		idx = vv.size()-1;
		r = 1;
		break;
	}

	return make_tuple(idx, r);
}

tuple<ptrdiff_t, int> SearchNearEdge(const VVCoord& vv)
{
	CPointD		ptNow(CNCMakeMill::ms_xyz[NCA_X], CNCMakeMill::ms_xyz[NCA_Y]),
				ptF, ptB;
	ptrdiff_t	idx = -1;
	double		dGapF, dGapB, dGapMin = HUGE_VAL;
	int			r;

	for ( auto it=vv.begin(); it!=vv.end() && IsThread(); ++it ) {
		if ( it->front().dmy > 0 ) continue;
		ptF  = it->front();
		ptB  = it->back();
		ptF -= ptNow;
		ptB -= ptNow;
		dGapF = ptF.x*ptF.x + ptF.y*ptF.y;
		dGapB = ptB.x*ptB.x + ptB.y*ptB.y;
		if ( dGapF <= dGapB ) {
			if ( dGapF < dGapMin ) {
				dGapMin = dGapF;
				idx = std::distance(vv.begin(), it);
				r = 0;
			}
		}
		else {
			if ( dGapB < dGapMin ) {
				dGapMin = dGapB;
				idx = std::distance(vv.begin(), it);
				r = 1;
			}
		}
	}

	return make_tuple(idx, r);
}

tuple<ptrdiff_t, ptrdiff_t> SearchNearGroup(const VVCoord& vv)
{
	ptrdiff_t	i, grp = -1, idx = -1;
	double		dGap, dGapMin = HUGE_VAL;

	for ( auto it=vv.begin(); it!=vv.end() && IsThread(); ++it ) {
		tie(i, dGap) = SearchNearPoint(*it);
		if ( 0<=i && dGap<dGapMin ) {
			dGapMin = dGap;
			grp = std::distance(vv.begin(), it);
			idx = i;
		}
	}

	return make_tuple(grp, idx);
}

tuple<ptrdiff_t, double> SearchNearPoint(const VCoord& v)
{
	CPointD		ptNow(CNCMakeMill::ms_xyz[NCA_X], CNCMakeMill::ms_xyz[NCA_Y]), pt;
	double		dGap, dGapMin = HUGE_VAL;
	ptrdiff_t	minID = -1;

	for ( auto it=v.begin(); it!=v.end() && IsThread(); ++it ) {
		if ( it->dmy > 0 ) continue;	// �����ς�
		pt  = *it;
		pt -= ptNow;					// ���݈ʒu�Ƃ̍�
		dGap = pt.x*pt.x + pt.y*pt.y;
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			minID = std::distance(v.begin(), it);
		}
	}

	return make_tuple(minID, dGapMin);
}

void MakeLoopCoord(VCoord& v, size_t idx, int r)
{
	CPointD		ptF, ptB;
	CPoint3D	pt;

	if ( r < 0 ) {
		// �֐����ŕ����W�������f�i�d�グ�p�X�p�j
		ptF = v.front();
		ptB = v.back();
		if ( ptF.hypot(&ptB) < Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0 ) {
			// �J�n�_�Ɉړ�
			pt = v[idx];
			_AddMovePoint(pt);
			// �����܂ŉ��~
			_AddMakeG01Zcut(pt.z);
			// idx���珇�ɐ���
			size_t	i;
			for ( i=idx; i<v.size(); i++ ) {
				_AddMakeCoord(v[i]);
				v[i].dmy = 1.0;
			}
			for ( i=0; i<idx; i++ ) {
				_AddMakeCoord(v[i]);
				v[i].dmy = 1.0;
			}
			// �J�n�_�܂łȂ�
			_AddMakeCoord(v[idx]);
			// �����I��
			return;
		}
	}

	if ( r < 0 ) {
		// �֐����łǂ��炪�߂������f
		CPointD	ptNow(CNCMakeMill::ms_xyz[NCA_X], CNCMakeMill::ms_xyz[NCA_Y]);
		ptF -= ptNow;
		ptB -= ptNow;
		if ( ptF.x*ptF.x+ptF.y*ptF.y < ptB.x*ptB.x+ptB.y*ptB.y ) {
			r = 0;	// ����
		}
		else {
			r = 1;	// �t��
		}
	}

	// �[�_���珇�ɐ���
	if ( r == 0 ) {
		// ����
		pt = v.front();
		_AddMovePoint(pt);
		_AddMakeG01Zcut(pt.z);
		BOOST_FOREACH(Coord& c, v) {
			_AddMakeCoord(c);
			c.dmy = 1.0;
		}
	}
	else {
		// �t��
		pt = v.back();
		_AddMovePoint(pt);
		_AddMakeG01Zcut(pt.z);
		BOOST_REVERSE_FOREACH(Coord& c, v) {
			_AddMakeCoord(c);
			c.dmy = 1.0;
		}
	}
}

//////////////////////////////////////////////////////////////////////

//	AddCustomNurbsCode() ����Ăяo��
class CMakeCustomNurbsCode : public CMakeCustomCode	// MakeCustomCode.h
{
	BOOL	m_bComment;		// Endmill���̃R�����g��}���������ǂ���

public:
	CMakeCustomNurbsCode(int n) :
				CMakeCustomCode(g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "G92_INITIAL", "G92X", "G92Y", "G92Z",
			"SPINDLE", "G0XY_INITIAL"
		};
		// ���ް�ǉ�
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
		// �R�����g�}��������
		m_bComment = n==MKNC_STR_HEADER ? FALSE : TRUE;	// Header�ȊO�ɂ͓���Ȃ�
	}

	CString	ReplaceCustomCode(const string& str) {
		extern	LPCTSTR	gg_szReturn;		// "\n";
		extern	const	DWORD	g_dwSetValFlags[];
		int		nTestCode;
		float	dValue[VALUESIZE];
		CString	strResult;

		// ���׽�Ăяo��
		tie(nTestCode, strResult) = CMakeCustomCode::ReplaceCustomCode(str);
		if ( !strResult.IsEmpty() )
			return strResult;	// �u���ς݂Ȃ�߂�i������G�R�[�h�͗��Ȃ��j

		// �h��replace
		switch ( nTestCode ) {
		case 0:		// ProgNo
			if ( GetFlg(MKNC_FLG_PROG) )
				strResult.Format(IDS_MAKENCD_PROG,
					GetFlg(MKNC_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKNC_NUM_PROG));
			break;
		case 1:		// G90orG91
			strResult = CNCMakeBase::MakeCustomString(GetNum(MKNC_NUM_G90)+90);
			break;
		case 2:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			dValue[NCA_Z] = GetDbl(MKNC_DBL_G92Z);
			strResult = CNCMakeBase::MakeCustomString(92, NCD_X|NCD_Y|NCD_Z, dValue, FALSE);
			break;
		case 3:		// G92X
		case 4:		// G92Y
		case 5:		// G92Z
			nTestCode -= 3;
			dValue[nTestCode] = GetDbl(MKNC_DBL_G92X+nTestCode);
			strResult = CNCMakeBase::MakeCustomString(-1, g_dwSetValFlags[nTestCode], dValue, FALSE);
			break;
		case 6:		// SPINDLE
			strResult = CNCMakeMill::MakeSpindle(DXFLINEDATA);	// GetNum(MKNC_NUM_SPINDLE)
			break;
		case 7:		// G0XY_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			strResult = CNCMakeBase::MakeCustomString(0, NCD_X|NCD_Y, dValue);
			break;
		default:
			strResult = str.c_str();
			if ( strResult[0]=='%' && !m_bComment ) {
				// '%'�̎��̍s�ɃR�����g��}��
				CString	strBuf;
				strBuf.Format(IDS_MAKENCD_ENDMILL, Get3dDbl(D3_DBL_ROUGH_BALLENDMILL));
				strResult += gg_szReturn + strBuf;
				m_bComment = TRUE;
			}
		}

		if ( strResult[0]=='G' && !m_bComment ) {
			// �ŏ���G�R�[�h�̑O�ɃR�����g��}��
			CString	strBuf;
			strBuf.Format(IDS_MAKENCD_ENDMILL, Get3dDbl(D3_DBL_ROUGH_BALLENDMILL));
			strResult = strBuf + gg_szReturn + strResult;
			m_bComment = TRUE;
		}

		return strResult;
	}
};

void AddCustomNurbsCode(int n)
{
	CString		strFileName, strBuf, strResult;
	CMakeCustomNurbsCode		custom(n);
	string	str, strTok;
	tokenizer<custom_separator>	tokens(str);

	strFileName = GetStr(n);	// MKNC_STR_HEADER or MKNC_STR_FOOTER

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) && IsThread() ) {
			str = strBuf;
			tokens.assign(str);
			strResult.Empty();
			BOOST_FOREACH(strTok, tokens) {
				strResult += custom.ReplaceCustomCode(strTok);
			}
			if ( !strResult.IsEmpty() )
				_AddMakeNurbsStr(strResult);
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// ���̴װ�͐�������(�x���̂�)
	}
}

//////////////////////////////////////////////////////////////////////

// NC�����̸�۰��ٕϐ�������(��n��)�گ��
UINT MakeNurbs_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

#ifdef _DEBUG
	printf("MakeNurbs_AfterThread() Start\n");
#endif
	for ( int i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	g_obMakeData.RemoveAll();

	if ( g_pMakeOpt )
		delete	g_pMakeOpt;

	g_csMakeAfter.Unlock();

	return 0;
}
