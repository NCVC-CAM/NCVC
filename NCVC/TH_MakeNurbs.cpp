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
static	tuple<int, int>			MoveFirstPoint(int);		// 最初のCoordポイントを検索
// 仕上げ等高線用サブ
static	tuple<int, double>	SearchNearPoint(VCoord&);

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
	if ( g_pDoc->GetRoughCoord() ) {
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
	int		mx, my, mz, i, j, k,
			fx, fy;
	Coord***	pRoughCoord = g_pDoc->GetRoughCoord();
	tie(mx, my) = g_pDoc->GetRoughNumXY();

	// フェーズ更新
	SendFaseMessage(g_pParent, g_nFase, mx*my);

	// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
	AddCustomNurbsCode(MKNC_STR_HEADER);

	// 開始点の検索と移動
	tie(fx, fy) = MoveFirstPoint(my);

	// スキャン座標の生成（記述が冗長なのでなんとかしたい）
	for ( i=0; i<mx && IsThread(); i++ ) {				// Zの階層
		if ( fy == 0 ) {
			for ( j=0; j<my && IsThread(); j++ ) {		// スキャン分割数
				mz = g_pDoc->GetRoughNumZ(j);
				if ( fx == 0 ) {
					k = 0;
					_AddMakeG01Zcut(pRoughCoord[i][j][k].z);	// Z軸の下降
					for ( ; k<mz && IsThread(); k++ ) {
						_AddMakeCoord(pRoughCoord[i][j][k]);	// Coord座標の生成
					}
				}
				else {
					k = mz - 1;
					_AddMakeG01Zcut(pRoughCoord[i][j][k].z);
					for ( ; k>=0 && IsThread(); k-- ) {
						_AddMakeCoord(pRoughCoord[i][j][k]);
					}
				}
				SetProgressPos(g_pParent, i*my+j);
				fx = 1 - fx;
			}
		}
		else {
			for ( j=my-1; j>=0 && IsThread(); j-- ) {
				mz = g_pDoc->GetRoughNumZ(j);
				if ( fx == 0 ) {
					k = 0;
					_AddMakeG01Zcut(pRoughCoord[i][j][k].z);
					for ( ; k<mz && IsThread(); k++ ) {
						_AddMakeCoord(pRoughCoord[i][j][k]);
					}
				}
				else {
					k = mz - 1;
					_AddMakeG01Zcut(pRoughCoord[i][j][k].z);
					for ( ; k>=0 && IsThread(); k-- ) {
						_AddMakeCoord(pRoughCoord[i][j][k]);
					}
				}
				SetProgressPos(g_pParent, i*my+my-j);
				fx = 1 - fx;
			}
		}
		fy = 1 - fy;
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
	std::vector<VCoord>&	vv = g_pDoc->GetContourCoord();
	int			idx, cnt = 0;
	double		dGap;

	// Coord::dmy のクリア．生成済みフラグとして使用
	for ( auto it1=vv.begin(); it1!=vv.end(); ++it1 ) {
		for ( auto it2=it1->begin(); it2!=it1->end(); ++it2 ) {
			(*it2).dmy = 0.0;
		}
	}
	
	// フェーズ更新
	SendFaseMessage(g_pParent, g_nFase, vv.size());

	// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
	AddCustomNurbsCode(MKNC_STR_HEADER);

	// 階層ごとにパス生成
	for ( auto it=vv.begin(); it!=vv.end() && IsThread(); ++it ) {
		// この階層が閉ループかどうかの判定


		// この階層で全て生成するまで繰り返し
		// 　にこだわらないほうがいい．近い座標から切削する方が効率いいはず
		idx = 0;
		while ( IsThread() ) {
			// 現在位置(x, y)に最も近い座標を検索
			tie(idx, dGap) = SearchNearPoint(*it);
			if ( idx < 0 ) {
				// R点まで上昇
				_AddMoveG00Z(GetDbl(MKNC_DBL_ZG0STOP));
				// この階層での処理終了
				break;
			}
			// dGapが規定値以下かどうか
			if ( sqrt(dGap) <= Get3dDbl(D3_DBL_CONTOUR_SPACE)*2.0f ) {	// *2.0fはマージン
				// 点間隔以下ならそのまま生成
				_AddMakeCoord( (*it)[idx] );
			}
			else {
				// 一旦R点まで上昇
				_AddMoveG00Z(GetDbl(MKNC_DBL_ZG0STOP));
				// 次の切削ポイントまで移動
				CPoint3D	pt( (*it)[idx] );
				_AddMovePoint(pt);
				// そこまで下降
				_AddMakeG01Zcut(pt.z);
			}
			// 生成済みマーク
			(*it)[idx].dmy = 1.0;
		}
		SetProgressPos(g_pParent, ++cnt);
	}

	// Z軸をイニシャル点に復帰
	_AddMoveG00Z(GetDbl(MKNC_DBL_G92Z));

	// Gｺｰﾄﾞﾌｯﾀﾞ(終了ｺｰﾄﾞ)
	AddCustomNurbsCode(MKNC_STR_FOOTER);

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

tuple<int, int>	MoveFirstPoint(int my)
{
	int		fx = 0, fy = 0;
	Coord***	pRoughCoord = g_pDoc->GetRoughCoord();

	// 4角の座標
	CPoint3D	pt0(pRoughCoord[0][0][0]),
				pt1(pRoughCoord[0][0][g_pDoc->GetRoughNumZ(0)-1]),
				pt2(pRoughCoord[0][my-1][0]),
				pt3(pRoughCoord[0][my-1][g_pDoc->GetRoughNumZ(my-1)-1]);
	// 距離計算(sqrt()いらないけど4点くらいなら問題なし)
	std::vector<double>		v;
	v.push_back( pt0.hypot() );
	v.push_back( pt1.hypot() );
	v.push_back( pt2.hypot() );
	v.push_back( pt3.hypot() );
	// 一番小さい要素
	auto	it  = std::min_element(v.begin(), v.end());
	switch ( std::distance(v.begin(), it) ) {
	case 0:		// pt0
		_AddMovePoint(pt0);	// 開始点まで移動
		break;
	case 1:		// pt1
		_AddMovePoint(pt1);
		fx = 1;			// 横方向反転
		break;
	case 2:		// pt2
		_AddMovePoint(pt2);
		fy = 1;			// 縦方向反転
		break;
	case 3:		// pt3
		_AddMovePoint(pt3);
		fx = fy = 1;	// 横も縦も反転
		break;
	}

	return make_tuple(fx, fy);
}

tuple<int, double> SearchNearPoint(VCoord& v)
{
	CPointF	ptNow(CNCMakeMill::ms_xyz[NCA_X], CNCMakeMill::ms_xyz[NCA_Y]);
	CPointD	pt;
	double	dGap, dGapMin = HUGE_VAL;
	int		i, minID = -1;

	// イテレータでやるとややこしい
	for ( i=0; i<v.size(); i++ ) {
		if ( v[i].dmy > 0 ) continue;	// 生成済み
		pt.SetPoint( v[i].x-ptNow.x, v[i].y-ptNow.y );	// 現在位置との差
		dGap = pt.x*pt.x + pt.y*pt.y;	// hypot()は使わない sqrt()が遅い
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			minID = i;
		}
	}

	return make_tuple(minID, dGapMin);
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
