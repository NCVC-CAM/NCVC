// TH_NCRead.cpp
// �m�b�R�[�h�̓����ϊ�
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "MCOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;
using namespace boost::spirit;

#define	IsThread()	g_pParent->IsThreadContinue()
//
static	CThreadDlg*	g_pParent;
static	CNCDoc*		g_pDoc;
static	NCARGV		g_ncArgv;		// NCVCdefine.h
static	DWORD		g_dwValFlags;	// ���W�ȊO�̒l�w���׸�
static	int			g_nSubprog;		// �����۸��ьĂяo���̊K�w
static	string		g_strComma;		// ��ψȍ~�̕�����
// �Œ軲�ق�Ӱ��ٕ�Ԓl
typedef	struct	tagCYCLE {
	BOOL	bCycle;		// �Œ軲�ُ�����
	double	dVal[3];	// Z, R, P
} CYCLE_INTERPOLATE;
static	CYCLE_INTERPOLATE	g_Cycle;
static	void	CycleInterpolate(void);

//
static	void		InitialVariable(void);
// ���l�ϊ�( 1/1000 �ް��𔻒f����)
inline	double		GetNCValue(const string& str)
{
	double	dResult = atof(str.c_str());
	if ( str.find('.') == string::npos )
		dResult /= 1000.0;		// �����_���Ȃ���� 1/1000
	return dResult;
}
// G00�`G03, G04, G10, G52, G8x, G92
// ��޼ު�Đ�������f��������
static	enum	ENGCODEOBJ {NOOBJ, MAKEOBJ, MAKEOBJ_NOTMODAL};
inline	ENGCODEOBJ	IsGcodeObject(int nCode)
{
	ENGCODEOBJ	enResult;

	if ( (0<=nCode && nCode<=3) || nCode==52 || nCode==92 ) {
		g_Cycle.bCycle = FALSE;
		enResult = MAKEOBJ;
	}
	else if ( nCode==4 || nCode==10 ) {
		enResult = MAKEOBJ_NOTMODAL;
	}
	else if ( nCode>=81 && nCode<=89 ) {
		g_Cycle.bCycle = TRUE;
		enResult = MAKEOBJ;
	}
	else
		enResult = NOOBJ;

	return enResult;
}

// ��͊֐�
static	int		NC_GSeparater(int, CNCdata*&);
static	void	CheckGcodeOther(int);
static	int		NC_SearchSubProgram(int*);
typedef	int		(*PFNSEARCHMACRO)(CNCblock*);
static	int		NC_SearchMacroProgram(CNCblock*);
static	int		NC_NoSearchMacro(CNCblock*);
static	PFNSEARCHMACRO	g_pfnSearchMacro;
static	CNCdata*	MakeChamferingObject(CNCblock*, CNCdata*, CNCdata*);
// F���Ұ�, �޳�َ��Ԃ̉���
typedef double (*PFNFEEDANALYZE)(const string&);
static	double	FeedAnalyze_Dot(const string&);
static	double	FeedAnalyze_Int(const string&);
static	PFNFEEDANALYZE	g_pfnFeedAnalyze;
// �����ہCϸۂ̌���
static	CString	g_strSearchFolder[2];	// ���ĂƎw��̫���
static	CString	SearchFolder(regex&);
static	CString	SearchFolder_Sub(int, LPCTSTR, regex&);
static	BOOL	SearchProgNo(LPCTSTR, regex&);

//////////////////////////////////////////////////////////////////////
//	NC���ނ̵�޼ު�Đ����گ��
//////////////////////////////////////////////////////////////////////

UINT NCDtoXYZ_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("NCDtoXYZ_Thread()\nStart", DBG_RED);
	CMagaDbg	dbg1(DBG_BLUE);
