// 3dModelDoc.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dModelDoc.h"
#include "MakeNCDlg.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(C3dModelDoc, CDocument)

BEGIN_MESSAGE_MAP(C3dModelDoc, CDocument)
	ON_UPDATE_COMMAND_UI(ID_FILE_3DCUT, &C3dModelDoc::OnUpdateFile3dMake)
	ON_COMMAND(ID_FILE_3DCUT, &C3dModelDoc::OnFile3dMake)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc

C3dModelDoc::C3dModelDoc()
{
	m_pKoBody  = NULL;
	m_pKoList = NULL;
	m_rcMax.SetRectMinimum();
	m_pRoughCoord = NULL;
	m_nRoughX = m_nRoughY = 0;
	m_pRoughNum = NULL;
}

C3dModelDoc::~C3dModelDoc()
{
	if ( m_pKoBody ) {
		m_pKoBody->DelBodyElem();
		delete	m_pKoBody;
	}
	if ( m_pKoList ) {
		m_pKoList->clear();
		delete	m_pKoList;
	}
	ClearRoughCoord();
	ClearContourCoord();
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �f�f


/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �V���A����

void C3dModelDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring()) return;	// �ۑ��͍��̂Ƃ���i�V

	const CFile* fp = ar.GetFile();
	CString	strPath( fp->GetFilePath() );

	// �R�c���f���̓ǂݍ���
	m_pKoBody = Read3dModel(strPath);
	if ( !m_pKoBody ) {
		return;
	}
	// Kodatuno BODY �o�^
	m_pKoList = new BODYList;
	m_pKoBody->RegistBody(m_pKoList, strPath);
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �R�}���h

BOOL C3dModelDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!__super::OnOpenDocument(lpszPathName))
		return FALSE;

	// �޷���ĕύX�ʒm�گ�ނ̐���
	OnOpenDocumentBase(lpszPathName);	// CDocBase

	// ����t�H���_�ɂ���I�v�V�����t�@�C���̓ǂݍ���
	m_3dOpt.Read3dOption(lpszPathName);

	// ��L��`�̎擾
	BODY*		pBody;
	CPoint3D	pt;
	int		i, nLoop = m_pKoList->getNum();

	for ( i=0; i<nLoop; i++ ) {
		pBody = (BODY *)m_pKoList->getData(i);
		// ���C�u����������������
		pt = pBody->minmaxCoord[0];
		m_rcMax |= pt;
		pt = pBody->minmaxCoord[1];
		m_rcMax |= pt;
	}
#ifdef _DEBUG
	printf("m_rcMax=(%f, %f)-(%f, %f) h=%f l=%f\n",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom,
		m_rcMax.high, m_rcMax.low);
#endif

	return TRUE;
}

void C3dModelDoc::OnCloseDocument() 
{
	// �������̽گ�ނ𒆒f������
	OnCloseDocumentBase();		// ̧�ٕύX�ʒm�گ��

	__super::OnCloseDocument();
}

void C3dModelDoc::OnUpdateFile3dMake(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pRoughCoord!=NULL || !m_vvvContourCoord.empty() );
}

void C3dModelDoc::OnFile3dMake()
{
	CString	strInit;
	BOOL	bNCView;
	{
		CMakeNCDlg	dlg(IDS_MAKENCD_TITLE_NURBS, NCMAKENURBS, this);
		if ( dlg.DoModal() != IDOK )
			return;
		m_strNCFileName	= dlg.m_strNCFileName;
		strInit = dlg.m_strInitFileName;
		bNCView = dlg.m_bNCView;
	}

	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();

	// �ݒ�̕ۑ�
	if ( !strInit.IsEmpty() ) {
		pOpt->AddInitHistory(NCMAKENURBS, strInit);
	}
	pOpt->SetViewFlag(bNCView);

	// ���łɊJ���Ă����޷���ĂȂ����
	CDocument* pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_NCD, m_strNCFileName);
	if ( pDoc ) {
		pDoc->OnCloseDocument();
	}

	// �����J�n
	CThreadDlg*	pDlg = new CThreadDlg(ID_FILE_3DROUGH, this);
	INT_PTR		nResult = pDlg->DoModal();
	delete	pDlg;

	// NC��������ް����J��
	if ( nResult==IDOK && bNCView ) {
		AfxGetNCVCApp()->OpenDocumentFile(m_strNCFileName);
	}
}

/////////////////////////////////////////////////////////////////////////////

void C3dModelDoc::ClearRoughCoord(void)
{
	if ( m_pRoughCoord ) {
		FreeCoord3(m_pRoughCoord, m_nRoughX, m_nRoughY);
		m_pRoughCoord = NULL;
		m_nRoughX = m_nRoughY = 0;
	}
	if ( m_pRoughNum ) {
		delete[]	m_pRoughNum;
		m_pRoughNum = NULL;
	}
}

void C3dModelDoc::ClearContourCoord(void)
{
	m_vvvContourCoord.clear();
}

