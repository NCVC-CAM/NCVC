// 3dModelDoc.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dModelDoc.h"
#include "MakeNCDlg.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#define	_DEBUG_FILEOUT_
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
	CThreadDlg*	pDlg = new CThreadDlg(ID_FILE_3DROUGH, this);
	INT_PTR		nResult = pDlg->DoModal();
	delete	pDlg;

	// NC生成後のﾃﾞｰﾀを開く
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
	// Kodatuno User's Guide いいかげんな3xCAMの作成
	NURBS_Func	nf;			// NURBS_Funcへのインスタンス
	Coord	plane_pt;		// 分割する平面上の1点
	Coord	plane_n;		// 分割する平面の法線ベクトル
	Coord	path_[2000];	// 一時格納用バッファ
	int		i, j, k,
			D = (int)(m_3dOpt.Get3dDbl(D3_DBL_WORKHEIGHT) / m_3dOpt.Get3dDbl(D3_DBL_ROUGH_ZCUT)) + 1,	// Z方向分割数（粗加工用）
			N = m_3dOpt.Get3dInt(D3_INT_LINESPLIT);					// スキャニングライン分割数(N < 100)
	BOOL	bResult = TRUE;

	try {
		// 座標点の初期化
		ClearRoughCoord();
		ClearContourCoord();
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
				m_pRoughCoord[D][i][j] = pt + n*m_3dOpt.Get3dDbl(D3_DBL_ROUGH_BALLENDMILL);	// 工具半径オフセット
			}
		}

		// 粗加工パス生成
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
		// ライブラリ側の例外に対応
		ClearRoughCoord();
		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

	// スキャンオプションの保存
	if ( bResult )
		m_3dOpt.Save3dOption();

	return bResult;
}