#endif

	int			i, nLoopCnt,
				nResult = IDOK;
	CNCdata*	pDataFirst = NULL;
	CNCdata*	pData;	// �P�O�̐�����޼ު��

	// �ϐ�������
	LPNCVCTHREADPARAM	pParam = (LPNCVCTHREADPARAM)pVoid;
	g_pParent = pParam->pParent;
	g_pDoc = (CNCDoc *)(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);
	InitialVariable();

	{
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_READ_NCD));
		g_pParent->SetFaseMessage(strMsg);
	}
	nLoopCnt = g_pDoc->GetNCBlockSize();
	g_pParent->m_ctReadProgress.SetRange32(0, nLoopCnt);
#ifdef _DEBUG
	dbg.printf("LoopCount=%d", nLoopCnt);
#endif

	try {
		// �P�O�̵�޼ު�ĎQ�Ƃ�NULL�Q�Ƃ��Ȃ�����
		pDataFirst = pData = new CNCdata(&g_ncArgv);
		// 1�s(1��ۯ�)��͂���޼ު�Ă̓o�^
		for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
			if ( NC_GSeparater(i, pData) != 0 )
				break;
			if ( (i & 0x003f) == 0 )	// 64�񂨂�(����6�ޯ�Ͻ�)
				g_pParent->m_ctReadProgress.SetPos(i);		// ��۸�ڽ�ް
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	// ����o�^�p��а�ް��̏���
	if ( pDataFirst )
		delete	pDataFirst;

	g_pParent->m_ctReadProgress.SetPos(nLoopCnt);
	g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
// G���ނ̍\�����

struct CGcode : grammar<CGcode>
{
	vector<string>&	vResult;
	CGcode(vector<string>& v) : vResult(v) {}

	template<typename T>
	struct definition
	{
		typedef	rule<T>	rule_t;
		rule_t	rr;
		definition( const CGcode& a )
		{
			// ���m���ނɂ��Ή����邽�߁u�S�Ẳp�啶���ɑ������l�v�ŉ��
			rr = +( upper_p >> real_p )[append(a.vResult)]
				>> !( ch_p(',') >> ((ch_p('R')|'C') >> real_p) )[append(a.vResult)];
		}
		const rule_t& start() const { return rr; }
	};
};

//////////////////////////////////////////////////////////////////////
// G���ނ̕���(�ċA�֐�)
int NC_GSeparater(int i, CNCdata*& pDataResult)
{
	extern	LPCTSTR			g_szNdelimiter; // "XYZRIJKPLDH"
	extern	const	DWORD	g_dwSetValFlags[];
	vector<string>	vResult;
	vector<string>::iterator	it;
	CGcode		a(vResult);
	int			ii, nCode, nNotModalCode = -1,
				nLoopCnt = g_pDoc->GetNCBlockSize(),
				nResult, nIndex, nRepeat;
	BOOL		bNCobj = FALSE, bNCval = FALSE, bNCsub = FALSE;
	ENGCODEOBJ	enGcode;
	string		strComma;
	CNCdata*	pData = NULL;
	CNCblock*	pBlock = g_pDoc->GetNCblock(i);
	// �ϐ�������
	g_ncArgv.nc.nLine		= i;
	g_ncArgv.nc.nErrorCode	= 0;
	g_ncArgv.nc.dwValFlags	&= 0xFFFF0000;
#ifdef _DEBUG
	CMagaDbg	dbg("NC_Gseparate()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_GREEN);
	dbg.printf("No.%004d Line=%s", i+1, g_pDoc->GetNCblock(i)->GetStrGcode());
#endif

	// ϸےu�����
	if ( (nIndex=(*g_pfnSearchMacro)(pBlock)) >= 0 ) {
		g_nSubprog++;
		for ( ii=nIndex; ii<nLoopCnt && IsThread(); ii++ ) {
			nResult = NC_GSeparater(ii, pDataResult);	// �ċA
			if ( nResult == 30 )
				return 30;
			else if ( nResult == 99 )
				break;
		}
		// EOF�ŏI���Ȃ�M99���A����
		if ( ii >= nLoopCnt && nResult == 0 ) {
			if ( g_nSubprog > 0 )
				g_nSubprog--;
		}
		return 0;
	}

	// G���ލ\�����
	if ( !parse( (LPCTSTR)(pBlock->GetStrGcode()),
					a, space_p|comment_p('(', ')') ).full ||
			vResult.empty() )
		return 0;

	for ( it=vResult.begin(); it!=vResult.end() && IsThread(); it++ ) {
#ifdef _DEBUG
		dbg1.printf("G Cut=%s", it->c_str());
#endif
		switch ( it->at(0) ) {
		case 'M':
			nCode = atoi(it->substr(1).c_str());
			switch ( nCode ) {
			case 2:
			case 30:
				return 30;
			case 98:
				// 5�K�w�ȏ�̌Ăяo���ʹװ(4�K�w�܂�)
				if ( g_nSubprog+1 >= 5 ) {
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98L);
					continue;
				}
				bNCsub = TRUE;
				break;
			case 99:
				if ( g_nSubprog > 0 )
					g_nSubprog--;
				return 99;
			}
			break;
		case 'G':
			nCode = atoi(it->substr(1).c_str());
			enGcode = IsGcodeObject(nCode);
			if ( enGcode != NOOBJ ) {
				// �O��̺��ނœo�^��޼ު�Ă�����Ȃ�
				if ( bNCobj ) {
					// ���W�l�������ĵ�޼ު�Đ���
					if ( !(g_ncArgv.nc.dwValFlags & 0x0000FFFF) )
						pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_VALUE);
					else {
						// Ӱ��قłȂ����ނ�����
						if ( nNotModalCode >= 0 ) {
							nResult = g_ncArgv.nc.nGcode;
							g_ncArgv.nc.nGcode = nNotModalCode;
							pDataResult = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
							g_ncArgv.nc.nGcode = nResult;
							nNotModalCode = -1;
						}
						else
							pDataResult = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
					}
				}
				else
					bNCobj = TRUE;
				if ( enGcode == MAKEOBJ )
					g_ncArgv.nc.nGcode = nCode;
				else
					nNotModalCode = nCode;
			}
			else
				CheckGcodeOther(nCode);
			break;
		case 'F':
			g_ncArgv.dFeed = (*g_pfnFeedAnalyze)(it->substr(1));
			break;
		case ',':
			strComma = ::Trim(it->substr(1));	// ��ψȍ~���擾
#ifdef _DEBUG
			dbg1.printf("strComma=%s", strComma.c_str());
#endif
			break;
		case 'X':
		case 'Y':
		case 'Z':
		case 'R':
		case 'I':
		case 'J':
		case 'K':
		case 'P':
		case 'L':
		case 'D':
		case 'H':
			nCode = (int)(strchr(g_szNdelimiter, it->at(0)) - g_szNdelimiter);
			// �l�擾
			if ( g_ncArgv.nc.nGcode>=81 && g_ncArgv.nc.nGcode<=89 ) {
				// �Œ軲�ق̓��ʏ���
				if ( nCode == NCA_K )		// K��Ȳè�ނ�
					g_ncArgv.nc.dValue[NCA_K] = atoi(it->substr(1).c_str());
				else if ( nCode == NCA_P )	// P(�޳�َ���)�͑��葬�x�Ɠ�������
					g_ncArgv.nc.dValue[NCA_P] = (*g_pfnFeedAnalyze)(it->substr(1));
				else
					g_ncArgv.nc.dValue[nCode] = GetNCValue(it->substr(1));
			}
			else if ( nCode < NCA_P )
				g_ncArgv.nc.dValue[nCode] = GetNCValue(it->substr(1));
			else
				g_ncArgv.nc.dValue[nCode] = atoi(it->substr(1).c_str());
			//
			g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[nCode];
			//
			if ( nCode <= NCA_L ) {
				if ( IsGcodeObject(g_ncArgv.nc.nGcode) != NOOBJ ) {
					g_ncArgv.nc.dwValFlags |= g_dwValFlags;	// �O���ٰ�ߕ�������
					bNCval = TRUE;
					g_dwValFlags = 0;
				}
			}
			else {		// D, H
				// ���W�ȊO�̒l�w���͎���ٰ�ߗp�ɑޔ�
				g_dwValFlags |= g_dwSetValFlags[nCode];
			}
			break;
		}
	} // End of for() iterator

	// M���ތ㏈��
	if ( bNCsub ) {
		if ( !(g_ncArgv.nc.dwValFlags & NCD_P) )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
		else if ( (nIndex=NC_SearchSubProgram(&nRepeat)) < 0 )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98);
		else {
			g_nSubprog++;
			// nRepeat���J��Ԃ�
			while ( nRepeat-- > 0 && IsThread() ) {
				for ( ii=nIndex; ii<nLoopCnt && IsThread(); ii++ ) {
					nResult = NC_GSeparater(ii, pDataResult);	// �ċA
					if ( nResult == 30 )
						return 30;
					else if ( nResult == 99 )
						break;
				}
			}
			// EOF�ŏI���Ȃ�M99���A����
			if ( ii >= nLoopCnt && nResult == 0 ) {
				if ( g_nSubprog > 0 )
					g_nSubprog--;
			}
		}
		return 0;
	}

	// NC�ް��o�^����
	if ( bNCobj || bNCval ) {
		// ���W�l����
		if ( !(g_ncArgv.nc.dwValFlags & 0x0000FFFF) )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_VALUE);
		else {
			// �Œ軲�ق�Ӱ��ٕ��
			if ( g_Cycle.bCycle )
				CycleInterpolate();
			// NC�ް��̓o�^
			// --- �ʎ�蓙�ɂ��Čv�Z���ڂ����邪�C
			// --- ���̎��_�ŵ�޼ު�ēo�^���Ă����Ȃ��ƐF�X�ʓ|(?)
			if ( nNotModalCode >= 0 ) {
				nResult = g_ncArgv.nc.nGcode;
				g_ncArgv.nc.nGcode = nNotModalCode;
				pData = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
				g_ncArgv.nc.nGcode = nResult;
			}
			else
				pData = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
			// �ʎ���޼ު�Ă̓o�^
			if ( !g_strComma.empty() )
				MakeChamferingObject(pBlock, pDataResult, pData);
			// ��ۯ����̍X�V
			pBlock->SetBlockToNCdata(pData, g_pDoc->GetNCsize());
			// ���̏�����
			pDataResult = pData;
		}
	}

	// ���̏����ɔ����Ķ�ϕ������ÓI�ϐ���
	if ( pData )
		g_strComma = strComma;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// �⏕�֐�

// �؍��ވȊO�̏d�v��G���ތ���
void CheckGcodeOther(int nCode)
{
	switch ( nCode ) {
	// �ضٕ��ʎw��
	case 17:
		g_ncArgv.nc.enPlane = XY_PLANE;
		break;
	case 18:
		g_ncArgv.nc.enPlane = XZ_PLANE;
		break;
	case 19:
		g_ncArgv.nc.enPlane = YZ_PLANE;
		break;
	// �H��a�␳
	case 40:
		g_ncArgv.nc.dwValFlags &= ~NCD_CORRECT;
		break;
	case 41:
		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_L;
		break;
	case 42:
		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_R;
		break;
	// ܰ����W�n
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// �Œ軲�ٷ�ݾ�
	case 80:
		g_Cycle.bCycle = FALSE;
		g_Cycle.dVal[0] = g_Cycle.dVal[1] = g_Cycle.dVal[2] = HUGE_VAL;
		break;
	// ��޿ح��, �ݸ����
	case 90:
		g_ncArgv.bAbs = TRUE;
		break;
	case 91:
		g_ncArgv.bAbs = FALSE;
		break;
	// �Œ軲�ٕ��A
	case 98:
		g_ncArgv.bInitial = TRUE;
		break;
	case 99:
		g_ncArgv.bInitial = FALSE;
		break;
	}
}

