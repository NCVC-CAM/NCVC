// TH_MakeNurbs.cpp
//		NURBS�ȖʗpNC����
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"		// NCMakeBase.h�p
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

// ��۰��ٕϐ���`
static	CThreadDlg*		g_pParent;
static	C3dModelDoc*	g_pDoc;
static	CNCMakeMillOpt*	g_pMakeOpt;
static	int				g_nFase;	// ̪��އ�

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)

// NC�����ɕK�v���ް��Q
static	CTypedPtrArrayEx<CPtrArray, CNCMakeMill*>	g_obMakeData;	// ���H�ް�

// ��ފ֐�
static	void	InitialVariable(void);			// �ϐ�������
static	void	SetStaticOption(void);			// �ÓI�ϐ��̏�����
static	BOOL	MakeNurbs_MainFunc(void);		// NC������Ҳ�ٰ��
static	BOOL	OutputNurbsCode(void);			// NC���ނ̏o��
static	function<void (const Coord&)>	g_pfnAddCoord;
static	void	AddCoord_Normal(const Coord&);
static	void	AddCoord_ZOrigin(const Coord&);

// ͯ�ް,̯�ް���̽�߼�ٺ��ސ���
static	void	AddCustomNurbsCode(const CString&);

// �C���ް��̐���
static inline	void	_AddMakeNurbsStr(const CString& strData)
{
	CNCMakeMill*	pNCD = new CNCMakeMill(strData);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

// ��޽گ�ފ֐�
static	CCriticalSection	g_csMakeAfter;	// MakeNurbs_AfterThread()�گ��ۯ���޼ު��
static	UINT	MakeNurbs_AfterThread(LPVOID);	// ��n���گ��

//////////////////////////////////////////////////////////////////////
// NURBS�ȖʗpNC�����گ��
//////////////////////////////////////////////////////////////////////

UINT MakeNurbs_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeNurbs_Thread() Start\n");
#endif
	int		nResult = IDCANCEL;

	// ��۰��ٕϐ�������
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc = static_cast<C3dModelDoc *>(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);

	// �������\��
	g_nFase = 0;
	SendFaseMessage(g_pParent, g_nFase, -1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;

	// ���ʂ� CMemoryException �͑S�Ă����ŏW��
	try {
		// NC������߼�ݵ�޼ު�Ă̐����Ƶ�߼�݂̓ǂݍ���
		g_pMakeOpt = new CNCMakeMillOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKENURBS)->GetHead());
		// NC������ٰ�ߑO�ɕK�v�ȏ�����
		InitialVariable();
		// �������Ƃɕω��������Ұ���ݒ�
		SetStaticOption();
		// �ϐ��������گ�ނ̏����҂�
		g_csMakeAfter.Lock();		// �گ�ޑ���ۯ���������܂ő҂�
		g_csMakeAfter.Unlock();
		// �������蓖��
		g_obMakeData.SetSize(0, 2048);
		// �����J�n
		BOOL bResult = MakeNurbs_MainFunc();
		if ( bResult )
			bResult = OutputNurbsCode();

		// �߂�l���
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

	// �I������
	_dp.SetDecimal3();
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��

	// ��������NC���ނ̏����گ��(�D��x��������)
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
	// ���W�l�̐���
	g_pfnAddCoord = g_pDoc->Get3dOption()->Get3dFlg(D3_FLG_ZORIGIN) ? &AddCoord_ZOrigin : &AddCoord_Normal;

	// ������߼�݂ɂ��ÓI�ϐ��̏�����
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
// NC����Ҳݽگ��
//////////////////////////////////////////////////////////////////////

BOOL MakeNurbs_MainFunc(void)
{
	int		mx, my, mz, i, j, k,
			fw = 0;
	Coord***	pScanCoord = g_pDoc->GetScanPathCoord();

	tie(mx, my) = g_pDoc->GetScanNumXY();
	SendFaseMessage(g_pParent, g_nFase, mx*my);

	// G����ͯ��(�J�n����)
	AddCustomNurbsCode(GetStr(MKNC_STR_HEADER));

	for ( i=0; i<mx && IsThread(); i++ ) {
		for ( j=0; j<my && IsThread(); j++ ) {
			mz = g_pDoc->GetScanNumZ(j);
			if ( fw == 0 ) {
				for ( k=0; k<mz && IsThread(); k++ ) {
					g_pfnAddCoord(pScanCoord[i][j][k]);
				}
			}
			else {
				for ( k=mz-1; k>=0 && IsThread(); k-- ) {
					g_pfnAddCoord(pScanCoord[i][j][k]);
				}
			}
			SetProgressPos(g_pParent, i*my+j);
			fw = 1 - fw;
		}
	}

	// G����̯��(�I������)
	AddCustomNurbsCode(GetStr(MKNC_STR_FOOTER));

	return IsThread();
}

void AddCoord_Normal(const Coord& c)
{
	CPoint3D		pt(c);

	CNCMakeMill* pNCD = new CNCMakeMill(pt, GetDbl(MKNC_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

void AddCoord_ZOrigin(const Coord& c)
{
	CPoint3D		pt(c);
	pt.z -= g_pDoc->Get3dOption()->Get3dDbl(D3_DBL_HEIGHT);

	CNCMakeMill* pNCD = new CNCMakeMill(pt, GetDbl(MKNC_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

//////////////////////////////////////////////////////////////////////

//	AddCustomNurbsCode() ����Ăяo��
class CMakeCustomNurbsCode : public CMakeCustomCode	// MakeCustomCode.h
{
	BOOL	m_bComment;		// Endmill���̃R�����g��}���������ǂ���

public:
	CMakeCustomNurbsCode() :
				CMakeCustomCode(g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "G92_INITIAL", "G92X", "G92Y", "G92Z",
			"SPINDLE", "G0XY_INITIAL"
		};
		// ���ް�ǉ�
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
		// �R�����g�}��������
		m_bComment = FALSE;
	}

	CString	ReplaceCustomCode(const string& str) {
		extern	LPCTSTR	gg_szReturn;		// "\n";
		extern	const	DWORD	g_dwSetValFlags[];
		int		nTestCode;
		float	dValue[VALUESIZE];
		CString	strResult;

		// ���׽�Ăяo��
		tie(nTestCode, strResult) = CMakeCustomCode::ReplaceCustomCode(str);
		if ( !strResult.IsEmpty() )
			return strResult;	// �u���ς݂Ȃ�߂�i������G�R�[�h�͗��Ȃ��j

		// �h��replace
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
				// '%'�̎��̍s�ɃR�����g��}��
				CString	strBuf;
				strBuf.Format(IDS_MAKENCD_ENDMILL, g_pDoc->Get3dOption()->Get3dDbl(D3_DBL_BALLENDMILL));
				strResult += gg_szReturn + strBuf;
				m_bComment = TRUE;
			}
		}

		if ( strResult[0]=='G' && !m_bComment ) {
			// �ŏ���G�R�[�h�̑O�ɃR�����g��}��
			CString	strBuf;
			strBuf.Format(IDS_MAKENCD_ENDMILL, g_pDoc->Get3dOption()->Get3dDbl(D3_DBL_BALLENDMILL));
			strResult = strBuf + gg_szReturn + strResult;
			m_bComment = TRUE;
		}

		return strResult;
	}
};

void AddCustomNurbsCode(const CString& strFileName)
{
	CString	strBuf, strResult;
	CMakeCustomNurbsCode		custom;
	string	str, strTok;
	tokenizer<custom_separator>	tokens(str);

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
		// ���̴װ�͐�������(�x���̂�)
	}
}

//////////////////////////////////////////////////////////////////////

// NC�����̸�۰��ٕϐ�������(��n��)�گ��
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
