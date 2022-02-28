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
	m_pPath = NULL;
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
	if ( m_pPath ) {
		FreeCoord3(m_pPath, m_pPathX, m_pPathY);
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
	Coord		path_[2000];	// �ꎞ�i�[�p�o�b�t�@
	int			ptnum[100];		// �X�L�������C��1�{���Ƃ̉��H�_�����i�[
	int		i, j, k,
			D = (int)(s.dHeight / s.dZCut) + 1;	// Z�����������i�e���H�p�j

	// ���W�_�̏�����
	if ( m_pPath ) {
		FreeCoord3(m_pPath, m_pPathX, m_pPathY);
	}
	m_pPathX = D+1;
	m_pPathY = s.nLineSplit+1;
	m_pPath = NewCoord3(m_pPathX, m_pPathY, 2000);

	// �K�C�h�J�[�u�ɉ����Đ������ʂ��V�t�g���Ă����C���H�ʂƂ̌�_�Q�����߂Ă���
	for ( i=0; i<=s.nLineSplit; i++ ) {
		double t = (double)i/s.nLineSplit;
		if ( i==0 ) {
			t += 0.0001;		// ���ٓ_���
		}
		else if ( i==s.nLineSplit ) {
			t -= 0.0001;		// ���ٓ_���
		}
		plane_pt = nf.CalcNurbsCCoord(nc, t);		// ���ڒ��̐������ʏ��1�_
		plane_n  = nf.CalcTanVecOnNurbsC(nc, t);	// ���ڒ��̐������ʂ̖@���x�N�g��
		ptnum[i] = nf.CalcIntersecPtsPlaneSearch(ns, plane_pt, plane_n, 0.5, 3, path_, 2000, RUNGE_KUTTA);  // ��_�Q�Z�o
		// ����ꂽ��_�Q���C���H�ʖ@�������ɍH��a���I�t�Z�b�g�������_�𓾂�
		for ( j=0; j<ptnum[i]; j++ ) {
			Coord pt = nf.CalcNurbsSCoord(ns, path_[j].x, path_[j].y);		// �H��R���^�N�g�_
			Coord n = nf.CalcNormVecOnNurbsS(ns, path_[j].x, path_[j].y);	// �@���x�N�g��
			if (n.z < 0) n = n*(-1);					// �@���x�N�g���̌�������
			m_pPath[D][i][j] = pt + n*s.dBallEndmill;	// �H��a�I�t�Z�b�g
		}
	}

	// �e���H�p�X����
	for ( i=0; i<D; i++ ) {
		for ( j=0; j<m_pPathY; j++ ) {
			for ( k=0; k<ptnum[j]; k++ ) {
				double del = (s.dHeight - m_pPath[D][j][k].z)/(double)D;
				double Z = s.dHeight - del*(double)i;
				m_pPath[i][j][k] = SetCoord(m_pPath[D][j][k].x, m_pPath[D][j][k].y, Z);
			}
		}
	}
}