// �����۸��т̌���
int NC_SearchSubProgram(int *pRepeat)
{
	int		nProg, i, n,
			nLoopCnt = g_pDoc->GetNCBlockSize();
	CString	strProg;

	if ( g_ncArgv.nc.dwValFlags & NCD_L ) {
		*pRepeat = (int)g_ncArgv.nc.dValue[NCA_L];
		nProg    = (int)g_ncArgv.nc.dValue[NCA_P];
	}
	else {
		// L:�J��Ԃ������w�肳��Ă��Ȃ���΁C
		// [times][number] (n��:4��) ���擾
		CString	strBuf;
		strBuf.Format("%d", (int)g_ncArgv.nc.dValue[NCA_P]);
		n = strBuf.GetLength();
		if ( n > 4 ) {
			*pRepeat = atoi(strBuf.Left(n-4));
			nProg    = atoi(strBuf.Right(4));
		}
		else {
			*pRepeat = 1;
			nProg    = (int)g_ncArgv.nc.dValue[NCA_P];
		}
	}

	strProg.Format("(O(0)*%d)($|[^0-9])", nProg);	// ���K�\��(Oxxxx��ϯ�����)
	regex	r(strProg);

	// ���������ۯ����猟��
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		if ( regex_search((LPCTSTR)(g_pDoc->GetNCblock(i)->GetStrGcode()), r) )
			return i;
	}

	// �@�B���̫��ނ���̧�ٌ���
	CString	strFile( SearchFolder(r) );
	if ( strFile.IsEmpty() )
		return -1;

	// �n�ԍ������݂����NC��ۯ��̑}�� ->�u���وʒu�ɓǂݍ��݁v����ۯ��ǉ�
	n = nLoopCnt;
	if ( g_pDoc->SerializeInsertBlock(strFile, n, NCF_AUTOREAD, FALSE) ) {
		// �}����ۯ��̍ŏ�����NCF_FOLDER
		if ( n < nLoopCnt )	// ��ۯ��}�������s�̉\��������
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// ϸ���۸��т̌���
int NC_SearchMacroProgram(CNCblock* pBlock)
{
	extern	int		g_nDefaultMacroID[];	// MCOption.cpp
	
	CString	strBlock(pBlock->GetStrGcode());
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	// pMCopt->IsMacroSearch() �������ς�
	regex	r(pMCopt->GetMacroStr(MCMACROCODE));
	if ( !regex_search((LPCTSTR)strBlock, r) )
		return -1;
	// 5�K�w�ȏ�̌Ăяo���ʹװ(4�K�w�܂�)
	if ( g_nSubprog+1 >= 5 ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98L);
		return -1;
	}

	// ϸۈ����̉��
	CString	strMacroFile,	// �ꎞ�o��̧��
			strArgv(pMCopt->GetMacroStr(MCMACROARGV)),	// ����
			strKey, strPath, strFile;
	int		nID;
	TCHAR	szPath[_MAX_PATH], szFile[_MAX_PATH];
	for ( int i=0; i<5/*SIZEOF(g_nDefaultMacroID)*/; i++ ) {
		nID = g_nDefaultMacroID[i];
		strKey = pMCopt->MakeMacroCommand(nID);
		switch ( i ) {
		case 0:		// MachineFile
			strArgv.Replace(strKey, pMCopt->GetMCHeadFileName());
			break;
		case 1:		// MacroCode
			strArgv.Replace(strKey, strBlock);
			break;
		case 2:		// MacroFolder
			strArgv.Replace(strKey, pMCopt->GetMacroStr(MCMACROFOLDER));
			break;
		case 3:		// CurrentFolder
			::Path_Name_From_FullPath(g_pDoc->GetCurrentFileName(), strPath, strFile);
			strArgv.Replace(strKey, strPath.Left(strPath.GetLength()-1));
			break;
		case 4:		// MacroResult
			::GetTempPath(_MAX_PATH, szPath);
			::GetTempFileName(szPath, AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3)/*ncd*/,
				0, szFile);
			strMacroFile = szFile;
			strArgv.Replace(strKey, szFile);
			break;
		}
	}
	// ϸەϊ�I/F�N��
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		pMCopt->GetMacroStr(MCMACROIF), strArgv, FALSE, TRUE);
	// ϸۓW�J�ꎞ̧�ق�o�^ -> �޷���Ĕj����ɏ���
	g_pDoc->AddMacroFile(strMacroFile);
	// ��ۯ��}��
	int	n = g_pDoc->GetNCBlockSize();
	if ( g_pDoc->SerializeInsertBlock(strMacroFile, n, NCF_AUTOREAD, FALSE) ) {
		// �}����ۯ��̍ŏ�����NCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// ��ۯ��}�������s�̉\��������
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

int NC_NoSearchMacro(CNCblock*)
{
	return -1;
}

CNCdata* MakeChamferingObject(CNCblock* pBlock, CNCdata* pData1, CNCdata* pData2)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeChamferingObject()", DBG_BLUE);
#endif
	// �ް�����
	if ( pData1->GetGtype() != G_TYPE || pData2->GetGtype() != G_TYPE ||
			pData1->GetGcode() < 1 || pData1->GetGcode() > 3 ||
			pData2->GetGcode() < 1 || pData2->GetGcode() > 3 ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_GTYPE);
		return NULL;
	}
	if ( pData1->GetPlane() != pData2->GetPlane() ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_PLANE);
		return NULL;
	}
	TCHAR	cCham = g_strComma[0];
	if ( cCham != 'R' && cCham != 'C' ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CHAMFERING);
		return NULL;
	}

	double	r1, r2, cr = fabs(atof(g_strComma.substr(1).c_str()));
	CPointD	pts, pte, pto;
	boost::optional<CPointD>	ptResult;
	BOOL	bResult;

	// �v�Z�J�n
	if ( cCham == 'C' )
		r1 = r2 = cr;
	else {
		// ��ŰR�̏ꍇ�́C�ʎ��ɑ�������C�l�̌v�Z
		tie(bResult, pto, r1, r2) = pData1->CalcRoundPoint(pData2, cr);
		if ( !bResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_INTERSECTION);
			return NULL;
		}
	}

	// pData1(�O�̵�޼ު��)�̏I�_��␳
	ptResult = pData1->SetChamferingPoint(FALSE, r1);
	if ( !ptResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
		return NULL;
	}
	pts = *ptResult;
	// pData2(���̵�޼ު��)�̎n�_��␳
	ptResult = pData2->SetChamferingPoint(TRUE, r2);
	if ( !ptResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
		return NULL;
	}
	pte = *ptResult;

