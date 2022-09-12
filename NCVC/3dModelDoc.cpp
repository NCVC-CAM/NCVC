// 3dModelDoc.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dModelDoc.h"
#include "MakeNCDlg.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUG_FILEOUT_
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
	ClearKoCoord();
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �f�f


#ifdef _DEBUG
void C3dModelDoc::DumpRoughCoord(const VVCoord& vv)
{
	CString	file, s;

	for ( auto it1=vv.begin(); it1!=vv.end(); ++it1 ) {
		file.Format("C:\\Users\\magara\\Documents\\tmp\\coord%02d.csv", std::distance(vv.begin(), it1));
		CStdioFile	f(file, CFile::typeText|CFile::modeCreate|CFile::modeWrite);
		for ( auto it2=it1->begin(); it2!=it1->end(); ++it2 ) {
			s.Format("%.3f, %.3f, %.3f\n", it2->x, it2->y, it2->z);
			f.WriteString(s);
		}
	}
}

void C3dModelDoc::DumpContourCoord(const VCoord& v)
{
	CString	file, s;
	file.Format("C:\\Users\\magara\\Documents\\tmp\\coord%02d.csv", m_vvvKoCoord.size());
	CStdioFile	f(file, CFile::typeText|CFile::modeCreate|CFile::modeWrite);

	for ( auto it=v.begin(); it!=v.end(); ++it ) {
		s.Format("%.3f, %.3f, %.3f\n", it->x, it->y, it->z);
		f.WriteString(s);
	}
}
#endif

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
	pCmdUI->Enable( !m_vvvKoCoord.empty() );
}

