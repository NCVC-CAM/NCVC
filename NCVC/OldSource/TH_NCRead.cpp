// TH_NCRead.cpp
// �m�b�R�[�h�̓����ϊ�
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "MainFrm.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"
#include "NCVCdefine.h"
#include "Sstring.h"

#include <stdlib.h>
#include <math.h>
#include <memory.h>

// Boost Regex Library
#include <boost/regex.hpp>

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

#define	IsThread()	g_pParent->IsThreadContinue()
//
static	CThreadDlg*	g_pParent;
static	CNCDoc*		g_pDoc;
static	NCARGV		g_ncArgv;		// NCVCaddin.h
static	int			g_nSubprog;		// �����۸��ьĂяo���̊K�w
static	CString		g_strComma;		// ��ψȍ~�̕�����

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
inline	double		GetNCValue(const CString& strBuf)
{
	double	dResult = atof(strBuf);
	if ( strBuf.Find('.') < 0 )
		dResult /= 1000.0;		// �����_���Ȃ���� 1/1000
	return dResult;
}
// G0�`G3, G8x �؍�C�ړ��C�Œ軲�َw�茟��
inline	BOOL		IsGcodeCutter(int nCode)
{
	BOOL	bResult = FALSE;

	if ( nCode >= 0 && nCode <= 3 ) {
		g_Cycle.bCycle = FALSE;
		bResult = TRUE;
	}
	else if ( nCode >= 81 && nCode <= 89 ) {
		g_Cycle.bCycle = TRUE;
		bResult = TRUE;
	}
	return bResult;
}

// ��͊֐�
static	int		NC_GSeparater(int, CNCdata*&);
static	BOOL	NC_NSeparater(const CString&);
static	BOOL	CheckGcodeOther(int nCode);
static	int		NC_SearchSubProgram(int*);
typedef	int		(*PFNSEARCHMACRO)(CNCblock*);
static	int		NC_SearchMacroProgram(CNCblock*);
static	int		NC_NoSearchMacro(CNCblock*);
static	PFNSEARCHMACRO	g_pfnSearchMacro;
static	BOOL	MakeChamferingObject(CNCdata*, CNCdata*);

// F���Ұ�, �޳�َ��Ԃ̉���
typedef double (*PFNFEEDANALYZE)(const CString&);
static	double		FeedAnalyze_Dot(const CString&);
static	double		FeedAnalyze_Int(const CString&);
static	PFNFEEDANALYZE	g_pfnFeedAnalyze;
inline	int			NC_Feed(const CString& strFeed, int nType)
{
	int	nGcode = atoi(strFeed);
	if ( nType == F_TYPE )
		g_ncArgv.dFeed = (*g_pfnFeedAnalyze)(strFeed);
	return nGcode;
}

// �����ہCϸۂ̌���
static	CString		g_strSearchFolder[2];	// ���ĂƎw��̫���
static	CString		SearchFolder(boost::regex&);
static	BOOL		SearchProgNo(LPCTSTR, boost::regex&);