#ifdef _DEBUG
	dbg.printf("%c=%f, %f", cCham, r1, r2);
	dbg.printf("pts=(%f, %f)", pts.x, pts.y);
	dbg.printf("pte=(%f, %f)", pte.x, pte.y);
#endif

	// pts, pte �Ŗʎ���޼ު�Ă̐���
	NCARGV	ncArgv;
	ZeroMemory(&ncArgv, sizeof(NCARGV));
	ncArgv.bAbs			= TRUE;
	ncArgv.dFeed		= pData1->GetFeed();
	ncArgv.nc.nLine		= pData1->GetStrLine();
	ncArgv.nc.nGtype	= G_TYPE;
	ncArgv.nc.enPlane	= pData1->GetPlane();
	// ���W�l�̾��
	switch ( pData1->GetPlane() ) {
	case XY_PLANE:
		ncArgv.nc.dValue[NCA_X] = pte.x;
		ncArgv.nc.dValue[NCA_Y] = pte.y;
		ncArgv.nc.dwValFlags = NCD_X|NCD_Y;
		break;
	case XZ_PLANE:
		ncArgv.nc.dValue[NCA_X] = pte.x;
		ncArgv.nc.dValue[NCA_Z] = pte.y;
		ncArgv.nc.dwValFlags = NCD_X|NCD_Z;
		break;
	case YZ_PLANE:
		ncArgv.nc.dValue[NCA_Y] = pte.x;
		ncArgv.nc.dValue[NCA_Z] = pte.y;
		ncArgv.nc.dwValFlags = NCD_Y|NCD_Z;
		break;
	}

	if ( cCham == 'C' )
		ncArgv.nc.nGcode = 1;
	else {
		// ��ŰR�̏ꍇ�́C���߂���ŰR�̒��S(pto)�����]�������v�Z
		double	pa, pb;
		pts -= pto;		pte -= pto;
		if ( (pa=atan2(pts.y, pts.x)) < 0.0 )
			pa += 360.0*RAD;
		if ( (pb=atan2(pte.y, pte.x)) < 0.0 )
			pb += 360.0*RAD;
		if ( fabs(pa-pb) > 180.0*RAD ) {
			if ( pa > pb )
				pa -= 360.0*RAD;
			else
				pb -= 360.0*RAD;
		}
		ncArgv.nc.nGcode = pa > pb ? 2 : 3;
		ncArgv.nc.dValue[NCA_R] = cr;
		ncArgv.nc.dwValFlags |= NCD_R;
	}
	// ���ɓo�^���ꂽ�P�O�ɖʎ���޼ު�Ă�}��
	return g_pDoc->DataOperation(pData1, &ncArgv, g_pDoc->GetNCsize()-1, NCINS);
}