BOOL C3dModelDoc::MakeRoughCoord(NURBSS* ns, NURBSC* nc)
{
	// Kodatuno User's Guide �����������3xCAM�̍쐬
	NURBS_Func	nf;			// NURBS_Func�ւ̃C���X�^���X
	Coord	plane_pt;		// �������镽�ʏ��1�_
	Coord	plane_n;		// �������镽�ʂ̖@���x�N�g��
	Coord	path_[2000];	// �ꎞ�i�[�p�o�b�t�@
	int		i, j, k,
			D = (int)(m_3dOpt.Get3dDbl(D3_DBL_WORKHEIGHT) / m_3dOpt.Get3dDbl(D3_DBL_ROUGH_ZCUT)) + 1,	// Z�����������i�e���H�p�j
			N = m_3dOpt.Get3dInt(D3_INT_LINESPLIT);					// �X�L���j���O���C��������(N < 100)
	BOOL	bResult = TRUE;

	try {
		// ���W�_�̏�����
		ClearRoughCoord();
		ClearContourCoord();
		m_nRoughX = D+1;
		m_nRoughY = N+1;
		m_pRoughCoord = NewCoord3(m_nRoughX, m_nRoughY, 2000);
		m_pRoughNum = new int[100];

		// �K�C�h�J�[�u�ɉ����Đ������ʂ��V�t�g���Ă����C���H�ʂƂ̌�_�Q�����߂Ă���
		for ( i=0; i<=N; i++ ) {
			double t = (double)i/N;
			if ( i==0 ) {
				t += 0.0001;		// ���ٓ_���
			}
			else if ( i==N ) {
				t -= 0.0001;		// ���ٓ_���
			}
			plane_pt = nf.CalcNurbsCCoord(nc, t);		// ���ڒ��̐������ʏ��1�_
			plane_n  = nf.CalcTanVecOnNurbsC(nc, t);	// ���ڒ��̐������ʂ̖@���x�N�g��
			m_pRoughNum[i] = nf.CalcIntersecPtsPlaneSearch(ns, plane_pt, plane_n, 0.5, 3, path_, 2000, RUNGE_KUTTA);  // ��_�Q�Z�o
			// ����ꂽ��_�Q���C���H�ʖ@�������ɍH��a���I�t�Z�b�g�������_�𓾂�
			for ( j=0; j<m_pRoughNum[i]; j++ ) {
				Coord pt = nf.CalcNurbsSCoord(ns, path_[j].x, path_[j].y);		// �H��R���^�N�g�_
				Coord n = nf.CalcNormVecOnNurbsS(ns, path_[j].x, path_[j].y);	// �@���x�N�g��
				if (n.z < 0) n = n*(-1);					// �@���x�N�g���̌�������
				m_pRoughCoord[D][i][j] = pt + n*m_3dOpt.Get3dDbl(D3_DBL_ROUGH_BALLENDMILL);	// �H��a�I�t�Z�b�g
			}
		}

		// �e���H�p�X����
		for ( i=0; i<D; i++ ) {
			for ( j=0; j<m_nRoughY; j++ ) {
				for ( k=0; k<m_pRoughNum[j]; k++ ) {
					double del = (m_3dOpt.Get3dDbl(D3_DBL_WORKHEIGHT) - m_pRoughCoord[D][j][k].z)/(double)D;
					double Z = m_3dOpt.Get3dDbl(D3_DBL_WORKHEIGHT) - del*i;
					m_pRoughCoord[i][j][k] = SetCoord(m_pRoughCoord[D][j][k].x, m_pRoughCoord[D][j][k].y, Z);
				}
			}
		}
	}
	catch(...) {
		// ���C�u�������̗�O�ɑΉ�
		ClearRoughCoord();
		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

	// �X�L�����I�v�V�����̕ۑ�
	if ( bResult )
		m_3dOpt.Save3dOption();

	return bResult;
}

BOOL C3dModelDoc::MakeContourCoord(NURBSS* ns)
{
	// Kodatuno User's Guide �������𐶐�����
	NURBS_Func	nf;		// NURBS�������֐��W���Ăяo��
	VCoord	v;			// 1���ʂ̌�_�Q
	Coord	pt, p,
			t[5000],	// ���̊i�[
			nvec = SetCoord(0.0, 0.0, 1.0);	// ���ʂ̖@���x�N�g���iXY���ʁj
	double	dSpace = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SPACE),	// ��_�Q�̓_�Ԋu
			dZmin  = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_ZMIN),		// ��������Zmin
			dZmax  = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_ZMAX),		// ��������Zmax
			dShift = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SHIFT);	// ������������Z�Ԋu
	int		i, j, num,
			step = (int)(fabs(dZmax - dZmin) / dShift + 1);		// �������̖{��
	BOOL	bResult = TRUE;

	try {
		// ���W�_�̏�����
		ClearRoughCoord();
		ClearContourCoord();

		// ���ʂ�Z�����ɃV�t�g���Ă����Ȃ��瓙�������Z�o����
		for ( i=0; i<step; i++ ) {
			pt = SetCoord(0.0, 0.0, dZmax - dShift*i);	// ���݂̕��ʂ�Z�ʒu��1�_���w���i��ʂ���v�Z�j
			num = nf.CalcIntersecPtsPlaneSearch(ns, pt, nvec, dSpace, 5, t, 5000, RUNGE_KUTTA);		// NURBS�Ȗʂƕ��ʂƂ̌�_�Q������ǐՖ@�ŋ��߂�
			for ( j=0; j<num; j++ ) {
				p = nf.CalcNurbsSCoord(ns, t[j].x, t[j].y);		// ��_���p�����[�^�l������W�l�֕ϊ�
				p.dmy = 0.0;
				v.push_back(p);
			}
			if ( !v.empty() ) {
				// 1���ʂ̍��W���O���[�v�W���œo�^
				SetCoordGroup(v);
				v.clear();
			}
		}
	}
	catch(...) {
		// ���C�u�������̗�O�ɑΉ�
		ClearRoughCoord();
		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

#ifdef _DEBUG
	printf("�K�w=%zd\n", m_vvvContourCoord.size());
	for ( auto it1=m_vvvContourCoord.begin(); it1!=m_vvvContourCoord.end(); ++it1 ) {
		printf(" �W��%Id=%zd\n", std::distance(m_vvvContourCoord.begin(), it1), it1->size());
		for ( auto it2=it1->begin(); it2!=it1->end(); ++it2 ) {
			CPointD	ptF( it2->front().x, it2->front().y ),
					ptB( it2->back().x,  it2->back().y  );
			BOOL	bLoop;
			if ( ptF.hypot(&ptB) < m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0f ) {
				bLoop = TRUE;
			}
			else {
				bLoop = FALSE;
			}
			printf("   %Id size=%zd(%c)\n", std::distance(it1->begin(), it2), it2->size(), bLoop ? 'o' : 'x');
		}
	}
#endif

	// �X�L�����I�v�V�����̕ۑ�
	if ( bResult )
		m_3dOpt.Save3dOption();

	return bResult;
}

