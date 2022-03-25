// TH_MakeNurbs.cpp
//		NURBS曲面用NC生成
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"		// NCMakeBase.h用
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

// ｸﾞﾛｰﾊﾞﾙ変数定義
static	CThreadDlg*		g_pParent;
static	C3dModelDoc*	g_pDoc;
static	CNCMakeMillOpt*	g_pMakeOpt;
static	MAKEMODE		g_enMode;
static	int				g_nMode;
static	float			g_dZoffset;		// ワークの高さを原点にするときのオフセット
static	int				g_nFase;

// よく使う変数や呼び出しの簡略置換
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)
#define	Get3dDbl(a)	g_pDoc->Get3dOption()->Get3dDbl(a)
#define	Get3dFlg(a)	g_pDoc->Get3dOption()->Get3dFlg(a)

// NC生成に必要なﾃﾞｰﾀ群
static	CTypedPtrArrayEx<CPtrArray, CNCMakeMill*>	g_obMakeData;	// 加工ﾃﾞｰﾀ

// サブ関数関数
static	void	InitialVariable(void);			// 変数初期化
static	void	SetStaticOption(void);			// 静的変数の初期化
static	BOOL	MakeNurbs_RoughFunc(void);		// 荒加工の生成ループ
static	BOOL	MakeNurbs_ContourFunc(void);	// 仕上げ等高線の生成ループ
static	BOOL	OutputNurbsCode(void);			// NCｺｰﾄﾞの出力
// 荒加工用サブ
static	tuple<ptrdiff_t, int>		SearchFirstPoint(const VVCoord&);
static	tuple<ptrdiff_t, int>		SearchNearEdge(const VVCoord&);
// 仕上げ等高線用サブ
static	tuple<ptrdiff_t, ptrdiff_t>	SearchNearGroup(const VVCoord&);
static	tuple<ptrdiff_t, double>	SearchNearPoint(const VCoord&);
static	void	MakeLoopCoord(VCoord&, size_t, int=-1);

// ﾍｯﾀﾞｰ,ﾌｯﾀﾞｰ等のｽﾍﾟｼｬﾙｺｰﾄﾞ生成
static	void	AddCustomNurbsCode(int);