double FeedAnalyze_Dot(const string& str)
{
	return fabs(GetNCValue(str));
}

double FeedAnalyze_Int(const string& str)
{
	return fabs(atof(str.c_str()));
}

CString SearchFolder(regex& r)
{
	extern	LPCTSTR	gg_szWild;	// "*.";
	CString	strResult, strExt;
	LPVOID	pFunc;

	for ( int i=0; i<SIZEOF(g_strSearchFolder); i++ ) {
		if ( g_strSearchFolder[i].IsEmpty() )
			continue;
		// ̫��ނ�W���g���q�Ō���
		strResult = SearchFolder_Sub(i,
			gg_szWild + AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3),	// "." �����uncd�v
			r);
		if ( !strResult.IsEmpty() )
			return strResult;
		// �o�^�g���q�ł�̫��ތ���
		for ( int j=0; j<2/*EXT_ADN,EXT_DLG*/; j++ ) {
			const CMapStringToPtr* pMap = AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetExtMap((EXTTYPE)j);
			for ( POSITION pos=pMap->GetStartPosition(); pos; ) {
				pMap->GetNextAssoc(pos, strExt, pFunc);
				strResult = SearchFolder_Sub(i, gg_szWild + strExt, r);
				if ( !strResult.IsEmpty() )
					return strResult;
			}
		}
	}

	return CString();
}