void C3dModelDoc::OnFile3dMake()
{
	CString	strInit;
	BOOL	bNCView;
	UINT	id;
	if ( m_enCoordMode == ROUGH ) {
		id = IDS_MAKENCD_TITLE_ROUGH;
	}
	else {
		id = IDS_MAKENCD_TITLE_CONTOUR;
	}

	{
		CMakeNCDlg	dlg(id, NCMAKENURBS, this);
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
	m_3dOpt.Save3dOutfile(id-IDS_MAKENCD_TITLE_ROUGH, m_strNCFileName);

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

void C3dModelDoc::ClearKoCoord(void)
{
	m_vvvKoCoord.clear();
}

BOOL C3dModelDoc::MakeRoughCoord(NURBSS* ns, NURBSC* nc)
{
	// Kodatuno User's Guide �����������3xCAM�̍쐬
	NURBS_Func	nf;			// NURBS_Func�ւ̃C���X�^���X
	Coord	plane_pt,		// �������镽�ʏ��1�_
			plane_n,		// �������镽�ʂ̖@���x�N�g��
			ct,				// �H��R���^�N�g�_
			nv,				// �@���x�N�g��
			path[2000];		// �ꎞ�i�[�p�o�b�t�@
	VVCoord	vv;
	double	H = m_3dOpt.Get3dDbl(D3_DBL_WORKHEIGHT),				// �f�ޏ�ʂ�Z���W
			R = m_3dOpt.Get3dDbl(D3_DBL_ROUGH_BALLENDMILL),			// �{�[���G���h�~�����a
			offset = m_3dOpt.Get3dDbl(D3_DBL_ROUGH_OFFSET);			// �Ȗʂ���̃I�t�Z�b�g
	int		i, j, num,
			D = (int)(H / m_3dOpt.Get3dDbl(D3_DBL_ROUGH_ZCUT)) + 1,	// Z����������
			N = m_3dOpt.Get3dInt(D3_INT_LINESPLIT);					// �X�L���j���O���C��������(N < 100)
	BOOL	bResult = TRUE;

	try {
		// ���W�_�̏�����
		ClearKoCoord();

		// �K�C�h�J�[�u�ɉ����Đ������ʂ��V�t�g���Ă����C���H�ʂƂ̌�_�Q�����߂Ă���
		for ( i=0; i<=N; i++ ) {
			double t = (double)i/N;
			if ( i==0 ) t += 0.0001;			// ���ٓ_���
			else if ( i==N ) t -= 0.0001;		// ���ٓ_���
			plane_pt = nf.CalcNurbsCCoord(nc, t);		// ���ڒ��̐������ʏ��1�_
			plane_n  = nf.CalcTanVecOnNurbsC(nc, t);	// ���ڒ��̐������ʂ̖@���x�N�g��
			num = nf.CalcIntersecPtsPlaneSearch(ns, plane_pt, plane_n, 0.5, 3, path, 2000, RUNGE_KUTTA);  // ��_�Q�Z�o
			// ����ꂽ��_�Q���C���H�ʖ@�������ɍH��a���I�t�Z�b�g�������_�𓾂�
			VCoord	v;
			for ( j=0; j<num; j++ ) {
				ct = nf.CalcNurbsSCoord(ns, path[j].x, path[j].y);			// �H��R���^�N�g�_
				nv = nf.CalcNormVecOnNurbsS(ns, path[j].x, path[j].y);		// �@���x�N�g��
				if ( nv.z < 0 ) nv = nv * (-1);	// �@���x�N�g���̌�������
				v.push_back(ct + nv*offset);	// �I�t�Z�b�g
			}
			if ( !v.empty() ) {
				// �A��������W�œo�^
				VVCoord vvtmp = SetGroupCoord(v, R/2.0);	// �L���p�͍H��a�̔���
				if ( !vvtmp.empty() ) {
					vv.insert(vv.end(), vvtmp.begin(), vvtmp.end());
				}
			}
		}

		// �r���H�p�X����
		for ( i=0; i<=D; i++ ) {	// Z����������
			VVCoord	vvtmp = vv;		// �f�[�^���R�s�[���Ă���Z�l�̍X�V
			for ( auto it2=vvtmp.begin(); it2!=vvtmp.end(); ++it2 ) {
				for ( auto it3=it2->begin(); it3!=it2->end(); ++it3 ) {
					double del = (H - it3->z)/D;
					it3->z = H - del*i;
				}
			}
			if ( !vvtmp.empty() ) {
				m_vvvKoCoord.push_back(vvtmp);
			}
		}
	}
	catch(...) {
		// ���C�u�������̗�O�ɑΉ�
		ClearKoCoord();
		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

#ifdef _DEBUG_FILEOUT_
	DumpRoughCoord(m_vvvKoCoord[0]);
#endif
#ifdef _DEBUG
	printf("�K�w=%zd\n", m_vvvKoCoord.size());
	for ( auto it1=m_vvvKoCoord.begin(); it1!=m_vvvKoCoord.end(); ++it1 ) {
		printf(" Line%0Id=%zd\n", std::distance(m_vvvKoCoord.begin(), it1), it1->size());
		for ( auto it2=it1->begin(); it2!=it1->end(); ++it2 ) {
			printf("  num=%zd\n", it2->size() );
		}
	}
#endif

	// �X�L�����I�v�V�����̕ۑ�
	if ( bResult ) {
		m_3dOpt.Save3dOption();
		m_enCoordMode = ROUGH;
	}

	return bResult;
}

BOOL C3dModelDoc::MakeContourCoord(NURBSS* ns)
{
	// Kodatuno User's Guide �������𐶐�����
	NURBS_Func	nf;		// NURBS�������֐��W���Ăяo��
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
		ClearKoCoord();

		// ���ʂ�Z�����ɃV�t�g���Ă����Ȃ��瓙�������Z�o����
		for ( i=0; i<step; i++ ) {
			pt = SetCoord(0.0, 0.0, dZmax - dShift*i);		// ���݂̕��ʂ�Z�ʒu��1�_���w���i��ʂ���v�Z�j
			num = nf.CalcIntersecPtsPlaneSearch(ns, pt, nvec, dSpace, 5, t, 5000, RUNGE_KUTTA);		// NURBS�Ȗʂƕ��ʂƂ̌�_�Q������ǐՖ@�ŋ��߂�
			VCoord	v;		// 1���ʂ̌�_�Q
			for ( j=0; j<num; j++ ) {
				p = nf.CalcNurbsSCoord(ns, t[j].x, t[j].y);	// ��_���p�����[�^�l������W�l�֕ϊ�
				v.push_back(p);
			}
			if ( !v.empty() ) {
#ifdef _DEBUG_FILEOUT_
				DumpContourCoord(v);	// �f�o�b�O�p�̍��W�o��
#endif
				// 1���ʂ̍��W���O���[�v�W���œo�^
				VVCoord vv = SetGroupCoord(v, m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0);
				// 1���ʂ̌�_�Q��ۑ�
				if ( !vv.empty() ) {
					m_vvvKoCoord.push_back(vv);
				}
			}
		}
	}
	catch(...) {
		// ���C�u�������̗�O�ɑΉ�
		ClearKoCoord();
		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

#ifdef _DEBUG
	printf("�K�w=%zd\n", m_vvvKoCoord.size());
	for ( auto it1=m_vvvKoCoord.begin(); it1!=m_vvvKoCoord.end(); ++it1 ) {
		printf(" �W��%Id=%zd\n", std::distance(m_vvvKoCoord.begin(), it1), it1->size());
		for ( auto it2=it1->begin(); it2!=it1->end(); ++it2 ) {
			// ���[�v���ۂ��̔���
			CPointD	ptF(it2->front()),
					ptB(it2->back() );
			BOOL	bLoop;
			if ( ptF.hypot(&ptB) < m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0f ) {
				bLoop = TRUE;
			}
			else {
				bLoop = FALSE;
			}
			printf("  %Id size=%zd(%c) ", std::distance(it1->begin(), it2), it2->size(), bLoop ? 'o' : 'x');
			printf("  s(%.3f, %.3f) e(%.3f, %.3f)\n", ptF.x, ptF.y, ptB.x, ptB.y);
		}
	}
#endif

	// �X�L�����I�v�V�����̕ۑ�
	if ( bResult ) {
		m_3dOpt.Save3dOption();
		m_enCoordMode = CONTOUR;
	}

	return bResult;
}

VVCoord C3dModelDoc::SetGroupCoord(VCoord& v, double dMargin)
{
	VCoord		vtmp;
	VVCoord		vv;
	VVCoord::iterator	itg;
	CPointD		ptF, ptB;

	// �ŏ��̌����|�C���g�o�^
	vtmp.push_back(v.front());
	vv.push_back(vtmp);

	// �߂��O���[�v�ɏW��
	for ( auto itc=v.begin()+1; itc!=v.end(); ++itc ) {
		for ( itg=vv.begin(); itg!=vv.end(); ++itg ) {
			ptF = itg->front() - (*itc);
			ptB = itg->back()  - (*itc);
			if ( ptB.hypot() < dMargin ) {
				itg->push_back(*itc);
				break;
			}
			else if ( ptF.hypot() < dMargin ) {
				boost::range::reverse(*itg);
				itg->push_back(*itc);
				break;
			}
		}
		if ( itg == vv.end() ) {
			// �V�K�O���[�v�̍쐬
			vtmp.clear();
			vtmp.push_back(*itc);
			vv.push_back(vtmp);
		}
	}

	// ���ٓ_�i���W��1�����Ǘ����Ă���_�j�̏���
	itg = vv.begin();
	while ( itg != vv.end() ) {
		if ( itg->size() == 1 ) {
			itg = vv.erase(itg);
		}
		else {
			++itg;
		}
	}

	return vv;
}