// ̪��ލX�V
static	void		SendFaseMessage(void);

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

	// ̪���1 G���ޕ���
	SendFaseMessage();
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
// G���ނ̕���(�ċA�֐�)
// �װ�ް��́CNCF_ERROR �ޯ��׸ނőΏ�
int NC_GSeparater(int i, CNCdata*& pDataResult)
{
	extern	LPCTSTR	gg_szComma;		// StdAfx.cpp
	extern	LPCTSTR	g_szGdelimiter;	// "GSMOF" NCDoc.cpp
	extern	LPCTSTR	g_szGtester[];
	extern	const	DWORD	g_dwSetValFlags[];
	static	CString	ss_strExclude("[({%");
	static	STRING_CUTTER_EX	cutGcode(CString(gg_szComma)+g_szGdelimiter);
	static	STRING_TESTER_EX	tstGcode(GTYPESIZE, g_szGtester, FALSE);

#ifdef _DEBUG
	CMagaDbg	dbg("NC_Gseparate()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_GREEN);
	dbg.printf("No.%004d Line=%s", i+1, g_pDoc->GetNCblock(i)->GetStrGcode());
#endif
	int			ii, nType, nCode, nResult, nIndex, nRepeat;
	BOOL		bNCadd = FALSE;
	CString		strGPiece, strComma;
	CNCdata*	pData = NULL;
	CNCblock*	pBlock = g_pDoc->GetNCblock(i);
	// �ϐ�������
	g_ncArgv.nc.nLine		= i;
	g_ncArgv.nc.dwFlags		= 0;
	g_ncArgv.nc.dwValFlags	= 0;

	// ϸےu�����
	if ( (nIndex=(*g_pfnSearchMacro)(pBlock)) >= 0 ) {
		g_nSubprog++;
		for ( ii=nIndex; ii<g_pDoc->GetNCBlockSize() && IsThread(); ii++ ) {
			nResult = NC_GSeparater(ii, pDataResult);	// �ċA
			if ( nResult == 30 )
				return 30;
			else if ( nResult == 99 )
				break;
			else if ( nResult != 0 ) {
				pBlock->SetBlockFlag(NCF_ERROR);
				continue;
			}
		}
		return 0;
	}

	// G���މ��
	cutGcode.Set(pBlock->GetStrGcode());
	while ( cutGcode.GiveAPieceEx(strGPiece) && IsThread() ) {
#ifdef _DEBUG
		dbg1.printf("G Cut=%s", strGPiece);
#endif
		// ���O����������
		if ( strGPiece.GetLength()<=0 || ss_strExclude.Find(strGPiece[0])>=0 )
			continue;

		// �������
		if ( strGPiece[0] == gg_szComma[0] ) {
			strComma = strGPiece.Mid(1);	// ��ψȍ~���擾
			strComma.TrimLeft();
#ifdef _DEBUG
			dbg1.printf("strComma=%s", strComma);
#endif
			continue;
		}

		// ��������
		nType = tstGcode.TestEx(strGPiece) - 1;
		if ( nType > G_TYPE ) {
			if ( (nCode=NC_Feed(strGPiece.Mid(1), nType)) <= 0 ) {
				pBlock->SetBlockFlag(NCF_ERROR);
				continue;
			}
			if ( nType != M_TYPE )
				continue;
			// M���ޏ���
			switch ( nCode ) {
			case 2:
			case 30:
				return 30;
			case 98:
				// 5�K�w�ȏ�̌Ăяo���ʹװ(4�K�w�܂�)
				if ( g_nSubprog+1 >= 5 ) {
					pBlock->SetBlockFlag(NCF_ERROR);
					continue;
				}
				if ( !NC_NSeparater(strGPiece) ||
					 !(g_ncArgv.nc.dwValFlags & g_dwSetValFlags[NCA_P]) ||
					 (nIndex=NC_SearchSubProgram(&nRepeat)) < 0 ) {
					pBlock->SetBlockFlag(NCF_ERROR);
					continue;
				}
				g_nSubprog++;
				// nRepeat���J��Ԃ�
				while ( nRepeat-- > 0 && IsThread() ) {
					for ( ii=nIndex; ii<g_pDoc->GetNCBlockSize() && IsThread(); ii++ ) {
						nResult = NC_GSeparater(ii, pDataResult);	// �ċA
						if ( nResult == 30 )
							return 30;
						else if ( nResult == 99 )
							break;
						else if ( nResult != 0 ) {
							pBlock->SetBlockFlag(NCF_ERROR);
							continue;
						}
					}
				}
				break;
			case 99:
				if ( g_nSubprog > 0 )
					g_nSubprog--;
				return 99;
			}
			continue;
		}
		// G���ޏ���
		if ( nType == G_TYPE ) {
			nCode = atoi(strGPiece.Mid(1));
			if ( IsGcodeCutter(nCode) ) {
				g_ncArgv.nc.nGcode = nCode;
				bNCadd = TRUE;
			}
			else if ( CheckGcodeOther(nCode) ) {
				g_ncArgv.nc.nGcode = nCode;
				bNCadd = TRUE;
			}
		}
		// G���ނɑ������W�l�C�܂��͍��W�P�Ǝw���̏���
		if ( NC_NSeparater(strGPiece) )
			bNCadd = TRUE;

	} // End of while()

	if ( bNCadd ) {
		// �Œ軲�ق�Ӱ��ٕ��
		if ( g_Cycle.bCycle )
			CycleInterpolate();
		// NC�ް��̓o�^
		pData = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
		// �ʎ���޼ު�Ă̓o�^
		if ( !g_strComma.IsEmpty() && !MakeChamferingObject(pDataResult, pData) )
			g_pDoc->GetNCblock(pDataResult->GetStrLine())->SetBlockFlag(NCF_ERROR);
		//
		pBlock->SetBlockToNCdata(pData, g_pDoc->GetNCsize());
		pDataResult = pData;
	}

	if ( pData )
		g_strComma = strComma;		// ���̏����ɔ����Ķ�ϕ������ÓI�ϐ���

	return 0;
}

//////////////////////////////////////////////////////////////////////
// G���ނɑ����l�̕���
BOOL NC_NSeparater(const CString& strGPiece)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZRIJKPLDH" NCDoc.cpp
	extern	LPCTSTR	g_szNtester[];
	extern	const	DWORD	g_dwSetValFlags[];
	static	STRING_CUTTER_EX	cutNcode(g_szNdelimiter);
	static	STRING_TESTER_EX	tstNcode(VALUESIZE, g_szNtester, FALSE);

#ifdef _DEBUG
	CMagaDbg	dbg("NC_NSeparater()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_CYAN);
#endif
	CString	strNPiece;
	DWORD	dwValFlags = 0;
	int		nType, nPiece = strGPiece.FindOneOf(g_szNdelimiter);

	if ( nPiece < 0 )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("%s", strGPiece.Mid(nPiece));
#endif
	cutNcode.Set(strGPiece.Mid(nPiece));
	while ( cutNcode.GiveAPieceEx(strNPiece) && IsThread() ) {
		nType = tstNcode.TestEx(strNPiece) - 1;
#ifdef _DEBUG
		dbg1.printf("N Cut=%s Type=%d", strNPiece, nType);
#endif
		if ( nType<0 || nType>=VALUESIZE )
			continue;
		else if ( nType < GVALSIZE /*NCA_P*/) {
			// �Œ軲�ق�K��Ȳè�ނ�
			if ( g_ncArgv.nc.nGcode>=81 && g_ncArgv.nc.nGcode<=89 && nType==NCA_K )
				g_ncArgv.nc.dValue[NCA_K] = atoi(strNPiece.Mid(1));
			else
				g_ncArgv.nc.dValue[nType] = GetNCValue(strNPiece.Mid(1));
		}
		else {
			// �Œ軲�ق�P(�޳�َ���)�͑��葬�x�Ɠ�������
			if ( g_ncArgv.nc.nGcode>=81 && g_ncArgv.nc.nGcode<=89 && nType==NCA_P )
				g_ncArgv.nc.dValue[NCA_P] = (*g_pfnFeedAnalyze)(strNPiece.Mid(1));
			else
				g_ncArgv.nc.dValue[nType] = atoi(strNPiece.Mid(1));
		}
		g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[nType];
		dwValFlags |= g_dwSetValFlags[nType];
	}

	return dwValFlags == 0 ? FALSE : TRUE;
}

//////////////////////////////////////////////////////////////////////
// �⏕�֐�

// �؍��ވȊO�̏d�v��G���ތ���
//		FALSE : Modal and coordinates
//		TRUE  : CreateObject(itself)
BOOL CheckGcodeOther(int nCode)
{
	BOOL	bResult = FALSE;

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
	// ܰ����W�n
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// ۰�ٍ��W�n(�؍��ވȊO�ŗB��̵�޼ު�Đ���)
	case 52:
	case 92:
		bResult = TRUE;
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

	return bResult;
}

// �����۸��т̌���
int NC_SearchSubProgram(int *pRepeat)
{
	int		nProg, i, n;
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
	boost::regex	r(strProg);

	// ���������ۯ����猟��
	for ( i=0; i<g_pDoc->GetNCBlockSize() && IsThread(); i++ ) {
		if ( boost::regex_search(g_pDoc->GetNCblock(i)->GetStrGcode(), r) )
			return i;
	}

	// �@�B���̫��ނ���̧�ٌ���
	CString	strFile( SearchFolder(r) );
	if ( strFile.IsEmpty() )
		return -1;

	// �n�ԍ������݂����NC��ۯ��̑}�� ->�u���وʒu�ɓǂݍ��݁v����ۯ��ǉ�
	n = g_pDoc->GetNCBlockSize();
	if ( g_pDoc->SerializeInsertBlock(strFile, n, NCF_AUTOREAD, FALSE) ) {
		// ��ۯ����}������Ă��Ȃ��ꍇ������̂ŔO�̂�������
		if ( n < g_pDoc->GetNCBlockSize() )
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
	CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	// pMCopt->IsMacroSearch() �������ς�
	boost::regex	r(pMCopt->GetMacroStr(MCMACROCODE));
	if ( !boost::regex_search(strBlock, r) )
		return -1;
	// 5�K�w�ȏ�̌Ăяo���ʹװ(4�K�w�܂�)
	if ( g_nSubprog+1 >= 5 ) {
		pBlock->SetBlockFlag(NCF_ERROR);
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
		// ��ۯ����}������Ă��Ȃ��ꍇ������̂ŔO�̂�������
		if ( n < g_pDoc->GetNCBlockSize() )
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

int NC_NoSearchMacro(CNCblock*)
{
	return -1;
}

BOOL MakeChamferingObject(CNCdata* pData1, CNCdata* pData2)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeChamferingObject()", DBG_BLUE);
#endif
	// �ް�����
	if ( pData1->GetGtype() != G_TYPE || pData2->GetGtype() != G_TYPE ||
			pData1->GetGcode() < 1 || pData1->GetGcode() > 3 ||
			pData2->GetGcode() < 1 || pData2->GetGcode() > 3 ||
			pData1->GetPlane() != pData2->GetPlane() )
		return FALSE;
	TCHAR	cCham = g_strComma[0];
	if ( cCham != 'R' && cCham != 'C' )
		return FALSE;

	double	r1, r2, cr = fabs(atof(g_strComma.Mid(1)));
	CPointD	pts, pte, ptOrg;
	// �v�Z�J�n
	if ( cCham == 'C' )
		r1 = r2 = cr;
	else {
		// ��ŰR�̏ꍇ�́C�ʎ��ɑ�������C�l�̌v�Z
		if ( !pData1->CalcRoundPoint(pData2, cr, ptOrg, r1, r2) )
			return FALSE;
	}

	// pData1(�O�̵�޼ު��)�̏I�_��␳
	if ( !pData1->SetChamferingPoint(FALSE, r1, pts) )
		return FALSE;
	// pData2(���̵�޼ު��)�̎n�_��␳
	if ( !pData2->SetChamferingPoint(TRUE,  r2, pte) )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("%c=%f, %f", cCham, r1, r2);
	dbg.printf("pts=(%f, %f)", pts.x, pts.y);
	dbg.printf("pte=(%f, %f)", pte.x, pte.y);
#endif

	// pts, pte �Ŗʎ���޼ު�Ă̐���
	NCARGV	ncArgv;
	memcpy((void *)&ncArgv, &g_ncArgv, sizeof(NCARGV));	// ��۰��ٕϐ������߰
	ncArgv.bAbs		= TRUE;		// ��Βl�w��
	ncArgv.dFeed	= pData1->GetFeed();
	ncArgv.nc.nLine	= pData1->GetStrLine();
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
	default:
		return FALSE;
	}
	if ( cCham == 'C' )
		ncArgv.nc.nGcode = 1;
	else {
		// ��ŰR�̏ꍇ�́C���߂���ŰR�̒��S(ptOrg)�����]�������v�Z
		double	pa, pb;
		pts -= ptOrg;	pte -= ptOrg;
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
	pData2 = g_pDoc->DataOperation(pData1, &ncArgv, g_pDoc->GetNCsize()-1, NCINS);

	return TRUE;
}

double FeedAnalyze_Dot(const CString& strBuf)
{
	return fabs(GetNCValue(strBuf));
}

double FeedAnalyze_Int(const CString& strBuf)
{
	return fabs(atof(strBuf));
}

inline	CString	SearchFolder_Sub(int n, LPCTSTR lpszFind, boost::regex& r)
{
	CString	strFile;
	HANDLE	hFind;
	WIN32_FIND_DATA	fd;
	DWORD	dwFlags = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;

	if ( (hFind=::FindFirstFile(g_strSearchFolder[n]+lpszFind, &fd)) != INVALID_HANDLE_VALUE ) {
		do {
			strFile = g_strSearchFolder[n] + fd.cFileName;
			if ( !(fd.dwFileAttributes & dwFlags) &&
					boost::regex_search(fd.cFileName, r) &&
					SearchProgNo(strFile, r) )
				return strFile;
		} while ( ::FindNextFile(hFind, &fd) );
		::FindClose(hFind);
	}

	return CString();
}

CString SearchFolder(boost::regex& r)
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
			for ( POSITION pos = pMap->GetStartPosition(); pos; ) {
				pMap->GetNextAssoc(pos, strExt, pFunc);
				strResult = SearchFolder_Sub(i, gg_szWild + strExt, r);
				if ( !strResult.IsEmpty() )
					return strResult;
			}
		}
	}

	return CString();
}