CString	SearchFolder_Sub(int n, LPCTSTR lpszFind, regex& r)
{
	CString	strFile;
	HANDLE	hFind;
	WIN32_FIND_DATA	fd;
	DWORD	dwFlags = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;

	if ( (hFind=::FindFirstFile(g_strSearchFolder[n]+lpszFind, &fd)) != INVALID_HANDLE_VALUE ) {
		do {
			strFile = g_strSearchFolder[n] + fd.cFileName;
			if ( !(fd.dwFileAttributes & dwFlags) &&
					regex_search(fd.cFileName, r) &&
					SearchProgNo(strFile, r) )
				return strFile;
		} while ( ::FindNextFile(hFind, &fd) );
		::FindClose(hFind);
	}

	return CString();
}

BOOL SearchProgNo(LPCTSTR lpszFile, regex& r)
{
	// ̧��ϯ��ݸނ�����۸��єԍ�(������)�̑��݊m�F
	BOOL	bResult = FALSE;
	CFile	fp;

	if ( fp.Open(lpszFile, CFile::modeRead) ) {
		HANDLE hMap = CreateFileMapping((HANDLE)(fp.m_hFile), NULL,
							PAGE_READONLY, 0, 0, NULL);
		if ( hMap ) {
			LPCTSTR pMap = (LPCTSTR)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
			if ( pMap ) {
				if ( regex_search(pMap, r) )
					bResult = TRUE;
				UnmapViewOfFile(pMap);
			}
			CloseHandle(hMap);
		}
		fp.Close();
	}

	return bResult;
}

