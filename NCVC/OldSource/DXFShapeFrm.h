// DXFShapeFrm.h : �w�b�_�[ �t�@�C��
//

#if !defined(AFX_DXFSHAPEFRM_H__8C692F0D_FA3F_4973_903E_75A9A7E0FE53__INCLUDED_)
#define AFX_DXFSHAPEFRM_H__8C692F0D_FA3F_4973_903E_75A9A7E0FE53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeFrm �t���[��

class CDXFShapeFrm : public CFrameWnd
{
	CToolBar	m_wndToolBar;

protected:
	CDXFShapeFrm();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^�B
	DECLARE_DYNCREATE(CDXFShapeFrm)

// �A�g���r���[�g
public:

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDXFShapeFrm)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	virtual ~CDXFShapeFrm();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CDXFShapeFrm)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_DXFSHAPEFRM_H__8C692F0D_FA3F_4973_903E_75A9A7E0FE53__INCLUDED_)
