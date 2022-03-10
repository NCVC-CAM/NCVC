// 3dModelDoc.cpp : 実装ファイル
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
	ClearRoughPath();
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

	// 同一フォルダにあるオプションファイルの読み込み
	m_3dOpt.Read3dOption(lpszPathName);

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
#ifdef _DEBUG
	printf("m_rcMax=(%f, %f)-(%f, %f) h=%f l=%f\n",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom,
		m_rcMax.high, m_rcMax.low);
#endif

	return TRUE;
}

void C3dModelDoc::OnCloseDocument() 
{
	// 処理中のｽﾚｯﾄﾞを中断させる
	OnCloseDocumentBase();		// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ

	__super::OnCloseDocument();
}

void C3dModelDoc::OnUpdateFile3dMake(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( m_pRoughCoord != NULL );
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

	// 設定の保存
	if ( !strInit.IsEmpty() ) {
		pOpt->AddInitHistory(NCMAKENURBS, strInit);
	}
	pOpt->SetViewFlag(bNCView);

	// すでに開いているﾄﾞｷｭﾒﾝﾄなら閉じる
	CDocument* pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_NCD, m_strNCFileName);
	if ( pDoc ) {
		pDoc->OnCloseDocument();
	}

	// 生成開始
	CThreadDlg*	pDlg = new CThreadDlg(ID_FILE_3DARA, this);
	INT_PTR		nResult = pDlg->DoModal();
	delete	pDlg;

	// NC生成後のﾃﾞｰﾀを開く
	if ( nResult==IDOK && bNCView ) {
		AfxGetNCVCApp()->OpenDocumentFile(m_strNCFileName);
	}
}

/////////////////////////////////////////////////////////////////////////////

void C3dModelDoc::ClearRoughPath(void)
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

BOOL C3dModelDoc::MakeRoughPath(NURBSS* ns, NURBSC* nc)
{
	// Kodatuno User's Guide いいかげんな3xCAMの作成
	NURBS_Func	nf;				// NURBS_Funcへのインスタンス
	Coord		plane_pt;		// 分割する平面上の1点
	Coord		plane_n;		// 分割する平面の法線ベクトル
	Coord		path_[2000];	// 一時格納用バッファ
	int		i, j, k,
			D = (int)(m_3dOpt.Get3dDbl(D3_DBL_HEIGHT) / m_3dOpt.Get3dDbl(D3_DBL_ZCUT)) + 1,	// Z方向分割数（粗加工用）
			N = m_3dOpt.Get3dInt(D3_INT_LINESPLIT);					// スキャニングライン分割数(N < 100)
	BOOL	bResult = TRUE;

	try {
		// 座標点の初期化
		ClearRoughPath();
		m_nRoughX = D+1;
		m_nRoughY = N+1;
		m_pRoughCoord = NewCoord3(m_nRoughX, m_nRoughY, 2000);
		m_pRoughNum = new int[100];

		// ガイドカーブに沿って垂直平面をシフトしていき，加工面との交点群を求めていく
		for ( i=0; i<=N; i++ ) {
			double t = (double)i/N;
			if ( i==0 ) {
				t += 0.0001;		// 特異点回避
			}
			else if ( i==N ) {
				t -= 0.0001;		// 特異点回避
			}
			plane_pt = nf.CalcNurbsCCoord(nc, t);		// 注目中の垂直平面上の1点
			plane_n  = nf.CalcTanVecOnNurbsC(nc, t);	// 注目中の垂直平面の法線ベクトル
			m_pRoughNum[i] = nf.CalcIntersecPtsPlaneSearch(ns, plane_pt, plane_n, 0.5, 3, path_, 2000, RUNGE_KUTTA);  // 交点群算出
			// 得られた交点群を，加工面法線方向に工具半径分オフセットさせた点を得る
			for ( j=0; j<m_pRoughNum[i]; j++ ) {
				Coord pt = nf.CalcNurbsSCoord(ns, path_[j].x, path_[j].y);		// 工具コンタクト点
				Coord n = nf.CalcNormVecOnNurbsS(ns, path_[j].x, path_[j].y);	// 法線ベクトル
				if (n.z < 0) n = n*(-1);					// 法線ベクトルの向き調整
				m_pRoughCoord[D][i][j] = pt + n*m_3dOpt.Get3dDbl(D3_DBL_BALLENDMILL);	// 工具半径オフセット
			}
		}

		// 粗加工パス生成
		for ( i=0; i<D; i++ ) {
			for ( j=0; j<m_nRoughY; j++ ) {
				for ( k=0; k<m_pRoughNum[j]; k++ ) {
					double del = (m_3dOpt.Get3dDbl(D3_DBL_HEIGHT) - m_pRoughCoord[D][j][k].z)/(double)D;
					double Z = m_3dOpt.Get3dDbl(D3_DBL_HEIGHT) - del*i;
					m_pRoughCoord[i][j][k] = SetCoord(m_pRoughCoord[D][j][k].x, m_pRoughCoord[D][j][k].y, Z);
				}
			}
		}
	}
	catch(...) {
		// ライブラリ側の例外に対応
		ClearRoughPath();
		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

	// スキャンオプションの保存
	if ( bResult )
		m_3dOpt.Save3dOption();

	return bResult;
}