BOOL C3dModelDoc::MakeContourCoord(NURBSS* ns)
{
	// Kodatuno User's Guide 等高線を生成する
	NURBS_Func	nf;		// NURBSを扱う関数集を呼び出す
	VCoord	v;			// 1平面の交点群
	Coord	pt, p,
			t[5000],	// 解の格納
			nvec = SetCoord(0.0, 0.0, 1.0);	// 平面の法線ベクトル（XY平面）
	double	dSpace = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SPACE),	// 交点群の点間隔
			dZmin  = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_ZMIN),		// 等高線のZmin
			dZmax  = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_ZMAX),		// 等高線のZmax
			dShift = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SHIFT);	// 等高線生成のZ間隔
	int		i, j, num,
			step = (int)(fabs(dZmax - dZmin) / dShift + 1);		// 等高線の本数
	BOOL	bResult = TRUE;

	try {
		// 座標点の初期化
		ClearRoughCoord();
		ClearContourCoord();

		// 平面をZ方向にシフトしていきながら等高線を算出する
		for ( i=0; i<step; i++ ) {
			pt = SetCoord(0.0, 0.0, dZmax - dShift*i);	// 現在の平面のZ位置の1点を指示（上面から計算）
			num = nf.CalcIntersecPtsPlaneSearch(ns, pt, nvec, dSpace, 5, t, 5000, RUNGE_KUTTA);		// NURBS曲面と平面との交点群を交線追跡法で求める
			for ( j=0; j<num; j++ ) {
				p = nf.CalcNurbsSCoord(ns, t[j].x, t[j].y);		// 交点をパラメータ値から座標値へ変換
				p.dmy = 0.0;
				v.push_back(p);
			}
			if ( !v.empty() ) {
#ifdef _DEBUG_FILEOUT_
				DumpCoord(v);	// デバッグ用の座標出力
#endif
				// 1平面の座標をグループ集合で登録
				SetCoordGroup(v);
				v.clear();
			}
		}
	}
	catch(...) {
		// ライブラリ側の例外に対応
		ClearRoughCoord();
		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

#ifdef _DEBUG
	printf("階層=%zd\n", m_vvvContourCoord.size());
	for ( auto it1=m_vvvContourCoord.begin(); it1!=m_vvvContourCoord.end(); ++it1 ) {
		printf(" 集合%Id=%zd\n", std::distance(m_vvvContourCoord.begin(), it1), it1->size());
		for ( auto it2=it1->begin(); it2!=it1->end(); ++it2 ) {
			// 閉ループか否かの判定
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

	// スキャンオプションの保存
	if ( bResult )
		m_3dOpt.Save3dOption();

	return bResult;
}

void C3dModelDoc::SetCoordGroup(VCoord& v)
{
	ptrdiff_t	grp, idx;
	double		dGap, dMargin = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0;
	VCoord		vtmp;
	VVCoord		vv;

	// 最初の検索ポイント
	vtmp.push_back(v.front());
	vv.push_back(vtmp);
	grp = 0;	// 現在処理中のグループID
	CPointD	ptNow(v.front()), pt;
	v.front().dmy = 1.0;

	while ( TRUE ) {
		boost::tie(idx, dGap) = SearchNearPoint(v, ptNow);
		if ( idx < 0 ) {
			break;	// ループ終了条件
		}
		else if ( sqrt(dGap) < dMargin ) {
			// 同一グループ
			vv[grp].push_back(v[idx]);
		}
		else {
			// 近いグループを検索
			grp = SearchNearGroup(vv, ptNow, grp);
			if ( grp < 0 ) {
				// 新規グループ作成
				vtmp.clear();
				vtmp.push_back(v[idx]);
				vv.push_back(vtmp);
				grp = vv.size() - 1;
			}
			else {
				vv[grp].push_back(v[idx]);
			}
		}
		// 検索済みマーク
		v[idx].dmy = 1.0;
		// 現在位置更新
		ptNow = v[idx];
	}

	// 1平面の交点群を保存
	m_vvvContourCoord.push_back(vv);
}

boost::tuple<ptrdiff_t, double> C3dModelDoc::SearchNearPoint(const VCoord& v, const CPointD& ptNow)
{
	CPointD		pt;
	double		dGap, dGapMin = HUGE_VAL;
	ptrdiff_t	minID = -1;

	for ( auto it=v.begin(); it!=v.end(); ++it ) {
		if ( it->dmy > 0 ) continue;	// 処理対象済み
		pt  = *it;
		pt -= ptNow;	// 現在位置との差
		dGap = pt.x*pt.x + pt.y*pt.y;	// hypot()は使わない sqrt()が遅い
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			minID = std::distance(v.begin(), it);
		}
	}

	return boost::make_tuple(minID, dGapMin);
}

ptrdiff_t C3dModelDoc::SearchNearGroup(VVCoord& vv, const CPointD& ptNow, ptrdiff_t grpNow)
{
	double		dMargin = m_3dOpt.Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0;
	CPointD		pt;
	ptrdiff_t	grp = -1;

	for ( auto it=vv.begin(); it!=vv.end(); ++it ) {
		if ( grpNow == std::distance(vv.begin(), it) ) continue;
		pt  = it->front();
		pt -= ptNow;
		if ( pt.hypot() < dMargin ) {
			// このグループの先頭に近い場合
			boost::range::reverse(*it);
			grp = std::distance(vv.begin(), it);
			break;
		}
		else {
			pt  = it->back();
			pt -= ptNow;
			if ( pt.hypot() < dMargin ) {
				// このグループの末尾に近い場合
				grp = std::distance(vv.begin(), it);
				break;
			}
		}
	}

	return grp;
}

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void C3dModelDoc::DumpCoord(const VCoord& v)
{
	CString	file, s;
	file.Format("C:\\Users\\magara\\Documents\\tmp\\coord%d.csv", m_vvvContourCoord.size());
	CStdioFile	f(file, CFile::typeText|CFile::modeCreate|CFile::modeWrite);

	for ( auto it=v.begin(); it!=v.end(); ++it ) {
		s.Format("%.3f, %.3f, %.3f\n", it->x, it->y, it->z);
		f.WriteString(s);
	}
}
#endif