BOOL SearchProgNo(LPCTSTR lpszFile, boost::regex& r)
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
				if ( boost::regex_search(pMap, r) )
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
	// �O�̂��߂�����
	if ( g_ncArgv.nc.nGcode<81 || g_ncArgv.nc.nGcode>89 ) {
		g_Cycle.bCycle = FALSE;
		return;
	}

	if ( g_ncArgv.nc.dwValFlags & NCD_Z )
		g_Cycle.dVal[0] = g_ncArgv.nc.dValue[NCA_Z];
	else if ( g_Cycle.dVal[0] != HUGE_VAL ) {
		g_ncArgv.nc.dValue[NCA_Z] = g_Cycle.dVal[0];
		g_ncArgv.nc.dwValFlags |= NCD_Z;
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
	CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

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
	g_ncArgv.nc.dwFlags = g_ncArgv.nc.dwValFlags = 0;
	for ( i=0; i<NCXYZ; i++ )
		g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
	for ( ; i<VALUESIZE; i++ )
		g_ncArgv.nc.dValue[i] = 0.0;
	g_ncArgv.bAbs = pMCopt->GetModalSetting(MODALGROUP3) == 0 ? TRUE : FALSE;
	g_ncArgv.bInitial = pMCopt->GetModalSetting(MODALGROUP4) == 0 ? TRUE : FALSE;;
	g_ncArgv.dFeed = pMCopt->GetFeed();

	g_Cycle.bCycle = FALSE;
	g_Cycle.dVal[0] = g_Cycle.dVal[1] = g_Cycle.dVal[2] = HUGE_VAL;

	g_nSubprog = 0;
	g_strComma.Empty();

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

// ̪��ލX�V
void SendFaseMessage(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("NCDtoXYZ_Thread()", DBG_GREEN);
#endif
	CString	strMsg;
	VERIFY(strMsg.LoadString(IDS_READ_NCD));
	g_pParent->SetFaseMessage(strMsg);
}
