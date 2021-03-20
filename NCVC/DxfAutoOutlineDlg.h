// DxfAutoOutlineDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoOutlineDlg ダイアログ

class CDxfAutoOutlineDlg : public CDialogEx
{
	LPAUTOWORKINGDATA	m_pAuto;

public:
	CDxfAutoOutlineDlg(LPAUTOWORKINGDATA);

// ダイアログ データ
	enum { IDD = IDD_DXFEDIT_AUTOOUTLINE };
	CFloatEdit	m_dOffset;
	CIntEdit	m_nLoop;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