void C3dModelDoc::SetCoordGroup(VCoord& v)
{
	__int64	idx;
	size_t	i, grp;	// ���ݏ����Ώۂ�vGroup
	double	dGap, dMargin = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0;
	VCoord	vGroup;
	VVCoord	vv;

	// �ŏ��̌����|�C���g
	vGroup.push_back(v.front());
	vv.push_back(vGroup);
	grp = 0;
	CPointD	ptNow(v.front().x, v.front().y);
	v.front().dmy = 1.0;

	while ( TRUE ) {
		boost::tie(idx, dGap) = SearchNearPoint(v, ptNow);
		if ( idx < 0 ) {
			break;	// ���[�v�I������
		}
		else if ( sqrt(dGap) < dMargin ) {
			// ����O���[�v
			vv[grp].push_back(v[idx]);
		}
		else {
			// ���̃O���[�v���猟��
			for ( i=0; i<vv.size(); i++ ) {
				if ( i==grp )
					continue;
				CPointD	ptF(vv[i].front().x-ptNow.x, vv[i].front().y-ptNow.y),
						ptB(vv[i].back().x -ptNow.x, vv[i].back().y -ptNow.y);
				if ( ptF.hypot() < dMargin ) {
					std::reverse(vv[i].begin(), vv[i].end());
					vv[i].push_back(v[idx]);
					grp = i;
					break;
				}
				else if ( ptB.hypot() < dMargin ) {
					vv[i].push_back(v[idx]);
					grp = i;
					break;
				}
			}
			if ( i >= vv.size() ) {
				// �V�K�O���[�v
				vGroup.clear();
				vGroup.push_back(v[idx]);
				vv.push_back(vGroup);
				grp = vv.size() - 1;
			}
		}
		// �����ς݃}�[�N
		v[idx].dmy = 1.0;
		// ���݈ʒu�X�V
		ptNow.SetPoint(v[idx].x, v[idx].y);
	}

	// 1���ʂ̌�_�Q��ۑ�
	m_vvvContourCoord.push_back(vv);
}

boost::tuple<__int64, double> C3dModelDoc::SearchNearPoint(const VCoord& v, const CPointD& ptNow)
{
	CPointD	pt;
	double	dGap, dGapMin = HUGE_VAL;
	__int64	minID = -1;

	for ( auto it=v.begin(); it!=v.end(); ++it ) {
		if ( it->dmy > 0 ) continue;	// �����ς�
		pt.SetPoint(it->x-ptNow.x, it->y-ptNow.y);	// ���݈ʒu�Ƃ̍�
		dGap = pt.x*pt.x + pt.y*pt.y;	// hypot()�͎g��Ȃ� sqrt()���x��
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			minID = std::distance(v.begin(), it);
		}
	}

	return boost::make_tuple(minID, dGapMin);
}