void CycleInterpolate(void)
{
	extern	const	DWORD	g_dwSetValFlags[];

	// �O�̂��߂�����
	if ( g_ncArgv.nc.nGcode<81 || g_ncArgv.nc.nGcode>89 ) {
		g_Cycle.bCycle = FALSE;
		return;
	}

	int	z;
	// ����ʂɑ΂��钼����
	switch ( g_ncArgv.nc.enPlane ) {
	case XY_PLANE:
		z = NCA_Z;
		break;
	case XZ_PLANE:
		z = NCA_Y;
		break;
	case YZ_PLANE:
		z = NCA_X;
		break;
	}

	// �Œ軲�ق̍��W���
	if ( g_ncArgv.nc.dwValFlags & g_dwSetValFlags[z] )
		g_Cycle.dVal[0] = g_ncArgv.nc.dValue[z];
	else if ( g_Cycle.dVal[0] != HUGE_VAL ) {
		g_ncArgv.nc.dValue[z] = g_Cycle.dVal[0];
		g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[z];
	}

	if ( g_ncArgv.nc.dwValFlags & NCD_R )
		g_Cycle.dVal[1] = g_ncArgv.nc.dValue[NCA_R];
	else if ( g_Cycle.dVal[1] != HUGE_VAL ) {
		g_ncArgv.nc.dValue[NCA_R] = g_Cycle.dVal[1];
		g_ncArgv.nc.dwValFlags |= NCD_R;
	}

	if ( g_ncArgv.nc.dwValFlags & NCD_P )
		g_Cycle.dVal[2] = g_ncArgv.nc.dValue[NCA_P];
	else if ( g_Cycle.dVal[2] != HUGE_VAL ) {
		g_ncArgv.nc.dValue[NCA_P] = g_Cycle.dVal[2];
		g_ncArgv.nc.dwValFlags |= NCD_P;
	}
}

// �ϐ�������
void InitialVariable(void)
{
	int		i;
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	g_pfnFeedAnalyze = pMCopt->GetFDot()==0 ? &FeedAnalyze_Int : &FeedAnalyze_Dot;
	g_ncArgv.nc.nGtype = G_TYPE;
	g_ncArgv.nc.nGcode = pMCopt->GetModalSetting(MODALGROUP0);
	switch ( pMCopt->GetModalSetting(MODALGROUP1) ) {
	case 1:
		g_ncArgv.nc.enPlane = XZ_PLANE;
		break;
	case 2:
		g_ncArgv.nc.enPlane = YZ_PLANE;
		break;
	default:	// or �u0�v
		g_ncArgv.nc.enPlane = XY_PLANE;
		break;
	}
	g_ncArgv.nc.nErrorCode = 0;
	g_ncArgv.nc.dwValFlags = 0;
	for ( i=0; i<NCXYZ; i++ )
		g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
	for ( ; i<VALUESIZE; i++ )
		g_ncArgv.nc.dValue[i] = 0.0;
	g_ncArgv.bAbs = pMCopt->GetModalSetting(MODALGROUP3) == 0 ? TRUE : FALSE;
	g_ncArgv.bInitial = pMCopt->GetModalSetting(MODALGROUP4) == 0 ? TRUE : FALSE;;
	g_ncArgv.dFeed = pMCopt->GetFeed();

	g_Cycle.bCycle = FALSE;
	g_Cycle.dVal[0] = g_Cycle.dVal[1] = g_Cycle.dVal[2] = HUGE_VAL;

	g_dwValFlags = 0;
	g_nSubprog = 0;
	g_strComma.clear();

	g_pfnSearchMacro = pMCopt->IsMacroSearch() ? &NC_SearchMacroProgram : &NC_NoSearchMacro;
	// ���ĂƎw��̫��ނ̏�����
	CString	strFile;	// dummy
	::Path_Name_From_FullPath(g_pDoc->GetCurrentFileName(), g_strSearchFolder[0], strFile);
	g_strSearchFolder[1] = pMCopt->GetMacroStr(MCMACROFOLDER);
	for ( i=0; i<SIZEOF(g_strSearchFolder); i++ ) {
		if ( !g_strSearchFolder[i].IsEmpty() && g_strSearchFolder[i].Right(1) != "\\" )
			g_strSearchFolder[i] += "\\";
	}
	if ( g_strSearchFolder[0].CompareNoCase(g_strSearchFolder[1]) == 0 )
		g_strSearchFolder[1].Empty();
}
