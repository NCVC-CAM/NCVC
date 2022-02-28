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
	ON_UPDATE_COMMAND_UI(ID_FILE_3DPATH, &C3dModelDoc::OnUpdateFile3dMake)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc

C3dModelDoc::C3dModelDoc()
{
	m_pKoBody  = NULL;
	m_pKoList = NULL;
	m_rcMax.SetRectMinimum();
	m_pScanPath = NULL;
	m_pScanX = m_pScanY = 0;
	m_pScanNum = NULL;
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
	ClearScanPath();
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

void C3dModelDoc::OnUpdateFile3dMake(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( m_pScanPath != NULL );
}

/////////////////////////////////////////////////////////////////////////////

void C3dModelDoc::ClearScanPath(void)
{
	if ( m_pScanPath ) {
		FreeCoord3(m_pScanPath, m_pScanX, m_pScanY);
		m_pScanPath = NULL;
	}
	if ( m_pScanNum ) {
		delete[]	m_pScanNum;
		m_pScanNum = NULL;
	}
}

BOOL C3dModelDoc::MakeScanPath(NURBSS* ns, NURBSC* nc, SCANSETUP& s)
{
	// Kodatuno User's Guide �����������3xCAM�̍쐬
	NURBS_Func	nf;				// NURBS_Func�ւ̃C���X�^���X
	Coord		plane_pt;		// �������镽�ʏ��1�_
	Coord		plane_n;		// �������镽�ʂ̖@���x�N�g��
	Coord		path_[2000];	// �ꎞ�i�[�p�o�b�t�@
	int		i, j, k,
			D = (int)(s.dHeight / s.dZCut) + 1,	// Z�����������i�e���H�p�j
			N = s.nLineSplit;					// �X�L���j���O���C��������(N < 100)
	BOOL	bResult = TRUE;

	try {
		// ���W�_�̏�����
		ClearScanPath();
		m_pScanX = D+1;
		m_pScanY = N+1;
		m_pScanPath = NewCoord3(m_pScanX, m_pScanY, 2000);
		m_pScanNum = new int[100];

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
			m_pScanNum[i] = nf.CalcIntersecPtsPlaneSearch(ns, plane_pt, plane_n, 0.5, 3, path_, 2000, RUNGE_KUTTA);  // ��_�Q�Z�o
			// ����ꂽ��_�Q���C���H�ʖ@�������ɍH��a���I�t�Z�b�g�������_�𓾂�
			for ( j=0; j<m_pScanNum[i]; j++ ) {
				Coord pt = nf.CalcNurbsSCoord(ns, path_[j].x, path_[j].y);		// �H��R���^�N�g�_
				Coord n = nf.CalcNormVecOnNurbsS(ns, path_[j].x, path_[j].y);	// �@���x�N�g��
				if (n.z < 0) n = n*(-1);					// �@���x�N�g���̌�������
				m_pScanPath[D][i][j] = pt + n*s.dBallEndmill;	// �H��a�I�t�Z�b�g
			}
		}

		// �e���H�p�X����
		for ( i=0; i<D; i++ ) {
			for ( j=0; j<m_pScanY; j++ ) {
				for ( k=0; k<m_pScanNum[j]; k++ ) {
					double del = (s.dHeight - m_pScanPath[D][j][k].z)/(double)D;
					double Z = s.dHeight - del*(double)i;
					m_pScanPath[i][j][k] = SetCoord(m_pScanPath[D][j][k].x, m_pScanPath[D][j][k].y, Z);
				}
			}
		}
	}
	catch(...) {
		// ���C�u�������̗�O�ɑΉ�
		ClearScanPath();
		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

	return bResult;
}
