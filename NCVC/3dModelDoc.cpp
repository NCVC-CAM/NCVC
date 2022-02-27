// 3dModelDoc.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dModelDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(C3dModelDoc, CDocument)

BEGIN_MESSAGE_MAP(C3dModelDoc, CDocument)
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

	return TRUE;
}

void C3dModelDoc::OnCloseDocument() 
{
	// �������̽گ�ނ𒆒f������
	OnCloseDocumentBase();		// ̧�ٕύX�ʒm�گ��

	__super::OnCloseDocument();
}

/////////////////////////////////////////////////////////////////////////////

void C3dModelDoc::MakeScanPath(NURBSS* ns, NURBSC* nc, SCANSETUP& s)
{
	// Kodatuno User's Guide �����������3xCAM�̍쐬
	NURBS_Func	nf;				// NURBS_Func�ւ̃C���X�^���X
	Coord		plane_pt;		// �������镽�ʏ��1�_
	Coord		plane_n;		// �������镽�ʂ̖@���x�N�g��
	vector<Coord>	v_path;		// �ꎞ�i�[�p�o�b�t�@
	int				ptnum;
//	vector<int>		v_ptnum;	// �X�L�������C��1�{���Ƃ̉��H�_�����i�[�����Ԃ񂢂�Ȃ�
	int		i, j, k,
			D = (int)(s.dHeight / s.dZCut) + 1;	// Z�����������i�e���H�p�j

	// ���W�_�̏�����
	m_vPath.clear();
	m_vPath.resize(D+1);
	for ( auto& v : m_vPath ) v.resize(s.nLineSplit+1);

	// �K�C�h�J�[�u�ɉ����Đ������ʂ��V�t�g���Ă����C���H�ʂƂ̌�_�Q�����߂Ă���
	for ( i=0; i<=s.nLineSplit; i++ ) {
		double t = (double)i/s.nLineSplit;
		if ( i==0 ) {
			t += 0.0001;		// ���ٓ_���
		}
		else if ( i==s.nLineSplit ) {
			t-= 0.0001;			// ���ٓ_���
		}
		plane_pt = nf.CalcNurbsCCoord(nc, t);		// ���ڒ��̐������ʏ��1�_
		plane_n  = nf.CalcTanVecOnNurbsC(nc, t);	// ���ڒ��̐������ʂ̖@���x�N�g��
		// �����C�u�������̕ύX�҂�
		ptnum = nf.CalcIntersecPtsPlaneSearch(ns, plane_pt, plane_n, 0.5, 3, v_path, RUNGE_KUTTA);	// ��_�Q�Z�o
//		v_ptnum.push_back(ptnum);
		// ����ꂽ��_�Q���C���H�ʖ@�������ɍH��a���I�t�Z�b�g�������_�𓾂�
		for ( j=0; j<ptnum; j++ ) {
			Coord pt = nf.CalcNurbsSCoord(ns, v_path[j].x, v_path[j].y);	// �H��R���^�N�g�_
			Coord n = nf.CalcNormVecOnNurbsS(ns, v_path[j].x, v_path[j].y);	// �@���x�N�g��
			if (n.z < 0) n = n*(-1);					// �@���x�N�g���̌�������
//			m_vPath[D][i][j] = pt + n*s.dBallEndmill;	// �H��a�I�t�Z�b�g
			m_vPath[D][i].push_back( pt + n*s.dBallEndmill );	// �H��a�I�t�Z�b�g
		}
	}

	// �e���H�p�X����
	for ( i=0; i<D; i++ ) {
		for ( j=0; j<s.nLineSplit+1; j++ ) {
//			for ( k=0; k<v_ptnum[j]; k++ ) {
			for ( k=0; k<m_vPath[i][j].size(); k++ ) {
				double del = (s.dHeight - m_vPath[D][j][k].z)/(double)D;
				double Z = s.dHeight - del*(double)i;
				m_vPath[i][j][k] = SetCoord(m_vPath[D][j][k].x, m_vPath[D][j][k].y, Z);
			}
		}
	}
}
