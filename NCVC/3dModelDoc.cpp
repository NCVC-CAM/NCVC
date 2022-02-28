// 3dModelDoc.cpp : 実装ファイル
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
// C3dModelDoc 診断


/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc シリアル化

void C3dModelDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring()) return;	// 保存は今のところナシ

	const CFile* fp = ar.GetFile();
	CString	strPath( fp->GetFilePath() );

	// ３Ｄモデルの読み込み
	m_pKoBody = Read3dModel(strPath);
	if ( !m_pKoBody ) {
		return;
	}
	// Kodatuno BODY 登録
	m_pKoList = new BODYList;
	m_pKoBody->RegistBody(m_pKoList, strPath);
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc コマンド

BOOL C3dModelDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!__super::OnOpenDocument(lpszPathName))
		return FALSE;

	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
	OnOpenDocumentBase(lpszPathName);	// CDocBase

	// 占有矩形の取得
	BODY*		pBody;
	CPoint3D	pt;
	int		i, nLoop = m_pKoList->getNum();

	for ( i=0; i<nLoop; i++ ) {
		pBody = (BODY *)m_pKoList->getData(i);
		// ライブラリ側を少し改造
		pt = pBody->minmaxCoord[0];
		m_rcMax |= pt;
		pt = pBody->minmaxCoord[1];
		m_rcMax |= pt;
	}

	return TRUE;
}

void C3dModelDoc::OnCloseDocument() 
{
	// 処理中のｽﾚｯﾄﾞを中断させる
	OnCloseDocumentBase();		// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ

	__super::OnCloseDocument();
}

/////////////////////////////////////////////////////////////////////////////

void C3dModelDoc::MakeScanPath(NURBSS* ns, NURBSC* nc, SCANSETUP& s)
{
	// Kodatuno User's Guide いいかげんな3xCAMの作成
	NURBS_Func	nf;				// NURBS_Funcへのインスタンス
	Coord		plane_pt;		// 分割する平面上の1点
	Coord		plane_n;		// 分割する平面の法線ベクトル
	Coord		path_[2000];	// 一時格納用バッファ
	int			ptnum[100];		// スキャンライン1本ごとの加工点数を格納
	int		i, j, k,
			D = (int)(s.dHeight / s.dZCut) + 1;	// Z方向分割数（粗加工用）

	// 座標点の初期化
	if ( m_pPath ) {
		FreeCoord3(m_pPath, m_pPathX, m_pPathY);
	}
	m_pPathX = D+1;
	m_pPathY = s.nLineSplit+1;
	m_pPath = NewCoord3(m_pPathX, m_pPathY, 2000);

	// ガイドカーブに沿って垂直平面をシフトしていき，加工面との交点群を求めていく
	for ( i=0; i<=s.nLineSplit; i++ ) {
		double t = (double)i/s.nLineSplit;
		if ( i==0 ) {
			t += 0.0001;		// 特異点回避
		}
		else if ( i==s.nLineSplit ) {
			t -= 0.0001;		// 特異点回避
		}
		plane_pt = nf.CalcNurbsCCoord(nc, t);		// 注目中の垂直平面上の1点
		plane_n  = nf.CalcTanVecOnNurbsC(nc, t);	// 注目中の垂直平面の法線ベクトル
		ptnum[i] = nf.CalcIntersecPtsPlaneSearch(ns, plane_pt, plane_n, 0.5, 3, path_, 2000, RUNGE_KUTTA);  // 交点群算出
		// 得られた交点群を，加工面法線方向に工具半径分オフセットさせた点を得る
		for ( j=0; j<ptnum[i]; j++ ) {
			Coord pt = nf.CalcNurbsSCoord(ns, path_[j].x, path_[j].y);		// 工具コンタクト点
			Coord n = nf.CalcNormVecOnNurbsS(ns, path_[j].x, path_[j].y);	// 法線ベクトル
			if (n.z < 0) n = n*(-1);					// 法線ベクトルの向き調整
			m_pPath[D][i][j] = pt + n*s.dBallEndmill;	// 工具半径オフセット
		}
	}

	// 粗加工パス生成
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