// 任意ﾃﾞｰﾀの生成
static inline	void _AddMakeNurbsStr(const CString& strData)
{
	CNCMakeMill*	pNCD = new CNCMakeMill(strData);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// G00でZ軸移動
static inline	void _AddMoveG00Z(double z)
{
	CNCMakeMill*	pNCD = new CNCMakeMill(0, (float)z, 0.0f);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// G01でZ軸移動
static inline	void _AddMakeG01Zcut(double z)
{
	z -= g_dZoffset;
	CNCMakeMill* pNCD = new CNCMakeMill(1, (float)z, GetDbl(MKNC_DBL_ZFEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// ptまで移動し，pt.z+R点までZ軸下降
static inline	void _AddMovePoint(const CPoint3D& pt)
{
	CNCMakeMill*	pNCD;
	// 切削ポイントまで移動
	pNCD = new CNCMakeMill(0, pt.GetXY(), 0.0f);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	// pt.z+R点までG00Z軸移動
	_AddMoveG00Z(pt.z - g_dZoffset + GetDbl(MKNC_DBL_ZG0STOP));
}
// Coord座標の生成
static inline	void _AddMakeCoord(const Coord& c)
{
	CPoint3D		pt(c);
	pt.z -= g_dZoffset;
	CNCMakeMill* pNCD = new CNCMakeMill(pt, GetDbl(MKNC_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

// ｻﾌﾞｽﾚｯﾄﾞ関数
static	CCriticalSection	g_csMakeAfter;	// MakeNurbs_AfterThread()ｽﾚｯﾄﾞﾛｯｸｵﾌﾞｼﾞｪｸﾄ
static	UINT	MakeNurbs_AfterThread(LPVOID);	// 後始末ｽﾚｯﾄﾞ

//////////////////////////////////////////////////////////////////////
// NURBS曲面用NC生成ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT MakeNurbs_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeNurbs_Thread() Start\n");
#endif
	int		nResult = IDCANCEL;

	// ｸﾞﾛｰﾊﾞﾙ変数初期化
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc = static_cast<C3dModelDoc *>(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);

	// 準備中表示
	g_nFase = 0;
	SendFaseMessage(g_pParent, g_nFase, -1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;
	// 生成モードの決定
	if ( !g_pDoc->GetRoughCoord().empty() ) {
		g_enMode = ROUGH;
	}
	else if ( !g_pDoc->GetContourCoord().empty() ) {
		g_enMode = CONTOUR;
	}

	// 下位の CMemoryException は全てここで集約
	try {
		// NC生成ｵﾌﾟｼｮﾝｵﾌﾞｼﾞｪｸﾄの生成とｵﾌﾟｼｮﾝの読み込み
		g_pMakeOpt = new CNCMakeMillOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKENURBS)->GetHead());
		// NC生成のﾙｰﾌﾟ前に必要な初期化
		InitialVariable();
		// 条件ごとに変化するﾊﾟﾗﾒｰﾀを設定
		SetStaticOption();
		// 変数初期化ｽﾚｯﾄﾞの処理待ち
		g_csMakeAfter.Lock();		// ｽﾚｯﾄﾞ側でﾛｯｸ解除するまで待つ
		g_csMakeAfter.Unlock();
		// 増分割り当て
		g_obMakeData.SetSize(0, 2048);
		// 生成開始
		BOOL bResult = FALSE;
		switch ( g_enMode ) {
		case ROUGH:
			// 荒加工生成ループ
			bResult = MakeNurbs_RoughFunc();
			break;
		case CONTOUR:
			// 仕上げ生成ループ
			bResult = MakeNurbs_ContourFunc();
			break;
		}
		if ( bResult )
			bResult = OutputNurbsCode();

		// 戻り値ｾｯﾄ
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

	// 終了処理
	_dp.SetDecimal3();
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// このｽﾚｯﾄﾞからﾀﾞｲｱﾛｸﾞ終了

	// 生成したNCｺｰﾄﾞの消去ｽﾚｯﾄﾞ(優先度を下げる)
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
	// 座標値の生成
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

	// ABS, INC 関係なく G92値で初期化
	for ( int i=0; i<NCXYZ; i++ )
		CNCMakeMill::ms_xyz[i] = RoundUp(GetDbl(MKNC_DBL_G92X+i));

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化
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
//	荒加工生成ループ
//////////////////////////////////////////////////////////////////////

BOOL MakeNurbs_RoughFunc(void)
{
	VVVCoord::iterator	itv;
	VVVCoord&	vvv = g_pDoc->GetRoughCoord();
	INT_PTR		maxcnt = 0, cnt = 0;
	ptrdiff_t	idx;
	int			r;	// 0:正順, 1:逆順

	// Coord::dmy のクリア．生成済みフラグとして使用
	for ( itv=vvv.begin(); itv!=vvv.end(); ++itv ) {
		maxcnt += itv->size();			// 座標集合==処理数
		for ( auto it2=itv->begin(); it2!=itv->end(); ++it2 ) {
			for ( auto it3=it2->begin(); it3!=it2->end(); ++it3 ) {
				it3->dmy = 0.0;			// 生成済みフラグのクリア
			}
		}
	}

	// フェーズ更新
	SendFaseMessage(g_pParent, g_nFase, maxcnt);

	// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
	AddCustomNurbsCode(MKNC_STR_HEADER);

	// 最初の開始ポイントを検索
	// 最初だけ front と back からの検索
	itv = vvv.begin();
	tie(idx, r) = SearchFirstPoint(*itv);
	if ( idx < 0 ) {
		return FALSE;
	}
	MakeLoopCoord(itv->operator[](idx), 0, r);
	SetProgressPos(g_pParent, ++cnt);

	// スキャン座標の生成
	while ( IsThread() ) {
		tie(idx, r) = SearchNearEdge(*itv);
		if ( idx < 0 ) {
			if ( ++itv == vvv.end() ) {
				// パス生成ループ終了
				break;
			}
			continue;
		}
		MakeLoopCoord(itv->operator[](idx), 0, r);
		SetProgressPos(g_pParent, ++cnt);
	}

	// Z軸をイニシャル点に復帰
	_AddMoveG00Z(GetDbl(MKNC_DBL_G92Z));

	// Gｺｰﾄﾞﾌｯﾀﾞ(終了ｺｰﾄﾞ)
	AddCustomNurbsCode(MKNC_STR_FOOTER);

	return IsThread();
}

//////////////////////////////////////////////////////////////////////
//	仕上げ等高線の生成ループ
//////////////////////////////////////////////////////////////////////

BOOL MakeNurbs_ContourFunc(void)
{
	VVVCoord::iterator	itv;
	VVVCoord&	vvv = g_pDoc->GetContourCoord();
	std::vector<size_t>		vLayer;		// 階層ごとの残グループ数
	size_t		layer = 0;				// 現在処理中の階層
	INT_PTR		maxcnt = 0, cnt = 0;
	ptrdiff_t	grp, idx;
	CPointD		pt;

	// Coord::dmy のクリア．生成済みフラグとして使用
	for ( itv=vvv.begin(); itv!=vvv.end(); ++itv ) {
		maxcnt += itv->size();			// 座標集合==処理数
		vLayer.push_back(itv->size());	// 階層ごとの座標グループ数で初期化
		for ( auto it2=itv->begin(); it2!=itv->end(); ++it2 ) {
			for ( auto it3=it2->begin(); it3!=it2->end(); ++it3 ) {
				it3->dmy = 0.0;			// 生成済みフラグのクリア
			}
		}
	}
	
	// フェーズ更新
	SendFaseMessage(g_pParent, g_nFase, maxcnt);

	// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
	AddCustomNurbsCode(MKNC_STR_HEADER);

	// 最初の階層から開始位置を検索
	// エラーで最初の階層に有効なデータがない場合があるので
	// データ生成できるところまでループ
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

	// パス生成ループ
	while ( IsThread() ) {
		SetProgressPos(g_pParent, ++cnt);
		// 次の階層へ
		layer++;
		if ( layer >= vvv.size() ) {
			// まだ残っている階層から
			auto f = find_if(vLayer, lambda::_1>0);
			if ( f == vLayer.end() ) {
				// パス生成ループ終了
				break;
			}
			layer = std::distance(vLayer.begin(), f);
		}
		// この階層で近い座標グループを検索
		tie(grp, idx) = SearchNearGroup(vvv[layer]);
		if ( grp < 0 ) {
			ASSERT(TRUE);	// ないはずがない
			break;
		}
		//
		pt.x = vvv[layer][grp][idx].x - CNCMakeMill::ms_xyz[NCA_X];
		pt.y = vvv[layer][grp][idx].y - CNCMakeMill::ms_xyz[NCA_Y];
		if ( hypot(pt.x, pt.y) > Get3dDbl(D3_DBL_CONTOUR_SHIFT)*2.5 ) {
			// 階層シフト量*2.5より大きい移動の場合は
			// R点まで上昇（衝突回避）
			_AddMoveG00Z(GetDbl(MKNC_DBL_ZG0STOP));
		}
		else {
			// 最小の移動で
			// 現在位置＋R点だけ上昇
			_AddMoveG00Z(CNCMakeMill::ms_xyz[NCA_Z] + GetDbl(MKNC_DBL_ZG0STOP));
		}
		// この階層で生成
		MakeLoopCoord(vvv[layer][grp], idx);
		// この階層のグループ数を減らす
		vLayer[layer]--;
	}

	// Z軸をイニシャル点に復帰
	_AddMoveG00Z(GetDbl(MKNC_DBL_G92Z));

	// Gｺｰﾄﾞﾌｯﾀﾞ(終了ｺｰﾄﾞ)
	AddCustomNurbsCode(MKNC_STR_FOOTER);

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

tuple<ptrdiff_t, int>	SearchFirstPoint(const VVCoord& vv)
{
	ptrdiff_t	idx;
	int			r;
	// 最初と最後の点
	CPoint3D	pt0( vv.front().front() ),
				pt1( vv.front().back()  ),
				pt2( vv.back().front()  ),
				pt3( vv.back().back()   );
	CPointD		ptNow(CNCMakeMill::ms_xyz[NCA_X], CNCMakeMill::ms_xyz[NCA_Y]);
	pt0 -= ptNow;
	pt1 -= ptNow;
	pt2 -= ptNow;
	pt3 -= ptNow;
	// 距離計算
	// 固定長配列なので vector ではなく array
	boost::array<double, 4>	v = {
		pt0.x*pt0.x + pt0.y*pt0.y,
		pt1.x*pt1.x + pt1.y*pt1.y,
		pt2.x*pt2.x + pt2.y*pt2.y,
		pt3.x*pt3.x + pt3.y*pt3.y
	};
	// 一番小さい要素
	auto	it  = boost::range::min_element(v);
	switch ( std::distance(v.begin(), it) ) {
	case 0:		// pt0
		idx = 0;
		r = 0;		// 正順
		break;
	case 1:		// pt1
		idx = 0;
		r = 1;		// 逆順
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
		if ( it->dmy > 0 ) continue;	// 生成済み
		pt  = *it;
		pt -= ptNow;					// 現在位置との差
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
		// 関数側で閉じた集合か判断（仕上げパス用）
		ptF = v.front();
		ptB = v.back();
		if ( ptF.hypot(&ptB) < Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0 ) {
			// 開始点に移動
			pt = v[idx];
			_AddMovePoint(pt);
			// そこまで下降
			_AddMakeG01Zcut(pt.z);
			// idxから順に生成
			size_t	i;
			for ( i=idx; i<v.size(); i++ ) {
				_AddMakeCoord(v[i]);
				v[i].dmy = 1.0;
			}
			for ( i=0; i<idx; i++ ) {
				_AddMakeCoord(v[i]);
				v[i].dmy = 1.0;
			}
			// 開始点までつなぐ
			_AddMakeCoord(v[idx]);
			// 生成終了
			return;
		}
	}

	if ( r < 0 ) {
		// 関数側でどちらが近いか判断
		CPointD	ptNow(CNCMakeMill::ms_xyz[NCA_X], CNCMakeMill::ms_xyz[NCA_Y]);
		ptF -= ptNow;
		ptB -= ptNow;
		if ( ptF.x*ptF.x+ptF.y*ptF.y < ptB.x*ptB.x+ptB.y*ptB.y ) {
			r = 0;	// 正順
		}
		else {
			r = 1;	// 逆順
		}
	}

	// 端点から順に生成
	if ( r == 0 ) {
		// 正順
		pt = v.front();
		_AddMovePoint(pt);
		_AddMakeG01Zcut(pt.z);
		BOOST_FOREACH(Coord& c, v) {
			_AddMakeCoord(c);
			c.dmy = 1.0;
		}
	}
	else {
		// 逆順
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

//	AddCustomNurbsCode() から呼び出し
class CMakeCustomNurbsCode : public CMakeCustomCode	// MakeCustomCode.h
{
	BOOL	m_bComment;		// Endmill等のコメントを挿入したかどうか

public:
	CMakeCustomNurbsCode(int n) :
				CMakeCustomCode(g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "G92_INITIAL", "G92X", "G92Y", "G92Z",
			"SPINDLE", "G0XY_INITIAL"
		};
		// ｵｰﾀﾞｰ追加
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
		// コメント挿入初期化
		m_bComment = n==MKNC_STR_HEADER ? FALSE : TRUE;	// Header以外には入れない
	}

	CString	ReplaceCustomCode(const string& str) {
		extern	LPCTSTR	gg_szReturn;		// "\n";
		extern	const	DWORD	g_dwSetValFlags[];
		int		nTestCode;
		float	dValue[VALUESIZE];
		CString	strResult;

		// 基底ｸﾗｽ呼び出し
		tie(nTestCode, strResult) = CMakeCustomCode::ReplaceCustomCode(str);
		if ( !strResult.IsEmpty() )
			return strResult;	// 置換済みなら戻る（ここでGコードは来ない）

		// 派生replace
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
				// '%'の次の行にコメントを挿入
				CString	strBuf;
				strBuf.Format(IDS_MAKENCD_ENDMILL, Get3dDbl(D3_DBL_ROUGH_BALLENDMILL));
				strResult += gg_szReturn + strBuf;
				m_bComment = TRUE;
			}
		}

		if ( strResult[0]=='G' && !m_bComment ) {
			// 最初のGコードの前にコメントを挿入
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
		// このｴﾗｰは正常ﾘﾀｰﾝ(警告のみ)
	}
}

//////////////////////////////////////////////////////////////////////

// NC生成のｸﾞﾛｰﾊﾞﾙ変数初期化(後始末)ｽﾚｯﾄﾞ
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
